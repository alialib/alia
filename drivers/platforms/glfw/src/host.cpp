#include <alia/platforms/glfw/host.h>

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
#include <windows.h>
#endif

#include <chrono>
#include <new>

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
    // Nesting depth of host-driven frame callbacks (run/present) - Used to
    // suppress frame callbacks when a frame is already in progress.
    int frame_depth = 0;
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

#ifdef _WIN32
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

    glfwMakeContextCurrent(host->window);
    host->config.frame.fn(host->config.frame.user_data);
    glfwSwapBuffers(host->window);
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
    bool const should_tick = host_should_tick(host);
    if (!should_tick)
        return;

    ALIA_ASSERT(host->config.frame.fn);
    ALIA_ASSERT(host->window);

    ++host->frame_depth;
    glfwMakeContextCurrent(host->window);
    host->config.frame.fn(host->config.frame.user_data);
    glfwSwapBuffers(host->window);
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
    if (options)
        resolved_options = *options;

    glfwWindowHint(
        GLFW_RESIZABLE, resolved_options.resizable ? GLFW_TRUE : GLFW_FALSE);

    GLFWwindow* window
        = glfwCreateWindow(state.width, state.height, title, nullptr, nullptr);
    if (!window)
        return false;

    if (state.has_position)
        glfwSetWindowPos(window, state.x, state.y);

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        glfwDestroyWindow(window);
        return false;
    }

    glEnable(GL_FRAMEBUFFER_SRGB);
    glfwSwapInterval(resolved_options.vsync ? 1 : 0);

    host->window = window;
    host->window_state = state;
    host->window_state_seeded = true;

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

    GLFWwindow* const window = host->window;
    while (!glfwWindowShouldClose(window))
    {
        host_wait_for_events(host);
        if (glfwWindowShouldClose(window))
            break;
        host_run_frame(host);
    }
}

} // extern "C"
