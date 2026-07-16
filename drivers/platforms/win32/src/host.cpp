#include <alia/platforms/win32/host.h>

#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/host_window.h>
#include <alia/abi/ui/system/input_processing.h>
#include <alia/abi/ui/system/work.h>
#include <alia/platforms/win32/input_glue.h>
#include <alia/platforms/win32/ui_binding.h>
#include <alia/ui/system/work_internal.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>

#include <d3d11.h>
#include <dxgi1_3.h>
#include <timeapi.h>

#include <cmath>
#include <cstdio>
#include <new>

struct alia_win32_host
{
    // Outer overlapped window (frame, title bar, keyboard focus).
    HWND hwnd = nullptr;
    // Borderless child covering the parent client area. DXGI targets this so
    // composition is not tied to the non-client frame of the top-level HWND.
    // Mouse input is delivered here.
    HWND hwnd_render = nullptr;
    bool class_registered = false;
    bool running = false;
    bool vsync = true;
    bool continuous = true;
    bool probe_clear = false;
    bool fullscreen = false;
    bool tracking_mouse_leave = false;

    // Windowed placement saved across fullscreen toggles.
    WINDOWPLACEMENT windowed_placement{};
    LONG windowed_style = 0;

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain1* swapchain = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    HANDLE frame_latency_waitable = nullptr;
    // Full backbuffer allocation (may be larger than the visible source).
    UINT buffer_width = 0;
    UINT buffer_height = 0;
    // Active present/source size (SetSourceSize) and UI drawable size.
    UINT width = 0;
    UINT height = 0;
    UINT client_width = 0;
    UINT client_height = 0;

    alia_host_frame_handler frame{};
    alia_win32_ui_binding binding{};
    alia_window_state_handler on_window_state_changed{};

    LARGE_INTEGER qpc_freq{};
    uint64_t frame_index = 0;

    // True while the Win32 size/move modal loop is active.
    bool in_size_move = false;
    // Guards against re-entrant present from SetWindowPos / WM_SIZE.
    bool presenting = false;

    // Live-resize present policy: prefer immediate WM_SIZE presents when the
    // frame-latency slot is free; if size events overlap in-flight frames,
    // coalesce via a timer until things calm down.
    uint64_t size_serial = 0;
    uint64_t drawn_size_serial = 0;
    bool size_move_coalesce = false;
    int size_move_calm_frames = 0;
    bool size_move_timer_armed = false;
    // True while we have raised the system timer resolution (timeBeginPeriod)
    // for the duration of the size/move modal loop.
    bool timer_period_raised = false;
};

namespace {

wchar_t const* const k_window_class = L"AliaWin32Host";
wchar_t const* const k_render_class = L"AliaWin32Render";
UINT_PTR const k_size_move_timer_id = 1;
// Leave coalesce mode after this many presents with no intervening size
// change.
int const k_size_move_calm_frames_to_exit = 3;

alia_win32_host*
host_from_hwnd(HWND hwnd)
{
    return reinterpret_cast<alia_win32_host*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

void
host_read_client_size(alia_win32_host* host, UINT* width, UINT* height)
{
    RECT client{};
    GetClientRect(host->hwnd, &client);
    *width = UINT(client.right > client.left ? client.right - client.left : 0);
    *height
        = UINT(client.bottom > client.top ? client.bottom - client.top : 0);
}

void
host_note_size_event(alia_win32_host* host)
{
    ++host->size_serial;
}

void
host_size_move_ensure_timer(alia_win32_host* host)
{
    if (!host->hwnd || host->size_move_timer_armed)
        return;
    SetTimer(host->hwnd, k_size_move_timer_id, 16, nullptr);
    host->size_move_timer_armed = true;
}

void
host_size_move_kill_timer(alia_win32_host* host)
{
    if (!host->hwnd || !host->size_move_timer_armed)
        return;
    KillTimer(host->hwnd, k_size_move_timer_id);
    host->size_move_timer_armed = false;
}

// The size/move pump rides on a USER timer, whose accuracy is tied to the
// system timer resolution. Raise it to ~1ms for the duration of the modal
// loop so the pump fires at a steady cadence instead of the default ~15.6ms
// granularity (which makes move/resize animation visibly choppy).
void
host_size_move_raise_timer_period(alia_win32_host* host)
{
    if (host->timer_period_raised)
        return;
    if (timeBeginPeriod(1) == TIMERR_NOERROR)
        host->timer_period_raised = true;
}

void
host_size_move_restore_timer_period(alia_win32_host* host)
{
    if (!host->timer_period_raised)
        return;
    timeEndPeriod(1);
    host->timer_period_raised = false;
}

void
host_size_move_enter_coalesce(alia_win32_host* host)
{
    host->size_move_coalesce = true;
    host->size_move_calm_frames = 0;
    host_size_move_ensure_timer(host);
}

bool
host_layout_render_child(alia_win32_host* host, UINT width, UINT height)
{
    if (!host->hwnd_render)
        return false;

    RECT rc{};
    GetClientRect(host->hwnd_render, &rc);
    if (UINT(rc.right - rc.left) == width
        && UINT(rc.bottom - rc.top) == height)
        return true;

    // SWP_NOREDRAW avoids an intermediate DWM frame that stretches the old
    // swapchain buffers to the new child size before we Present.
    return SetWindowPos(
               host->hwnd_render,
               nullptr,
               0,
               0,
               int(width),
               int(height),
               SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW)
        != FALSE;
}

bool
host_ensure_render_child(alia_win32_host* host, UINT width, UINT height)
{
    if (!host->hwnd)
        return false;
    if (host->hwnd_render)
        return true;

    host->hwnd_render = CreateWindowExW(
        0,
        k_render_class,
        L"",
        WS_CHILD | WS_VISIBLE,
        0,
        0,
        int(width),
        int(height),
        host->hwnd,
        nullptr,
        GetModuleHandleW(nullptr),
        host);
    if (!host->hwnd_render)
    {
        std::fprintf(
            stderr, "[alia win32] render child CreateWindowExW failed\n");
        return false;
    }
    return true;
}

void
host_release_rtv(alia_win32_host* host)
{
    if (host->rtv)
    {
        host->rtv->Release();
        host->rtv = nullptr;
    }
}

bool
host_create_rtv(alia_win32_host* host)
{
    host_release_rtv(host);
    ID3D11Texture2D* backbuffer = nullptr;
    HRESULT hr = host->swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
    if (FAILED(hr) || !backbuffer)
        return false;

    D3D11_TEXTURE2D_DESC td{};
    backbuffer->GetDesc(&td);
    host->buffer_width = td.Width;
    host->buffer_height = td.Height;

    // The shaders output linear-space color and rely on the render target to
    // encode linear->sRGB on write. Flip-model swapchains can't use an _SRGB
    // buffer format, so we keep the buffer as UNORM and view it as _SRGB here
    // to get the encode (matching the OpenGL framebuffer's behavior).
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc{};
    rtv_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    hr = host->device->CreateRenderTargetView(
        backbuffer, &rtv_desc, &host->rtv);
    backbuffer->Release();
    return SUCCEEDED(hr) && host->rtv;
}

bool
host_bind_frame_latency(alia_win32_host* host)
{
    IDXGISwapChain2* swap2 = nullptr;
    HRESULT hr = host->swapchain->QueryInterface(IID_PPV_ARGS(&swap2));
    if (FAILED(hr) || !swap2)
        return false;
    swap2->SetMaximumFrameLatency(1);
    host->frame_latency_waitable = swap2->GetFrameLatencyWaitableObject();
    swap2->Release();
    return host->frame_latency_waitable != nullptr;
}

static UINT
host_max_u(UINT a, UINT b)
{
    return a > b ? a : b;
}

bool
host_resize_buffers(alia_win32_host* host, UINT width, UINT height)
{
    if (!host->swapchain || width == 0 || height == 0)
        return false;

    host->context->OMSetRenderTargets(0, nullptr, nullptr);
    host->context->Flush();
    host_release_rtv(host);

    HRESULT hr = host->swapchain->ResizeBuffers(
        0,
        width,
        height,
        DXGI_FORMAT_UNKNOWN,
        DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
    if (FAILED(hr) || !host_create_rtv(host))
        return false;
    return host->buffer_width == width && host->buffer_height == height;
}

bool
host_create_swapchain(alia_win32_host* host, UINT width, UINT height)
{
    IDXGIFactory2* factory = nullptr;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr) || !factory)
    {
        std::fprintf(
            stderr, "[alia win32] CreateDXGIFactory1 failed (0x%08lx)\n", hr);
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = width;
    desc.Height = height;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    // NONE: when the HWND is smaller than the buffer (grow-only shrink path),
    // present the top-left 1:1 instead of stretching the full buffer.
    desc.Scaling = DXGI_SCALING_NONE;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    hr = factory->CreateSwapChainForHwnd(
        host->device,
        host->hwnd_render,
        &desc,
        nullptr,
        nullptr,
        &host->swapchain);
    if (SUCCEEDED(hr))
        factory->MakeWindowAssociation(host->hwnd, DXGI_MWA_NO_ALT_ENTER);
    factory->Release();
    if (FAILED(hr))
    {
        std::fprintf(
            stderr,
            "[alia win32] CreateSwapChainForHwnd failed (0x%08lx)\n",
            hr);
        return false;
    }

    if (!host_bind_frame_latency(host))
    {
        std::fprintf(stderr, "[alia win32] frame latency bind failed\n");
        return false;
    }
    if (!host_create_rtv(host))
        return false;

    host->width = width;
    host->height = height;
    host->client_width = width;
    host->client_height = height;
    return host->buffer_width == width && host->buffer_height == height;
}

// Grow-only buffers during live resize; draw/UI use the client size into the
// top-left of the buffer. DXGI_SCALING_NONE clips (no stretch) when the HWND
// is smaller than the allocation. Trim the buffer once size/move ends.
//
// Avoid SetSourceSize here — it scaled UI incorrectly on shrink with a larger
// buffer on this path.
bool
host_sync_swapchain_to_client(alia_win32_host* host, bool commit_buffers)
{
    if (!host || !host->hwnd || !host->device || !host->context)
        return true;

    UINT client_w = 0;
    UINT client_h = 0;
    host_read_client_size(host, &client_w, &client_h);
    if (client_w == 0 || client_h == 0)
        return true;

    if (!host_ensure_render_child(host, client_w, client_h))
        return false;

    if (!commit_buffers)
    {
        host_layout_render_child(host, client_w, client_h);
        return host->rtv != nullptr;
    }

    if (!host->swapchain || !host->rtv)
    {
        if (!host_create_swapchain(host, client_w, client_h))
            return false;
        host_layout_render_child(host, client_w, client_h);
        return true;
    }

    bool const needs_grow
        = client_w > host->buffer_width || client_h > host->buffer_height;
    bool const allow_trim = !host->in_size_move;
    bool const needs_trim
        = allow_trim
       && (host->buffer_width > client_w || host->buffer_height > client_h);

    if (needs_grow || needs_trim)
    {
        UINT const new_w
            = needs_grow ? host_max_u(host->buffer_width, client_w) : client_w;
        UINT const new_h
            = needs_grow ? host_max_u(host->buffer_height, client_h)
                         : client_h;

        if (!host_resize_buffers(host, new_w, new_h))
        {
            host->swapchain->Release();
            host->swapchain = nullptr;
            host->frame_latency_waitable = nullptr;
            host->buffer_width = 0;
            host->buffer_height = 0;
            if (!host_create_swapchain(host, client_w, client_h))
                return false;
        }
    }

    // Drawable size tracks the client, which may be smaller than the buffer
    // while sizing (grow-only). Viewport/UI must use this, not buffer_*.
    host->width = client_w;
    host->height = client_h;
    host->client_width = client_w;
    host->client_height = client_h;

    host_layout_render_child(host, client_w, client_h);
    return true;
}

void
host_set_viewport(alia_win32_host* host)
{
    D3D11_VIEWPORT vp{};
    vp.Width = float(host->width);
    vp.Height = float(host->height);
    vp.MinDepth = 0.f;
    vp.MaxDepth = 1.f;
    host->context->RSSetViewports(1, &vp);

    D3D11_RECT scissor{0, 0, LONG(host->width), LONG(host->height)};
    host->context->RSSetScissorRects(1, &scissor);
    host->context->OMSetRenderTargets(1, &host->rtv, nullptr);
}

bool
host_create_device_and_swapchain(alia_win32_host* host)
{
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    D3D_FEATURE_LEVEL got = D3D_FEATURE_LEVEL_11_0;

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        levels,
        UINT(sizeof(levels) / sizeof(levels[0])),
        D3D11_SDK_VERSION,
        &host->device,
        &got,
        &host->context);
    if (FAILED(hr))
    {
        flags &= ~D3D11_CREATE_DEVICE_DEBUG;
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            levels,
            UINT(sizeof(levels) / sizeof(levels[0])),
            D3D11_SDK_VERSION,
            &host->device,
            &got,
            &host->context);
    }
    if (FAILED(hr))
    {
        std::fprintf(
            stderr, "[alia win32] D3D11CreateDevice failed (0x%08lx)\n", hr);
        return false;
    }

    UINT client_w = 0;
    UINT client_h = 0;
    host_read_client_size(host, &client_w, &client_h);
    if (client_w == 0)
        client_w = 1;
    if (client_h == 0)
        client_h = 1;

    if (!host_ensure_render_child(host, client_w, client_h))
        return false;
    if (!host_create_swapchain(host, client_w, client_h))
        return false;

    QueryPerformanceFrequency(&host->qpc_freq);
    std::fprintf(
        stderr,
        "[alia win32] D3D11 feature level 0x%x, %ux%u, waitable swapchain "
        "ok\n",
        unsigned(got),
        host->width,
        host->height);
    return true;
}

void
host_destroy_gpu(alia_win32_host* host)
{
    host_release_rtv(host);
    if (host->swapchain)
    {
        host->swapchain->Release();
        host->swapchain = nullptr;
    }
    host->frame_latency_waitable = nullptr;
    host->buffer_width = 0;
    host->buffer_height = 0;
    host->width = 0;
    host->height = 0;
    host->client_width = 0;
    host->client_height = 0;
    if (host->context)
    {
        host->context->ClearState();
        host->context->Flush();
        host->context->Release();
        host->context = nullptr;
    }
    if (host->device)
    {
        host->device->Release();
        host->device = nullptr;
    }
}

void
host_enter_fullscreen(alia_win32_host* host)
{
    if (host->fullscreen || !host->hwnd)
        return;

    host->windowed_placement.length = sizeof(host->windowed_placement);
    GetWindowPlacement(host->hwnd, &host->windowed_placement);
    host->windowed_style = GetWindowLongW(host->hwnd, GWL_STYLE);

    HMONITOR monitor = MonitorFromWindow(host->hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, &mi))
        return;

    SetWindowLongW(
        host->hwnd,
        GWL_STYLE,
        host->windowed_style
            & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
                | WS_SYSMENU));
    SetWindowPos(
        host->hwnd,
        HWND_TOP,
        mi.rcMonitor.left,
        mi.rcMonitor.top,
        mi.rcMonitor.right - mi.rcMonitor.left,
        mi.rcMonitor.bottom - mi.rcMonitor.top,
        SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
    host->fullscreen = true;
}

void
host_leave_fullscreen(alia_win32_host* host)
{
    if (!host->fullscreen || !host->hwnd)
        return;

    SetWindowLongW(host->hwnd, GWL_STYLE, host->windowed_style);
    SetWindowPlacement(host->hwnd, &host->windowed_placement);
    SetWindowPos(
        host->hwnd,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER
            | SWP_FRAMECHANGED);
    host->fullscreen = false;
}

void
host_toggle_fullscreen(alia_win32_host* host)
{
    if (host->fullscreen)
        host_leave_fullscreen(host);
    else
        host_enter_fullscreen(host);
}

// Present one frame. When `nonblocking` is true, returns false if the
// frame-latency slot is not free yet (caller should coalesce). When false,
// blocks on the waitable object when vsync or size/move backpressure applies.
bool
host_present_frame(alia_win32_host* host, bool nonblocking)
{
    if (!host || host->presenting)
        return false;
    host->presenting = true;

    uint64_t const serial_at_start = host->size_serial;

    if (!host_sync_swapchain_to_client(host, /*commit_buffers=*/true)
        || !host->rtv)
    {
        host->presenting = false;
        return false;
    }

    // Latency-1 backpressure: never Present without a free slot when we have
    // a waitable object. Size/move always uses it (even if vsync is off) so
    // we do not enqueue flip-model frames.
    bool const need_pace
        = host->frame_latency_waitable && (host->vsync || host->in_size_move);
    if (need_pace)
    {
        DWORD const wait_ms = nonblocking ? 0u : 1000u;
        DWORD const wr = WaitForSingleObjectEx(
            host->frame_latency_waitable, wait_ms, TRUE);
        if (nonblocking && wr != WAIT_OBJECT_0)
        {
            host->presenting = false;
            return false;
        }
        // Client size may have changed while waiting.
        if (!host_sync_swapchain_to_client(host, /*commit_buffers=*/true)
            || !host->rtv)
        {
            host->presenting = false;
            return false;
        }
    }

    host_set_viewport(host);

    if (host->binding.ui)
        alia_win32_host_sync_surface(host, host->binding.ui);

    if (host->frame.fn)
        host->frame.fn(host->frame.user_data);

    if (host->probe_clear)
    {
        double const t = double(host->frame_index) * (1.0 / 60.0);
        float const pulse = float(0.5 + 0.5 * std::sin(t * 2.0));
        float clear[4]
            = {0.08f + 0.12f * pulse, 0.10f, 0.18f + 0.20f * pulse, 1.f};
        host->context->ClearRenderTargetView(host->rtv, clear);
    }

    // Present(0)+RESTART during size/move: low Present blocking, drop stale
    // flips. Present(1) when idle with vsync.
    UINT const sync_interval = (host->vsync && !host->in_size_move) ? 1u : 0u;
    UINT const present_flags = host->in_size_move ? DXGI_PRESENT_RESTART : 0u;
    host->swapchain->Present(sync_interval, present_flags);
    ++host->frame_index;

    host->drawn_size_serial = serial_at_start;

    if (host->in_size_move)
    {
        if (host->size_serial != serial_at_start)
        {
            // A newer WM_SIZE arrived while this frame was in flight.
            host_size_move_enter_coalesce(host);
        }
        else if (host->size_move_coalesce)
        {
            ++host->size_move_calm_frames;
            if (host->size_move_calm_frames >= k_size_move_calm_frames_to_exit)
            {
                // Leave coalesce mode so WM_SIZE resumes its immediate-present
                // fast path, but keep the pump timer running for the rest of
                // the modal loop so paused resizes and moves keep animating.
                host->size_move_coalesce = false;
                host->size_move_calm_frames = 0;
            }
        }
    }

    host->presenting = false;
    return true;
}

void
host_present_frame(alia_win32_host* host)
{
    host_present_frame(host, /*nonblocking=*/false);
}

void
host_toggle_fullscreen_cb(void* user)
{
    host_toggle_fullscreen(static_cast<alia_win32_host*>(user));
}

bool
host_should_tick(alia_win32_host* host)
{
    if (host->continuous)
        return true;
    if (!host->binding.ui)
        return false;
    if (alia_ui_needs_tick(host->binding.ui))
        return true;
    alia_nanosecond_count wake_ns = 0;
    return alia_ui_next_wake_ns(host->binding.ui, &wake_ns);
}

DWORD
host_wait_timeout_ms(alia_win32_host* host)
{
    if (host->continuous || !host->binding.ui)
        return 0;

    alia_ui_system* ui = host->binding.ui;
    if (alia_ui_needs_tick(ui))
        return 0;

    alia_nanosecond_count wake_ns = 0;
    if (!alia_ui_next_wake_ns(ui, &wake_ns))
        return INFINITE;

    alia_ui_system_poll_clock(ui);
    alia_nanosecond_count const now = alia::steady_clock_now_ns();
    if (wake_ns <= now)
        return 0;

    alia_nanosecond_count const delta_ns = wake_ns - now;
    alia_nanosecond_count const delta_ms = delta_ns / 1000000ull;
    if (delta_ms >= INFINITE)
        return INFINITE - 1;
    return DWORD(delta_ms);
}

LRESULT CALLBACK
host_render_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    alia_win32_host* host = host_from_hwnd(hwnd);
    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    alia_ui_system* ui = host && host->binding.ui ? host->binding.ui : nullptr;
    float const x = float(GET_X_LPARAM(lParam));
    float const y = float(GET_Y_LPARAM(lParam));

    switch (msg)
    {
        case WM_MOUSEMOVE:
            if (ui)
            {
                if (host && !host->tracking_mouse_leave)
                {
                    TRACKMOUSEEVENT tme{};
                    tme.cbSize = sizeof(tme);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = hwnd;
                    TrackMouseEvent(&tme);
                    host->tracking_mouse_leave = true;
                }
                alia_win32_enqueue_mouse_motion(ui, x, y);
            }
            return 0;
        case WM_MOUSELEAVE:
            if (host)
                host->tracking_mouse_leave = false;
            if (ui)
                alia_ui_enqueue_mouse_loss(ui);
            return 0;
        case WM_LBUTTONDOWN:
            if (ui)
            {
                SetCapture(hwnd);
                alia_win32_enqueue_mouse_button(
                    ui, x, y, ALIA_BUTTON_LEFT, true);
            }
            return 0;
        case WM_LBUTTONUP:
            if (ui)
            {
                if (GetCapture() == hwnd)
                    ReleaseCapture();
                alia_win32_enqueue_mouse_button(
                    ui, x, y, ALIA_BUTTON_LEFT, false);
            }
            return 0;
        case WM_LBUTTONDBLCLK:
            if (ui)
                alia_win32_enqueue_double_click(ui, x, y, ALIA_BUTTON_LEFT);
            return 0;
        case WM_RBUTTONDOWN:
            if (ui)
            {
                SetCapture(hwnd);
                alia_win32_enqueue_mouse_button(
                    ui, x, y, ALIA_BUTTON_RIGHT, true);
            }
            return 0;
        case WM_RBUTTONUP:
            if (ui)
            {
                if (GetCapture() == hwnd)
                    ReleaseCapture();
                alia_win32_enqueue_mouse_button(
                    ui, x, y, ALIA_BUTTON_RIGHT, false);
            }
            return 0;
        case WM_MBUTTONDOWN:
            if (ui)
            {
                SetCapture(hwnd);
                alia_win32_enqueue_mouse_button(
                    ui, x, y, ALIA_BUTTON_MIDDLE, true);
            }
            return 0;
        case WM_MBUTTONUP:
            if (ui)
            {
                if (GetCapture() == hwnd)
                    ReleaseCapture();
                alia_win32_enqueue_mouse_button(
                    ui, x, y, ALIA_BUTTON_MIDDLE, false);
            }
            return 0;
        case WM_XBUTTONDOWN:
            if (ui)
            {
                SetCapture(hwnd);
                alia_button_t const button
                    = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
                        ? ALIA_BUTTON_4
                        : ALIA_BUTTON_5;
                alia_win32_enqueue_mouse_button(ui, x, y, button, true);
            }
            return TRUE;
        case WM_XBUTTONUP:
            if (ui)
            {
                if (GetCapture() == hwnd)
                    ReleaseCapture();
                alia_button_t const button
                    = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
                        ? ALIA_BUTTON_4
                        : ALIA_BUTTON_5;
                alia_win32_enqueue_mouse_button(ui, x, y, button, false);
            }
            return TRUE;
        case WM_ERASEBKGND:
            return 1;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK
host_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    alia_win32_host* host = host_from_hwnd(hwnd);

    switch (msg)
    {
        case WM_NCCREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        case WM_ENTERSIZEMOVE:
            if (host)
            {
                host->in_size_move = true;
                host->size_move_coalesce = false;
                host->size_move_calm_frames = 0;
                // The OS owns the message thread for the whole modal loop, so
                // our normal run loop cannot present. Drive a steady repaint
                // pump instead. This is what keeps animations alive while the
                // window is being moved (no WM_SIZE is ever sent) and while a
                // resize drag is paused (WM_SIZE stops firing).
                host_size_move_raise_timer_period(host);
                host_size_move_ensure_timer(host);
                host_note_size_event(host);
            }
            return 0;
        case WM_EXITSIZEMOVE:
            if (host)
            {
                host_size_move_kill_timer(host);
                host_size_move_restore_timer_period(host);
                host->in_size_move = false;
                host->size_move_coalesce = false;
                host->size_move_calm_frames = 0;
                host_sync_swapchain_to_client(host, /*commit_buffers=*/true);
                host_present_frame(host);
            }
            return 0;
        case WM_TIMER:
            if (host && wParam == k_size_move_timer_id)
            {
                // Steady pump for the size/move modal loop. Covers window
                // moves and paused resizes; WM_SIZE still presents immediately
                // for responsiveness while the pointer is actively resizing.
                if (host->in_size_move)
                    host_present_frame(host, /*nonblocking=*/false);
                return 0;
            }
            break;
        case WM_SIZE:
            if (host && host->device && wParam != SIZE_MINIMIZED)
            {
                host_note_size_event(host);
                if (host->in_size_move)
                {
                    // Prefer an immediate present at the latest size when the
                    // waitable slot is free. If not, enter coalesce mode and
                    // let the timer present once per slot.
                    if (host->size_move_coalesce)
                        return 0;
                    if (!host_present_frame(host, /*nonblocking=*/true))
                        host_size_move_enter_coalesce(host);
                }
                else
                {
                    host_sync_swapchain_to_client(
                        host, /*commit_buffers=*/true);
                }
            }
            return 0;
        case WM_ERASEBKGND:
            // DXGI owns the client pixels via the render child.
            return 1;
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            if (host && host->binding.ui)
            {
                short const delta = GET_WHEEL_DELTA_WPARAM(wParam);
                alia_win32_enqueue_wheel(
                    host->binding.ui, delta, msg == WM_MOUSEHWHEEL);
                return 0;
            }
            break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (wParam == VK_ESCAPE && host)
            {
                PostMessageW(hwnd, WM_CLOSE, 0, 0);
                return 0;
            }
            if (wParam == VK_F11 && host
                && (lParam & (1 << 30)) == 0) // ignore key repeat
            {
                host_toggle_fullscreen(host);
                return 0;
            }
            if (host && host->binding.ui)
            {
                alia_win32_enqueue_key(host->binding.ui, wParam, lParam, true);
                return 0;
            }
            break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (host && host->binding.ui)
            {
                alia_win32_enqueue_key(
                    host->binding.ui, wParam, lParam, false);
                return 0;
            }
            break;
        case WM_DESTROY:
            if (host)
            {
                host_size_move_kill_timer(host);
                host_size_move_restore_timer_period(host);
                host->running = false;
            }
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool
host_register_class(alia_win32_host* host)
{
    if (host->class_registered)
        return true;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = host_wndproc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)); // IDC_ARROW
    wc.hbrBackground = nullptr;
    wc.lpszClassName = k_window_class;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        std::fprintf(stderr, "[alia win32] RegisterClassExW(frame) failed\n");
        return false;
    }

    WNDCLASSEXW render{};
    render.cbSize = sizeof(render);
    render.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    render.lpfnWndProc = host_render_wndproc;
    render.hInstance = GetModuleHandleW(nullptr);
    render.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    render.hbrBackground = nullptr;
    render.lpszClassName = k_render_class;
    if (!RegisterClassExW(&render)
        && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        std::fprintf(stderr, "[alia win32] RegisterClassExW(render) failed\n");
        return false;
    }

    host->class_registered = true;
    return true;
}

} // namespace

extern "C" {

alia_win32_host*
alia_win32_host_create(void)
{
    return new (std::nothrow) alia_win32_host{};
}

void
alia_win32_host_destroy(alia_win32_host* host)
{
    if (!host)
        return;
    host_destroy_gpu(host);
    if (host->hwnd_render)
    {
        DestroyWindow(host->hwnd_render);
        host->hwnd_render = nullptr;
    }
    if (host->hwnd)
    {
        DestroyWindow(host->hwnd);
        host->hwnd = nullptr;
    }
    delete host;
}

bool
alia_win32_host_open(
    alia_win32_host* host, alia_win32_host_open_config const* config)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(config);

    // Must be set before window creation so GetClientRect / DXGI agree on
    // pixel sizes.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (!host_register_class(host))
        return false;

    host->vsync = config->vsync;

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!config->resizable)
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

    int width
        = config->window_state.width > 0 ? config->window_state.width : 1280;
    int height
        = config->window_state.height > 0 ? config->window_state.height : 720;

    HDC screen_dc = GetDC(nullptr);
    int const dpi = screen_dc ? GetDeviceCaps(screen_dc, LOGPIXELSX) : 96;
    if (screen_dc)
        ReleaseDC(nullptr, screen_dc);

    RECT rect{0, 0, width, height};
    AdjustWindowRectExForDpi(&rect, style, FALSE, 0, UINT(dpi));
    int const win_w = rect.right - rect.left;
    int const win_h = rect.bottom - rect.top;

    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    if (config->window_state.has_position)
    {
        x = config->window_state.x;
        y = config->window_state.y;
    }

    wchar_t title_w[256];
    char const* title = config->title ? config->title : "Alia";
    MultiByteToWideChar(CP_UTF8, 0, title, -1, title_w, 256);

    host->hwnd = CreateWindowExW(
        0,
        k_window_class,
        title_w,
        style,
        x,
        y,
        win_w,
        win_h,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        host);
    if (!host->hwnd)
    {
        std::fprintf(stderr, "[alia win32] CreateWindowExW failed\n");
        return false;
    }

    if (!host_create_device_and_swapchain(host))
    {
        DestroyWindow(host->hwnd);
        host->hwnd = nullptr;
        host_destroy_gpu(host);
        return false;
    }

    ShowWindow(host->hwnd, SW_SHOW);
    UpdateWindow(host->hwnd);

    if (config->window_state.fullscreen)
        host_enter_fullscreen(host);
    else if (config->window_state.maximized)
        ShowWindow(host->hwnd, SW_MAXIMIZE);

    return true;
}

void
alia_win32_host_run(
    alia_win32_host* host, alia_win32_host_run_config const* config)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(config);
    ALIA_ASSERT(host->hwnd);
    ALIA_ASSERT(config->frame.fn || config->probe_clear);
    ALIA_ASSERT(config->continuous || config->ui);

    host->frame = config->frame;
    host->continuous = config->continuous;
    host->probe_clear = config->probe_clear;
    host->on_window_state_changed = config->on_window_state_changed;
    host->running = true;

    if (config->ui)
    {
        alia_win32_host_install(host, config->ui);
        alia_host_window_ops const ops = {
            .toggle_fullscreen = host_toggle_fullscreen_cb,
            .user = host,
        };
        alia_ui_system_set_host_window_ops(config->ui, &ops);
        // First paint so event-driven apps are not blank until input.
        host_present_frame(host);
    }

    while (host->running)
    {
        if (host->continuous)
        {
            MSG msg;
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    host->running = false;
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            if (!host->running)
                break;
            if (IsIconic(host->hwnd))
            {
                WaitMessage();
                continue;
            }
            host_present_frame(host);
        }
        else
        {
            MSG msg;
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    host->running = false;
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            if (!host->running)
                break;

            if (IsIconic(host->hwnd))
            {
                MsgWaitForMultipleObjects(
                    0, nullptr, FALSE, INFINITE, QS_ALLINPUT);
                continue;
            }

            if (host_should_tick(host))
                host_present_frame(host);
            else
            {
                DWORD const timeout_ms = host_wait_timeout_ms(host);
                MsgWaitForMultipleObjects(
                    0, nullptr, FALSE, timeout_ms, QS_ALLINPUT);
            }
        }
    }
}

void
alia_win32_host_toggle_fullscreen(alia_win32_host* host)
{
    ALIA_ASSERT(host);
    host_toggle_fullscreen(host);
}

void
alia_win32_host_install(alia_win32_host* host, alia_ui_system* ui)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(ui);
    host->binding.ui = ui;
    host->binding.host = host;
}

void
alia_win32_host_sync_surface(alia_win32_host* host, alia_ui_system* ui)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(ui);
    ALIA_ASSERT(host->hwnd);

    UINT const dpi = GetDpiForWindow(host->hwnd);
    alia_ui_surface_set_dpi(ui, float(dpi ? dpi : 96));

    // Prefer the active drawable size (client), which may be smaller than the
    // grow-only backbuffer during live resize.
    if (host->width > 0 && host->height > 0)
    {
        alia_ui_surface_set_size(ui, {int(host->width), int(host->height)});
        return;
    }

    RECT client{};
    GetClientRect(host->hwnd, &client);
    alia_ui_surface_set_size(
        ui,
        {int(client.right - client.left), int(client.bottom - client.top)});
}

ID3D11Device*
alia_win32_host_device(alia_win32_host* host)
{
    ALIA_ASSERT(host);
    return host->device;
}

ID3D11DeviceContext*
alia_win32_host_context(alia_win32_host* host)
{
    ALIA_ASSERT(host);
    return host->context;
}

ID3D11RenderTargetView*
alia_win32_host_rtv(alia_win32_host* host)
{
    ALIA_ASSERT(host);
    return host->rtv;
}

UINT
alia_win32_host_width(alia_win32_host* host)
{
    ALIA_ASSERT(host);
    return host->width;
}

UINT
alia_win32_host_height(alia_win32_host* host)
{
    ALIA_ASSERT(host);
    return host->height;
}

} // extern "C"
