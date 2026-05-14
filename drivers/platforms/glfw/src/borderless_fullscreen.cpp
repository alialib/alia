#include <alia/platforms/glfw/borderless_fullscreen.h>

#include <GLFW/glfw3.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#endif

// For windowed windows, glfwGetWindowMonitor is NULL; pick the monitor whose
// virtual-area rectangle overlaps the window the most (straddling uses the
// majority monitor).
static GLFWmonitor*
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
// After toggling GLFW_DECORATED, DWM sometimes skips repainting the caption
// until the next pointer message; SWP_FRAMECHANGED forces a non-client
// refresh.
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

extern "C" {

void
alia_glfw_fullscreen_host_toggle(void* user)
{
    auto* binding = static_cast<alia_glfw_fullscreen_host_binding*>(user);
    if (!binding || !binding->window || !binding->state)
        return;

    GLFWwindow* const window = binding->window;
    alia_glfw_borderless_fullscreen_state& impl = *binding->state;

    bool const is_fullscreen = impl.is_fullscreen;
    if (is_fullscreen)
    {
        glfwSetWindowMonitor(
            window,
            nullptr,
            impl.windowed_position.x,
            impl.windowed_position.y,
            impl.windowed_size.x,
            impl.windowed_size.y,
            GLFW_DONT_CARE);
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
        glfwPollEvents();
#ifdef _WIN32
        glfw_request_win32_frame_refresh(window);
#endif
    }
    else
    {
        GLFWmonitor* monitor = glfw_monitor_covering_most_of_window(window);
        GLFWvidmode const* mode = glfwGetVideoMode(monitor);
        if (!mode)
        {
            monitor = glfwGetPrimaryMonitor();
            mode = glfwGetVideoMode(monitor);
        }
        if (!mode)
            return;
        glfwGetWindowPos(
            window, &impl.windowed_position.x, &impl.windowed_position.y);
        glfwGetWindowSize(
            window, &impl.windowed_size.x, &impl.windowed_size.y);
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
            mode->refreshRate);
    }
    impl.is_fullscreen = !is_fullscreen;
}

} // extern "C"
