#include <alia/platforms/glfw/host.h>

#if defined(ALIA_ENABLE_VK_PRESENT)
#include <alia/platforms/glfw/vk_present.h>
#endif

#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/host_window.h>
#include <alia/abi/ui/system/work.h>
#include <alia/platforms/glfw/input_glue.h>
#include <alia/platforms/glfw/ui_binding.h>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dwmapi.h>
#include <mmsystem.h>
#include <windows.h>
#endif

#include <chrono>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <new>
#include <vector>

struct alia_glfw_host
{
    alia_glfw_host_config config{};
    alia_glfw_ui_binding binding{};
    GLFWwindow* window = nullptr;
    alia_window_state window_state{};
    bool window_state_seeded = false;
    bool installed = false;
    // true while entering or leaving fullscreen - Used to suppress window rect
    // callbacks that would overwrite the saved windowed rect.
    bool mode_transition = false;
    bool vsync = true;
    // When true, the continuous run loop sleeps to match the monitor refresh.
    bool pace_to_monitor = false;
    alia_nanosecond_count pacing_interval_ns = 16'666'667;
    bool vulkan_present = false;
#if defined(ALIA_ENABLE_VK_PRESENT)
    alia_glfw_vk_present* vk_present = nullptr;
    alia_glfw_vk_gl_target vk_gl_target{};
    struct host_wgl_context
    {
        GLFWwindow* gl_window = nullptr;
    } wgl{};
    bool owns_wgl_context = false;
#endif
    // Nesting depth of host-driven frame callbacks (run/present) - Used to
    // suppress frame callbacks when a frame is already in progress.
    int frame_depth = 0;
    // Main window is created hidden and shown after the first present.
    bool window_shown = false;
#ifdef _WIN32
    WNDPROC win32_original_wndproc = nullptr;
    bool win32_in_modal_interaction = false;
#endif
};

namespace {

bool g_platform_initialized = false;

alia_nanosecond_count
steady_clock_now_ns()
{
    auto const duration = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration)
        .count();
}

alia_glfw_host*
host_from_window(GLFWwindow* window)
{
    auto* binding
        = static_cast<alia_glfw_ui_binding*>(glfwGetWindowUserPointer(window));
    return binding ? binding->host : nullptr;
}

void
host_notify_window_state(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    if (!host->config.on_window_state_changed.fn)
        return;
    host->config.on_window_state_changed.fn(
        host->config.on_window_state_changed.user_data, &host->window_state);
}

void
host_sync_rect_from_glfw(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(host->window);

    GLFWwindow* const window = host->window;
    glfwGetWindowPos(window, &host->window_state.x, &host->window_state.y);
    glfwGetWindowSize(
        window, &host->window_state.width, &host->window_state.height);
    host->window_state.has_position = true;
}

void
host_sync_maximized_from_glfw(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(host->window);
    host->window_state.maximized
        = glfwGetWindowAttrib(host->window, GLFW_MAXIMIZED) == GLFW_TRUE;
}

GLFWmonitor*
glfw_monitor_covering_most_of_window(GLFWwindow* window)
{
    int wx, wy, ww, wh;
    glfwGetWindowPos(window, &wx, &wy);
    glfwGetWindowSize(window, &ww, &wh);

    int n = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&n);
    GLFWmonitor* best = glfwGetPrimaryMonitor();
    long long best_area = -1;

    for (int i = 0; i < n; ++i)
    {
        GLFWmonitor* m = monitors[i];
        int mx, my;
        glfwGetMonitorPos(m, &mx, &my);
        GLFWvidmode const* mode = glfwGetVideoMode(m);
        if (!mode)
            continue;
        int const mw = mode->width;
        int const mh = mode->height;

        int const ix1 = (wx > mx) ? wx : mx;
        int const iy1 = (wy > my) ? wy : my;
        int const ix2 = (wx + ww < mx + mw) ? wx + ww : mx + mw;
        int const iy2 = (wy + wh < my + mh) ? wy + wh : my + mh;
        int const iw = ix2 - ix1;
        int const ih = iy2 - iy1;
        if (iw <= 0 || ih <= 0)
            continue;
        long long const area
            = static_cast<long long>(iw) * static_cast<long long>(ih);
        if (area > best_area)
        {
            best_area = area;
            best = m;
        }
    }
    return best;
}

void
host_update_pacing_interval(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    if (!host->window)
        return;

    int hz = 60;
    GLFWmonitor* const monitor
        = glfw_monitor_covering_most_of_window(host->window);
    if (monitor)
    {
        GLFWvidmode const* const mode = glfwGetVideoMode(monitor);
        if (mode && mode->refreshRate > 0)
            hz = mode->refreshRate;
    }
    host->pacing_interval_ns = 1'000'000'000LL / hz;
}

#ifdef _WIN32
static int host_high_res_timer_refcount = 0;

static void
host_acquire_high_res_timer()
{
    if (host_high_res_timer_refcount++ == 0)
        timeBeginPeriod(1);
}

static void
host_release_high_res_timer()
{
    if (host_high_res_timer_refcount > 0 && --host_high_res_timer_refcount == 0)
        timeEndPeriod(1);
}
#else
static void
host_acquire_high_res_timer()
{
}

static void
host_release_high_res_timer()
{
}
#endif

static alia_nanosecond_count const host_pace_spin_threshold_ns = 2'000'000;

static void
host_make_context_current_if_needed(alia_glfw_host* host)
{
    if (!host || !host->window)
        return;
#if defined(ALIA_ENABLE_VK_PRESENT)
    if (host->owns_wgl_context)
    {
        if (glfwGetCurrentContext() != host->wgl.gl_window)
            glfwMakeContextCurrent(host->wgl.gl_window);
        return;
    }
#endif
    if (glfwGetCurrentContext() != host->window)
        glfwMakeContextCurrent(host->window);
}

#if defined(ALIA_ENABLE_VK_PRESENT)
static bool
host_wgl_context_create(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    host->wgl = {};

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    host->wgl.gl_window = glfwCreateWindow(1, 1, "", nullptr, nullptr);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    if (!host->wgl.gl_window)
        return false;

    glfwMakeContextCurrent(host->wgl.gl_window);
    host->owns_wgl_context = true;
    return true;
}

static void
host_wgl_context_destroy(alia_glfw_host* host)
{
    if (!host || !host->owns_wgl_context)
        return;

    if (host->wgl.gl_window)
    {
        if (glfwGetCurrentContext() == host->wgl.gl_window)
            glfwMakeContextCurrent(nullptr);
        glfwDestroyWindow(host->wgl.gl_window);
    }

    host->wgl = {};
    host->owns_wgl_context = false;
}
#endif

static void
host_show_window_if_needed(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    if (host->window_shown || !host->window)
        return;
    glfwShowWindow(host->window);
    host->window_shown = true;
}

#if defined(ALIA_ENABLE_VK_PRESENT)
static bool
host_vk_sync_surfaces(alia_glfw_host* host)
{
    if (!host || !host->vulkan_present || !host->vk_present || !host->window)
        return true;

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(host->window, &width, &height);
    return alia_glfw_vk_present_get_gl_target(
        host->vk_present, width, height, &host->vk_gl_target);
}

static void
host_swap_or_present(alia_glfw_host* host)
{
    if (!host->vulkan_present || !host->vk_present)
    {
        glfwSwapBuffers(host->window);
        host_show_window_if_needed(host);
        return;
    }

    if (!alia_glfw_vk_present_from_gl_target(host->vk_present))
    {
        std::fprintf(
            stderr, "[alia host] Vulkan present failed; using GL swap\n");
        glfwSwapBuffers(host->window);
    }
    host_show_window_if_needed(host);
}

static void
host_begin_gl_frame_target(alia_glfw_host* host)
{
    if (!host->vulkan_present)
        return;
    if (!host_vk_sync_surfaces(host))
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, host->vk_gl_target.framebuffer);
    glViewport(0, 0, host->vk_gl_target.width, host->vk_gl_target.height);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void
host_end_gl_frame_target(alia_glfw_host* host)
{
    if (!host->vulkan_present)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
#else
static void
host_begin_gl_frame_target(alia_glfw_host* /*host*/)
{
}

static void
host_end_gl_frame_target(alia_glfw_host* /*host*/)
{
}

static void
host_swap_or_present(alia_glfw_host* host)
{
    glfwSwapBuffers(host->window);
    host_show_window_if_needed(host);
}
#endif

static void
host_wait_until_ns(alia_nanosecond_count target_ns)
{
    for (;;)
    {
        alia_nanosecond_count const now = steady_clock_now_ns();
        if (now >= target_ns)
            return;

        alia_nanosecond_count const remaining = target_ns - now;
        if (remaining > host_pace_spin_threshold_ns)
        {
            double const wait_s
                = static_cast<double>(remaining - host_pace_spin_threshold_ns)
                / 1e9;
            glfwWaitEventsTimeout(wait_s);
        }
        else
        {
            while (steady_clock_now_ns() < target_ns)
                glfwPollEvents();
        }
    }
}

void
host_pace_frame(alia_glfw_host* host, alia_nanosecond_count frame_start_ns)
{
    if (!host->pace_to_monitor || !host->config.continuous)
        return;

    if (host->pacing_interval_ns == 0)
        return;

    alia_nanosecond_count const target_ns
        = frame_start_ns + host->pacing_interval_ns;
    if (steady_clock_now_ns() >= target_ns)
        return;

    host_wait_until_ns(target_ns);
}

#ifdef _WIN32
static UINT_PTR const win32_modal_timer_id = 1;
// Wake promptly after a modal frame finishes; render cost is the real limiter.
static UINT const win32_modal_timer_ms = 1;

static void
host_present_frame(alia_glfw_host* host);

static void
host_defer_redraw(alia_glfw_host* host);

static void
host_win32_after_modal_swap(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(host->window);
    HWND const hwnd = glfwGetWin32Window(host->window);
    if (hwnd)
        InvalidateRect(hwnd, nullptr, FALSE);
    DwmFlush();
}

static void
host_begin_modal_interaction(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    host->win32_in_modal_interaction = true;
    host_acquire_high_res_timer();
#if defined(ALIA_ENABLE_VK_PRESENT)
    if (host->vulkan_present && host->vk_present)
        alia_glfw_vk_present_set_modal_interaction(host->vk_present, true);
    else
#endif
    if (host->vsync)
        glfwSwapInterval(0);
}

static void
host_end_modal_interaction(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    host->win32_in_modal_interaction = false;
#if defined(ALIA_ENABLE_VK_PRESENT)
    if (host->vulkan_present && host->vk_present)
        alia_glfw_vk_present_set_modal_interaction(host->vk_present, false);
    else
#endif
    if (host->vsync)
        glfwSwapInterval(1);
    host_release_high_res_timer();
}

static void
host_present_continuous(alia_glfw_host* host)
{
    if (!host || !host->config.frame.fn || !host->config.continuous)
        return;

    if (host->frame_depth > 0)
    {
        host_defer_redraw(host);
        return;
    }

    host_present_frame(host);
}

static void
host_mark_modal_surface_dirty(alia_glfw_host* host)
{
    if (!host || !host->config.ui)
        return;
    alia_glfw_host_sync_surface(host, host->config.ui);
    alia_ui_mark_dirty(host->config.ui);
}

static LRESULT CALLBACK
host_win32_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    alia_glfw_host* host
        = reinterpret_cast<alia_glfw_host*>(GetPropW(hwnd, L"AliaGlfwHost"));

    if (host && host->config.continuous)
    {
        switch (msg)
        {
            case WM_ENTERSIZEMOVE:
                host_begin_modal_interaction(host);
                SetTimer(
                    hwnd, win32_modal_timer_id, win32_modal_timer_ms, nullptr);
                host_present_continuous(host);
                break;
            case WM_EXITSIZEMOVE:
                KillTimer(hwnd, win32_modal_timer_id);
                host_present_continuous(host);
                host_end_modal_interaction(host);
                break;
            case WM_MOVING:
                host_present_continuous(host);
                break;
            case WM_TIMER:
                if (wParam == win32_modal_timer_id)
                {
                    host_present_continuous(host);
                    return 0;
                }
                break;
            default:
                break;
        }
    }

    WNDPROC const original = host ? host->win32_original_wndproc : nullptr;
    if (!original)
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    return CallWindowProcW(original, hwnd, msg, wParam, lParam);
}

static void
host_install_win32_modal_present(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(host->window);
    if (!host->config.continuous || host->win32_original_wndproc)
        return;

    HWND const hwnd = glfwGetWin32Window(host->window);
    if (!hwnd)
        return;

    SetPropW(hwnd, L"AliaGlfwHost", host);
    host->win32_original_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
        hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(host_win32_wndproc)));
}

static void
host_uninstall_win32_modal_present(alia_glfw_host* host)
{
    if (!host || !host->window || !host->win32_original_wndproc)
        return;

    HWND const hwnd = glfwGetWin32Window(host->window);
    if (hwnd)
    {
        KillTimer(hwnd, win32_modal_timer_id);
        if (host->win32_in_modal_interaction)
            host_end_modal_interaction(host);
        SetWindowLongPtrW(
            hwnd,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(host->win32_original_wndproc));
        RemovePropW(hwnd, L"AliaGlfwHost");
    }
    host->win32_original_wndproc = nullptr;
}

static void
glfw_request_win32_frame_refresh(GLFWwindow* window)
{
    HWND const hwnd = glfwGetWin32Window(window);
    if (!hwnd)
        return;
    SetWindowPos(
        hwnd,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}
#endif

void
host_set_fullscreen(alia_glfw_host* host, bool fullscreen)
{
    ALIA_ASSERT(host);
    GLFWwindow* const window = host->window;
    if (!window || fullscreen == host->window_state.fullscreen)
        return;

    host->mode_transition = true;

    if (!fullscreen)
    {
        glfwSetWindowMonitor(
            window,
            nullptr,
            host->window_state.x,
            host->window_state.y,
            host->window_state.width,
            host->window_state.height,
            GLFW_DONT_CARE);
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
        glfwPollEvents();
#ifdef _WIN32
        glfw_request_win32_frame_refresh(window);
#endif
        host->window_state.fullscreen = false;
    }
    else
    {
        if (!host->window_state.maximized)
            host_sync_rect_from_glfw(host);

        GLFWmonitor* monitor = glfw_monitor_covering_most_of_window(window);
        GLFWvidmode const* mode = glfwGetVideoMode(monitor);
        if (!mode)
        {
            monitor = glfwGetPrimaryMonitor();
            mode = glfwGetVideoMode(monitor);
        }
        if (!mode)
        {
            host->mode_transition = false;
            return;
        }
        int mx, my;
        glfwGetMonitorPos(monitor, &mx, &my);
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(
            window,
            nullptr,
            mx,
            my,
            mode->width,
            mode->height,
            GLFW_DONT_CARE);
        host->window_state.fullscreen = true;
    }

    host_notify_window_state(host);

    host->mode_transition = false;
    host_update_pacing_interval(host);
}

void
host_toggle_fullscreen(void* user)
{
    auto* host = static_cast<alia_glfw_host*>(user);
    if (!host || !host->window)
        return;

    host_set_fullscreen(host, !host->window_state.fullscreen);
}

bool
host_tracks_window_rect(alia_glfw_host* host)
{
    return !host->window_state.fullscreen && !host->window_state.maximized
        && !host->mode_transition;
}

void
window_pos_callback(GLFWwindow* window, int x, int y)
{
    alia_glfw_host* host = host_from_window(window);
    if (!host || !host_tracks_window_rect(host))
        return;

    host->window_state.x = x;
    host->window_state.y = y;
    host->window_state.has_position = true;
    host_notify_window_state(host);
    host_update_pacing_interval(host);
}

void
window_size_callback(GLFWwindow* window, int width, int height)
{
    alia_glfw_host* host = host_from_window(window);
    if (!host || !host_tracks_window_rect(host))
        return;

    host->window_state.width = width;
    host->window_state.height = height;
    host_notify_window_state(host);
}

void
window_maximize_callback(GLFWwindow* window, int maximized)
{
    alia_glfw_host* host = host_from_window(window);
    if (!host)
        return;

    host->window_state.maximized = maximized == GLFW_TRUE;
    if (!maximized && host_tracks_window_rect(host))
        host_sync_rect_from_glfw(host);
    host_notify_window_state(host);
}

void
window_close_callback(GLFWwindow* window)
{
    alia_glfw_host* host = host_from_window(window);
    if (!host)
        return;
    host_notify_window_state(host);
}

void
host_present_frame(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(host->config.ui);
    ALIA_ASSERT(host->config.frame.fn);
    ALIA_ASSERT(host->window);

    ++host->frame_depth;
    alia_glfw_host_sync_surface(host, host->config.ui);
    alia_ui_mark_dirty(host->config.ui);
    host_make_context_current_if_needed(host);
    host_begin_gl_frame_target(host);
    host->config.frame.fn(host->config.frame.user_data);
    host_end_gl_frame_target(host);
    host_swap_or_present(host);
#ifdef _WIN32
    if (host->win32_in_modal_interaction)
        host_win32_after_modal_swap(host);
#endif
    --host->frame_depth;
}

void
host_defer_redraw(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(host->config.ui);
    alia_glfw_host_sync_surface(host, host->config.ui);
    alia_ui_mark_dirty(host->config.ui);
}

void
window_refresh_callback(GLFWwindow* window)
{
    alia_glfw_host* host = host_from_window(window);
    if (!host || !host->config.frame.fn)
        return;

    if (host->frame_depth > 0)
    {
        host_defer_redraw(host);
        return;
    }

#ifdef _WIN32
    if (host->win32_in_modal_interaction && host->config.continuous)
    {
        host_present_continuous(host);
        return;
    }
#endif

    host_present_frame(host);
}

void
host_install_window_state_callbacks(GLFWwindow* window)
{
    glfwSetWindowPosCallback(window, window_pos_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetWindowMaximizeCallback(window, window_maximize_callback);
    glfwSetWindowCloseCallback(window, window_close_callback);
    glfwSetWindowRefreshCallback(window, window_refresh_callback);
}

bool
host_should_tick(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(host->config.ui);
    if (host->config.continuous)
        return true;
    if (alia_ui_needs_tick(host->config.ui))
        return true;
    alia_nanosecond_count wake_ns = 0;
    return alia_ui_next_wake_ns(host->config.ui, &wake_ns);
}

double
host_wait_timeout_seconds(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(host->config.ui);

    if (host->config.continuous)
        return 0.0;

    alia_ui_system* ui = host->config.ui;
    if (alia_ui_needs_tick(ui))
        return 0.0;

    alia_nanosecond_count wake_ns = 0;
    if (!alia_ui_next_wake_ns(ui, &wake_ns))
        return -1.0;

    alia_ui_system_poll_clock(ui);
    alia_nanosecond_count const now = steady_clock_now_ns();
    if (wake_ns <= now)
        return 0.0;

    alia_nanosecond_count const delta_ns = wake_ns - now;
    return static_cast<double>(delta_ns) / 1e9;
}

void
host_wait_for_events(alia_glfw_host* host)
{
    double const timeout = host_wait_timeout_seconds(host);
    if (timeout < 0.0)
        glfwWaitEvents();
    else if (timeout == 0.0)
        glfwPollEvents();
    else
        glfwWaitEventsTimeout(timeout);
}

void
host_run_frame(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    if (!host_should_tick(host))
        return;

    ALIA_ASSERT(host->config.frame.fn);
    ALIA_ASSERT(host->window);

    ++host->frame_depth;
    host_make_context_current_if_needed(host);
    host_begin_gl_frame_target(host);
    host->config.frame.fn(host->config.frame.user_data);
    host_end_gl_frame_target(host);
    host_swap_or_present(host);
    --host->frame_depth;
}

void
host_wire_window_ops(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(host->config.ui);

    alia_host_window_ops const ops = {
        .toggle_fullscreen = host_toggle_fullscreen,
        .user = host,
    };
    alia_ui_system_set_host_window_ops(host->config.ui, &ops);
}

void
host_init_window_state_if_needed(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    if (host->window_state_seeded)
        return;

    host_sync_rect_from_glfw(host);
    host_sync_maximized_from_glfw(host);
    host->window_state.fullscreen = false;
    host->window_state_seeded = true;
}

} // namespace

void
alia_glfw_host_on_framebuffer_resized(GLFWwindow* window)
{
    alia_glfw_host* host = host_from_window(window);
    if (!host || !host->config.continuous)
        return;

#ifdef _WIN32
    if (host->win32_in_modal_interaction)
        host_mark_modal_surface_dirty(host);
#endif
}

extern "C" {

bool
alia_glfw_platform_init(void)
{
    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    g_platform_initialized = true;
    return true;
}

void
alia_glfw_platform_shutdown(void)
{
    if (!g_platform_initialized)
        return;
    glfwTerminate();
    g_platform_initialized = false;
}

alia_glfw_host*
alia_glfw_host_create(void)
{
    auto* host = new (std::nothrow) alia_glfw_host{};
    ALIA_ASSERT(host);
    return host;
}

void
alia_glfw_host_destroy(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
#ifdef _WIN32
    host_uninstall_win32_modal_present(host);
#endif
#if defined(ALIA_ENABLE_VK_PRESENT)
    host_wgl_context_destroy(host);
    alia_glfw_vk_present_destroy(host->vk_present);
    host->vk_present = nullptr;
#endif
    delete host;
}

bool
alia_glfw_host_open(
    alia_glfw_host* host,
    char const* title,
    alia_window_state state,
    alia_glfw_window_options const* options)
{
    ALIA_ASSERT(host);

    alia_glfw_window_options resolved_options{};
    resolved_options.resizable = true;
    resolved_options.vsync = true;
    resolved_options.vulkan_present = false;
    if (options)
        resolved_options = *options;
#if !defined(ALIA_ENABLE_VK_PRESENT)
    resolved_options.vulkan_present = false;
#endif

    glfwWindowHint(
        GLFW_RESIZABLE, resolved_options.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#if defined(ALIA_ENABLE_VK_PRESENT)
    if (resolved_options.vulkan_present)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

    GLFWwindow* window
        = glfwCreateWindow(state.width, state.height, title, nullptr, nullptr);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
#if defined(ALIA_ENABLE_VK_PRESENT)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
#endif
    if (!window)
        return false;

    if (state.has_position)
        glfwSetWindowPos(window, state.x, state.y);

#if defined(ALIA_ENABLE_VK_PRESENT)
    alia_glfw_vk_present* vk_present = nullptr;
    if (resolved_options.vulkan_present)
    {
        if (!alia_glfw_vk_present_begin(
                &vk_present, window, resolved_options.vsync))
        {
            std::fprintf(
                stderr,
                "[alia host] Vulkan present init failed; using glfwSwapBuffers\n");
        }
    }

    if (resolved_options.vulkan_present)
    {
        if (!host_wgl_context_create(host))
        {
            alia_glfw_vk_present_destroy(vk_present);
            glfwDestroyWindow(window);
            return false;
        }
        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
        {
            host_wgl_context_destroy(host);
            alia_glfw_vk_present_destroy(vk_present);
            glfwDestroyWindow(window);
            return false;
        }
    }
    else
#endif
    {
        glfwMakeContextCurrent(window);
        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
        {
#if defined(ALIA_ENABLE_VK_PRESENT)
            alia_glfw_vk_present_destroy(vk_present);
#endif
            glfwDestroyWindow(window);
            return false;
        }
    }

    glEnable(GL_FRAMEBUFFER_SRGB);
    glfwSwapInterval(resolved_options.vsync ? 1 : 0);

    host->window = window;
    host->window_state = state;
    host->window_state_seeded = true;
    host->vsync = resolved_options.vsync;
    host->pace_to_monitor = !resolved_options.vsync;
    host_update_pacing_interval(host);

#if defined(ALIA_ENABLE_VK_PRESENT)
    if (vk_present)
    {
        if (!alia_glfw_vk_present_complete(vk_present))
        {
            std::fprintf(
                stderr,
                "[alia host] Vulkan present init failed; using glfwSwapBuffers\n");
            alia_glfw_vk_present_destroy(vk_present);
        }
        else
        {
            host->vulkan_present = true;
            host->vk_present = vk_present;
        }
    }
#endif

    bool const restore_fullscreen = state.fullscreen;
    host->window_state.fullscreen = false;

    if (restore_fullscreen)
        host_set_fullscreen(host, true);
    else if (state.maximized)
        glfwMaximizeWindow(window);

    host_sync_maximized_from_glfw(host);
    return true;
}

void
alia_glfw_host_attach_window(alia_glfw_host* host, GLFWwindow* window)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(window);
    host->window = window;
    host->window_state_seeded = false;
}

void
alia_glfw_host_sync_surface(alia_glfw_host* host, alia_ui_system* ui)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(ui);
    ALIA_ASSERT(host->window);

    GLFWwindow* const window = host->window;
    float xscale = 1.f;
    float yscale = 1.f;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    alia_ui_surface_set_dpi(ui, ((xscale + yscale) / 2.f) * 96.f);

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    alia_ui_surface_set_size(ui, {width, height});
#ifdef _WIN32
    host_vk_sync_surfaces(host);
#endif
}

void
alia_glfw_host_install(
    alia_glfw_host* host, alia_glfw_host_config const* config)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(config);
    ALIA_ASSERT(config->ui);
    ALIA_ASSERT(host->window);

    host->config = *config;
    host->binding.ui = config->ui;
    host->binding.host = host;

    if (host->installed)
        return;

    host_init_window_state_if_needed(host);

    alia_glfw_install_surface_callbacks(host->window, &host->binding);
    alia_glfw_install_default_input_callbacks(host->window, &host->binding);
    host_install_window_state_callbacks(host->window);
    host_wire_window_ops(host);
#ifdef _WIN32
    host_install_win32_modal_present(host);
#endif
    host->installed = true;
}

void
alia_glfw_host_run(alia_glfw_host* host, alia_glfw_host_config const* config)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(config);
    ALIA_ASSERT(config->ui);
    ALIA_ASSERT(host->window);
    ALIA_ASSERT(config->frame.fn);

    alia_glfw_host_install(host, config);

    bool const use_pace_timer
        = host->pace_to_monitor && host->config.continuous;
    if (use_pace_timer)
        host_acquire_high_res_timer();

    GLFWwindow* const window = host->window;
    while (!glfwWindowShouldClose(window))
    {
        alia_nanosecond_count const frame_start = steady_clock_now_ns();

        host_wait_for_events(host);
        if (glfwWindowShouldClose(window))
            break;

        host_run_frame(host);
        host_pace_frame(host, frame_start);
    }

    if (use_pace_timer)
        host_release_high_res_timer();
}

void
alia_glfw_host_toggle_fullscreen(alia_glfw_host* host)
{
    ALIA_ASSERT(host);
    host_toggle_fullscreen(host);
}

} // extern "C"
