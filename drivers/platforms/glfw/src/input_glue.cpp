#include <alia/platforms/glfw/input_glue.h>

#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/scroll.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/system/input_processing.h>
#include <alia/platforms/glfw/host.h>

#include <GLFW/glfw3.h>

void
alia_glfw_host_on_framebuffer_resized(GLFWwindow* window);

static alia_hid_key_t
glfw_key_to_hid(int key)
{
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
        return (alia_hid_key_t) (ALIA_HID_A + (key - GLFW_KEY_A));
    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9)
        return (alia_hid_key_t) (ALIA_HID_DIGIT_1 + (key - GLFW_KEY_1));
    if (key == GLFW_KEY_0)
        return ALIA_HID_DIGIT_0;
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12)
        return (alia_hid_key_t) (ALIA_HID_F1 + (key - GLFW_KEY_F1));

    switch (key)
    {
        case GLFW_KEY_SPACE:
            return ALIA_HID_SPACE;
        case GLFW_KEY_APOSTROPHE:
            return ALIA_HID_APOSTROPHE;
        case GLFW_KEY_COMMA:
            return ALIA_HID_COMMA;
        case GLFW_KEY_MINUS:
            return ALIA_HID_MINUS;
        case GLFW_KEY_PERIOD:
            return ALIA_HID_PERIOD;
        case GLFW_KEY_SLASH:
            return ALIA_HID_SLASH;
        case GLFW_KEY_SEMICOLON:
            return ALIA_HID_SEMICOLON;
        case GLFW_KEY_EQUAL:
            return ALIA_HID_EQUAL;
        case GLFW_KEY_LEFT_BRACKET:
            return ALIA_HID_LEFT_BRACKET;
        case GLFW_KEY_RIGHT_BRACKET:
            return ALIA_HID_RIGHT_BRACKET;
        case GLFW_KEY_BACKSLASH:
            return ALIA_HID_BACKSLASH;
        case GLFW_KEY_GRAVE_ACCENT:
            return ALIA_HID_GRAVE_ACCENT;
        case GLFW_KEY_WORLD_1:
            return ALIA_HID_INTL_BACKSLASH;
        case GLFW_KEY_ESCAPE:
            return ALIA_HID_ESCAPE;
        case GLFW_KEY_ENTER:
            return ALIA_HID_ENTER;
        case GLFW_KEY_TAB:
            return ALIA_HID_TAB;
        case GLFW_KEY_BACKSPACE:
            return ALIA_HID_BACKSPACE;
        case GLFW_KEY_INSERT:
            return ALIA_HID_INSERT;
        case GLFW_KEY_DELETE:
            return ALIA_HID_DELETE_FORWARD;
        case GLFW_KEY_RIGHT:
            return ALIA_HID_RIGHT_ARROW;
        case GLFW_KEY_LEFT:
            return ALIA_HID_LEFT_ARROW;
        case GLFW_KEY_DOWN:
            return ALIA_HID_DOWN_ARROW;
        case GLFW_KEY_UP:
            return ALIA_HID_UP_ARROW;
        case GLFW_KEY_PAGE_UP:
            return ALIA_HID_PAGE_UP;
        case GLFW_KEY_PAGE_DOWN:
            return ALIA_HID_PAGE_DOWN;
        case GLFW_KEY_HOME:
            return ALIA_HID_HOME;
        case GLFW_KEY_END:
            return ALIA_HID_END;
        case GLFW_KEY_CAPS_LOCK:
            return ALIA_HID_CAPS_LOCK;
        case GLFW_KEY_SCROLL_LOCK:
            return ALIA_HID_SCROLL_LOCK;
        case GLFW_KEY_NUM_LOCK:
            return ALIA_HID_NUM_LOCK;
        case GLFW_KEY_PRINT_SCREEN:
            return ALIA_HID_PRINT_SCREEN;
        case GLFW_KEY_PAUSE:
            return ALIA_HID_PAUSE;
        case GLFW_KEY_LEFT_SHIFT:
            return ALIA_HID_LEFT_SHIFT;
        case GLFW_KEY_LEFT_CONTROL:
            return ALIA_HID_LEFT_CTRL;
        case GLFW_KEY_LEFT_ALT:
            return ALIA_HID_LEFT_ALT;
        case GLFW_KEY_LEFT_SUPER:
            return ALIA_HID_LEFT_GUI;
        case GLFW_KEY_RIGHT_SHIFT:
            return ALIA_HID_RIGHT_SHIFT;
        case GLFW_KEY_RIGHT_CONTROL:
            return ALIA_HID_RIGHT_CTRL;
        case GLFW_KEY_RIGHT_ALT:
            return ALIA_HID_RIGHT_ALT;
        case GLFW_KEY_RIGHT_SUPER:
            return ALIA_HID_RIGHT_GUI;
        case GLFW_KEY_MENU:
            return ALIA_HID_APPLICATION;
        case GLFW_KEY_KP_0:
            return ALIA_HID_KP_0;
        case GLFW_KEY_KP_1:
            return ALIA_HID_KP_1;
        case GLFW_KEY_KP_2:
            return ALIA_HID_KP_2;
        case GLFW_KEY_KP_3:
            return ALIA_HID_KP_3;
        case GLFW_KEY_KP_4:
            return ALIA_HID_KP_4;
        case GLFW_KEY_KP_5:
            return ALIA_HID_KP_5;
        case GLFW_KEY_KP_6:
            return ALIA_HID_KP_6;
        case GLFW_KEY_KP_7:
            return ALIA_HID_KP_7;
        case GLFW_KEY_KP_8:
            return ALIA_HID_KP_8;
        case GLFW_KEY_KP_9:
            return ALIA_HID_KP_9;
        case GLFW_KEY_KP_DECIMAL:
            return ALIA_HID_KP_DECIMAL;
        case GLFW_KEY_KP_DIVIDE:
            return ALIA_HID_KP_DIVIDE;
        case GLFW_KEY_KP_MULTIPLY:
            return ALIA_HID_KP_MULTIPLY;
        case GLFW_KEY_KP_SUBTRACT:
            return ALIA_HID_KP_MINUS;
        case GLFW_KEY_KP_ADD:
            return ALIA_HID_KP_PLUS;
        case GLFW_KEY_KP_ENTER:
            return ALIA_HID_KP_ENTER;
        case GLFW_KEY_KP_EQUAL:
            return ALIA_HID_KEYPAD_EQUAL;
        default:
            return ALIA_HID_UNKNOWN;
    }
}

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
    alia_ui_system* ui, GLFWwindow* window, int button, int action, int mods)
{
    ALIA_ASSERT(ui);
    ALIA_ASSERT(window);

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
    ALIA_ASSERT(ui);
    ALIA_ASSERT(window);
    alia_vec2f const p = alia_glfw_cursor_window_to_framebuffer(window, x, y);
    alia_ui_enqueue_mouse_motion(ui, p);
}

void
alia_glfw_enqueue_scroll(alia_ui_system* ui, double x, double y)
{
    ALIA_ASSERT(ui);
    // GLFW reports wheel clicks as small floats (typically +/-1 per notch)
    // with inverted vertical sign on Windows. Convert to canonical logical
    // pixels.
    alia_vec2f const delta = ALIA_BRACED_INIT(
        alia_vec2f,
        static_cast<float>(-x * ALIA_SCROLL_PIXELS_PER_WHEEL_NOTCH),
        static_cast<float>(-y * ALIA_SCROLL_PIXELS_PER_WHEEL_NOTCH));
    alia_ui_enqueue_scroll(ui, delta);
}

void
alia_glfw_enqueue_key(alia_ui_system* ui, int key, int action, int mods)
{
    ALIA_ASSERT(ui);
    alia_kmods_t const km = alia_glfw_mods_to_kmods(mods);
    alia_hid_key_t const hid = glfw_key_to_hid(key);
    alia_key_info mk{};
    mk.logical = static_cast<alia_key_code_t>(key);
    mk.mods = km;
    mk.fields_present = ALIA_KEY_FIELD_LOGICAL;
    if (hid != ALIA_HID_UNKNOWN)
    {
        mk.hid = hid;
        mk.fields_present = (alia_key_fields_present_t) (mk.fields_present
                                                         | ALIA_KEY_FIELD_HID);
    }
    else
    {
        mk.hid = ALIA_HID_UNKNOWN;
    }
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
        alia_ui_enqueue_key_press(ui, mk);
    else if (action == GLFW_RELEASE)
        alia_ui_enqueue_key_release(ui, mk);
}

static void
mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    auto* binding
        = static_cast<alia_glfw_ui_binding*>(glfwGetWindowUserPointer(window));
    ALIA_ASSERT(binding);
    ALIA_ASSERT(binding->ui);
    alia_glfw_enqueue_mouse_button(binding->ui, window, button, action, mods);
}

static void
cursor_position_callback(GLFWwindow* window, double x, double y)
{
    auto* binding
        = static_cast<alia_glfw_ui_binding*>(glfwGetWindowUserPointer(window));
    ALIA_ASSERT(binding);
    ALIA_ASSERT(binding->ui);
    alia_glfw_enqueue_cursor_pos(binding->ui, window, x, y);
}

static void
scroll_callback(GLFWwindow* window, double x, double y)
{
    auto* binding
        = static_cast<alia_glfw_ui_binding*>(glfwGetWindowUserPointer(window));
    ALIA_ASSERT(binding);
    ALIA_ASSERT(binding->ui);
    alia_glfw_enqueue_scroll(binding->ui, x, y);
}

static void
key_callback(
    GLFWwindow* window, int key, int /*scancode*/, int action, int mods)
{
    auto* binding
        = static_cast<alia_glfw_ui_binding*>(glfwGetWindowUserPointer(window));
    ALIA_ASSERT(binding);
    ALIA_ASSERT(binding->ui);

    if (key == GLFW_KEY_F11 && action == GLFW_PRESS && mods == 0
        && binding->host)
    {
        alia_glfw_host_toggle_fullscreen(binding->host);
        return;
    }

    alia_glfw_enqueue_key(binding->ui, key, action, mods);
}

static void
framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    auto* binding
        = static_cast<alia_glfw_ui_binding*>(glfwGetWindowUserPointer(window));
    ALIA_ASSERT(binding);
    ALIA_ASSERT(binding->ui);
    alia_vec2i const size = ALIA_BRACED_INIT(alia_vec2i, width, height);
    alia_ui_surface_set_size(binding->ui, size);
    alia_glfw_host_on_framebuffer_resized(window);
}

static void
content_scale_callback(GLFWwindow* window, float xscale, float yscale)
{
    auto* binding
        = static_cast<alia_glfw_ui_binding*>(glfwGetWindowUserPointer(window));
    ALIA_ASSERT(binding);
    ALIA_ASSERT(binding->ui);
    (void) window;
    alia_ui_surface_set_dpi(binding->ui, ((xscale + yscale) / 2.0f) * 96.f);
}

void
alia_glfw_install_default_input_callbacks(
    GLFWwindow* window, alia_glfw_ui_binding* binding)
{
    ALIA_ASSERT(window);
    ALIA_ASSERT(binding);
    ALIA_ASSERT(binding->ui);
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
    ALIA_ASSERT(window);
    ALIA_ASSERT(binding);
    ALIA_ASSERT(binding->ui);
    glfwSetWindowUserPointer(window, binding);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetWindowContentScaleCallback(window, content_scale_callback);
}

} // extern "C"
