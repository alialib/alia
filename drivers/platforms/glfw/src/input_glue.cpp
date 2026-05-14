#include <alia/platforms/glfw/input_glue.h>

#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/system/input_processing.h>

#include <GLFW/glfw3.h>

extern "C" {

alia_kmods_t
alia_glfw_mods_to_kmods(int glfw_mods)
{
    alia_kmods_t result = 0;
    if (glfw_mods & GLFW_MOD_SHIFT)
        result |= ALIA_KMOD_SHIFT;
    if (glfw_mods & GLFW_MOD_CONTROL)
        result |= ALIA_KMOD_CTRL;
    if (glfw_mods & GLFW_MOD_ALT)
        result |= ALIA_KMOD_ALT;
    if (glfw_mods & GLFW_MOD_SUPER)
        result |= ALIA_KMOD_WIN;
    return result;
}

alia_vec2f
alia_glfw_cursor_window_to_framebuffer(GLFWwindow* window, double x, double y)
{
    int framebuffer_width = 0;
    int framebuffer_height = 0;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    int window_width = 0;
    int window_height = 0;
    glfwGetWindowSize(window, &window_width, &window_height);

    if (window_width <= 0 || window_height <= 0)
        return ALIA_BRACED_INIT(alia_vec2f, 0.f, 0.f);

    float const internal_x = static_cast<float>(x) * framebuffer_width
                           / static_cast<float>(window_width);
    float const internal_y = static_cast<float>(y) * framebuffer_height
                           / static_cast<float>(window_height);
    return ALIA_BRACED_INIT(alia_vec2f, internal_x, internal_y);
}

void
alia_glfw_enqueue_mouse_button(
    alia_ui_system* ui,
    GLFWwindow* window,
    int button,
    int action,
    int mods)
{
    if (!ui || !window)
        return;

    double x = 0;
    double y = 0;
    glfwGetCursorPos(window, &x, &y);
    alia_vec2f const p = alia_glfw_cursor_window_to_framebuffer(window, x, y);
    alia_kmods_t const km = alia_glfw_mods_to_kmods(mods);

    switch (action)
    {
        case GLFW_PRESS:
            alia_ui_enqueue_mouse_press(ui, p, button, km);
            break;
        case GLFW_RELEASE:
            alia_ui_enqueue_mouse_release(ui, p, button, km);
            break;
        default:
            break;
    }
}

void
alia_glfw_enqueue_cursor_pos(
    alia_ui_system* ui, GLFWwindow* window, double x, double y)
{
    if (!ui || !window)
        return;
    alia_vec2f const p = alia_glfw_cursor_window_to_framebuffer(window, x, y);
    alia_ui_enqueue_mouse_motion(ui, p);
}

void
alia_glfw_enqueue_scroll(alia_ui_system* ui, double x, double y)
{
    if (!ui)
        return;
    alia_vec2f const delta
        = ALIA_BRACED_INIT(alia_vec2f, static_cast<float>(x), static_cast<float>(y));
    alia_ui_enqueue_scroll(ui, delta);
}

void
alia_glfw_enqueue_key(alia_ui_system* ui, int key, int action, int mods)
{
    if (!ui)
        return;
    alia_kmods_t const km = alia_glfw_mods_to_kmods(mods);
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
        alia_ui_enqueue_key_press(ui, alia_key_code_t(key), km);
    else if (action == GLFW_RELEASE)
        alia_ui_enqueue_key_release(ui, alia_key_code_t(key), km);
}

static void
mouse_button_callback(
    GLFWwindow* window, int button, int action, int mods)
{
    auto* binding = static_cast<alia_glfw_ui_binding*>(
        glfwGetWindowUserPointer(window));
    if (!binding || !binding->ui)
        return;
    alia_glfw_enqueue_mouse_button(binding->ui, window, button, action, mods);
}

static void
cursor_position_callback(GLFWwindow* window, double x, double y)
{
    auto* binding = static_cast<alia_glfw_ui_binding*>(
        glfwGetWindowUserPointer(window));
    if (!binding || !binding->ui)
        return;
    alia_glfw_enqueue_cursor_pos(binding->ui, window, x, y);
}

static void
scroll_callback(GLFWwindow* window, double x, double y)
{
    auto* binding = static_cast<alia_glfw_ui_binding*>(
        glfwGetWindowUserPointer(window));
    if (!binding || !binding->ui)
        return;
    alia_glfw_enqueue_scroll(binding->ui, x, y);
}

static void
key_callback(
    GLFWwindow* window, int key, int /*scancode*/, int action, int mods)
{
    auto* binding = static_cast<alia_glfw_ui_binding*>(
        glfwGetWindowUserPointer(window));
    if (!binding || !binding->ui)
        return;
    alia_glfw_enqueue_key(binding->ui, key, action, mods);
}

static void
framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    auto* binding = static_cast<alia_glfw_ui_binding*>(
        glfwGetWindowUserPointer(window));
    if (!binding || !binding->ui)
        return;
    alia_vec2i const size = ALIA_BRACED_INIT(alia_vec2i, width, height);
    alia_ui_surface_set_size(binding->ui, size);
}

static void
content_scale_callback(GLFWwindow* window, float xscale, float yscale)
{
    auto* binding = static_cast<alia_glfw_ui_binding*>(
        glfwGetWindowUserPointer(window));
    if (!binding || !binding->ui)
        return;
    (void) window;
    alia_ui_surface_set_dpi(
        binding->ui, ((xscale + yscale) / 2.0f) * 96.f);
}

void
alia_glfw_install_default_input_callbacks(
    GLFWwindow* window, alia_glfw_ui_binding* binding)
{
    if (!window || !binding || !binding->ui)
        return;
    glfwSetWindowUserPointer(window, binding);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
}

void
alia_glfw_install_surface_callbacks(
    GLFWwindow* window, alia_glfw_ui_binding* binding)
{
    if (!window || !binding || !binding->ui)
        return;
    glfwSetWindowUserPointer(window, binding);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetWindowContentScaleCallback(window, content_scale_callback);
}

} // extern "C"
