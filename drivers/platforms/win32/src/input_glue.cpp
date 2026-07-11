#include <alia/platforms/win32/input_glue.h>

#include <alia/abi/ui/input/scroll.h>
#include <alia/abi/ui/system/input_processing.h>

namespace {

alia_hid_key_t
logical_to_hid(alia_key_code_t key)
{
    if (key >= ALIA_KEY_A && key <= ALIA_KEY_Z)
        return (alia_hid_key_t) (ALIA_HID_A + (key - ALIA_KEY_A));
    if (key >= ALIA_KEY_DIGIT_1 && key <= ALIA_KEY_DIGIT_9)
        return (alia_hid_key_t) (ALIA_HID_DIGIT_1 + (key - ALIA_KEY_DIGIT_1));
    if (key == ALIA_KEY_DIGIT_0)
        return ALIA_HID_DIGIT_0;
    if (key >= ALIA_KEY_F1 && key <= ALIA_KEY_F12)
        return (alia_hid_key_t) (ALIA_HID_F1 + (key - ALIA_KEY_F1));

    switch (key)
    {
        case ALIA_KEY_SPACE:
            return ALIA_HID_SPACE;
        case ALIA_KEY_APOSTROPHE:
            return ALIA_HID_APOSTROPHE;
        case ALIA_KEY_COMMA:
            return ALIA_HID_COMMA;
        case ALIA_KEY_MINUS:
            return ALIA_HID_MINUS;
        case ALIA_KEY_PERIOD:
            return ALIA_HID_PERIOD;
        case ALIA_KEY_SLASH:
            return ALIA_HID_SLASH;
        case ALIA_KEY_SEMICOLON:
            return ALIA_HID_SEMICOLON;
        case ALIA_KEY_EQUAL:
            return ALIA_HID_EQUAL;
        case ALIA_KEY_ESCAPE:
            return ALIA_HID_ESCAPE;
        case ALIA_KEY_ENTER:
            return ALIA_HID_ENTER;
        case ALIA_KEY_TAB:
            return ALIA_HID_TAB;
        case ALIA_KEY_BACKSPACE:
            return ALIA_HID_BACKSPACE;
        case ALIA_KEY_INSERT:
            return ALIA_HID_INSERT;
        case ALIA_KEY_DEL:
            return ALIA_HID_DELETE_FORWARD;
        case ALIA_KEY_RIGHT_ARROW:
            return ALIA_HID_RIGHT_ARROW;
        case ALIA_KEY_LEFT_ARROW:
            return ALIA_HID_LEFT_ARROW;
        case ALIA_KEY_DOWN_ARROW:
            return ALIA_HID_DOWN_ARROW;
        case ALIA_KEY_UP_ARROW:
            return ALIA_HID_UP_ARROW;
        case ALIA_KEY_PAGE_UP:
            return ALIA_HID_PAGE_UP;
        case ALIA_KEY_PAGE_DOWN:
            return ALIA_HID_PAGE_DOWN;
        case ALIA_KEY_HOME:
            return ALIA_HID_HOME;
        case ALIA_KEY_END:
            return ALIA_HID_END;
        case ALIA_KEY_CAPS_LOCK:
            return ALIA_HID_CAPS_LOCK;
        case ALIA_KEY_LEFT_SHIFT:
            return ALIA_HID_LEFT_SHIFT;
        case ALIA_KEY_LEFT_CONTROL:
            return ALIA_HID_LEFT_CTRL;
        case ALIA_KEY_LEFT_ALT:
            return ALIA_HID_LEFT_ALT;
        case ALIA_KEY_LEFT_SUPER:
            return ALIA_HID_LEFT_GUI;
        case ALIA_KEY_RIGHT_SHIFT:
            return ALIA_HID_RIGHT_SHIFT;
        case ALIA_KEY_RIGHT_CONTROL:
            return ALIA_HID_RIGHT_CTRL;
        case ALIA_KEY_RIGHT_ALT:
            return ALIA_HID_RIGHT_ALT;
        case ALIA_KEY_RIGHT_SUPER:
            return ALIA_HID_RIGHT_GUI;
        case ALIA_KEY_KP_0:
            return ALIA_HID_KP_0;
        case ALIA_KEY_KP_1:
            return ALIA_HID_KP_1;
        case ALIA_KEY_KP_2:
            return ALIA_HID_KP_2;
        case ALIA_KEY_KP_3:
            return ALIA_HID_KP_3;
        case ALIA_KEY_KP_4:
            return ALIA_HID_KP_4;
        case ALIA_KEY_KP_5:
            return ALIA_HID_KP_5;
        case ALIA_KEY_KP_6:
            return ALIA_HID_KP_6;
        case ALIA_KEY_KP_7:
            return ALIA_HID_KP_7;
        case ALIA_KEY_KP_8:
            return ALIA_HID_KP_8;
        case ALIA_KEY_KP_9:
            return ALIA_HID_KP_9;
        case 91: // LEFT_BRACKET
            return ALIA_HID_LEFT_BRACKET;
        case 92: // BACKSLASH
            return ALIA_HID_BACKSLASH;
        case 93: // RIGHT_BRACKET
            return ALIA_HID_RIGHT_BRACKET;
        case 96: // GRAVE_ACCENT
            return ALIA_HID_GRAVE_ACCENT;
        case 330: // KP_DECIMAL
            return ALIA_HID_KP_DECIMAL;
        case 331: // KP_DIVIDE
            return ALIA_HID_KP_DIVIDE;
        case 332: // KP_MULTIPLY
            return ALIA_HID_KP_MULTIPLY;
        case 333: // KP_SUBTRACT
            return ALIA_HID_KP_MINUS;
        case 334: // KP_ADD
            return ALIA_HID_KP_PLUS;
        case 335: // KP_ENTER
            return ALIA_HID_KP_ENTER;
        default:
            return ALIA_HID_UNKNOWN;
    }
}

alia_key_code_t
vk_to_logical(UINT vk, LPARAM lParam)
{
    bool const extended = (lParam & (1 << 24)) != 0;

    // Prefer scancode-based extended VKs for left/right modifiers.
    UINT const scancode = UINT((lParam >> 16) & 0xff);
    UINT const vk_ex = MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK_EX);
    if (vk_ex == VK_LSHIFT)
        return ALIA_KEY_LEFT_SHIFT;
    if (vk_ex == VK_RSHIFT)
        return ALIA_KEY_RIGHT_SHIFT;
    if (vk_ex == VK_LCONTROL)
        return ALIA_KEY_LEFT_CONTROL;
    if (vk_ex == VK_RCONTROL)
        return ALIA_KEY_RIGHT_CONTROL;
    if (vk_ex == VK_LMENU)
        return ALIA_KEY_LEFT_ALT;
    if (vk_ex == VK_RMENU)
        return ALIA_KEY_RIGHT_ALT;

    if (vk >= 'A' && vk <= 'Z')
        return (alia_key_code_t) vk;
    if (vk >= '0' && vk <= '9')
        return (alia_key_code_t) vk;

    switch (vk)
    {
        case VK_SPACE:
            return ALIA_KEY_SPACE;
        case VK_OEM_7:
            return ALIA_KEY_APOSTROPHE;
        case VK_OEM_COMMA:
            return ALIA_KEY_COMMA;
        case VK_OEM_MINUS:
            return ALIA_KEY_MINUS;
        case VK_OEM_PERIOD:
            return ALIA_KEY_PERIOD;
        case VK_OEM_2:
            return ALIA_KEY_SLASH;
        case VK_OEM_1:
            return ALIA_KEY_SEMICOLON;
        case VK_OEM_PLUS:
            return ALIA_KEY_EQUAL;
        case VK_OEM_4:
            return 91; // GLFW_KEY_LEFT_BRACKET
        case VK_OEM_6:
            return 93; // GLFW_KEY_RIGHT_BRACKET
        case VK_OEM_5:
            return 92; // GLFW_KEY_BACKSLASH
        case VK_OEM_3:
            return 96; // GLFW_KEY_GRAVE_ACCENT
        case VK_ESCAPE:
            return ALIA_KEY_ESCAPE;
        case VK_RETURN:
            return extended ? (alia_key_code_t) 335 : ALIA_KEY_ENTER; // KP_ENTER
        case VK_TAB:
            return ALIA_KEY_TAB;
        case VK_BACK:
            return ALIA_KEY_BACKSPACE;
        case VK_INSERT:
            return ALIA_KEY_INSERT;
        case VK_DELETE:
            return ALIA_KEY_DEL;
        case VK_RIGHT:
            return ALIA_KEY_RIGHT_ARROW;
        case VK_LEFT:
            return ALIA_KEY_LEFT_ARROW;
        case VK_DOWN:
            return ALIA_KEY_DOWN_ARROW;
        case VK_UP:
            return ALIA_KEY_UP_ARROW;
        case VK_PRIOR:
            return ALIA_KEY_PAGE_UP;
        case VK_NEXT:
            return ALIA_KEY_PAGE_DOWN;
        case VK_HOME:
            return ALIA_KEY_HOME;
        case VK_END:
            return ALIA_KEY_END;
        case VK_CAPITAL:
            return ALIA_KEY_CAPS_LOCK;
        case VK_F1:
            return ALIA_KEY_F1;
        case VK_F2:
            return ALIA_KEY_F2;
        case VK_F3:
            return ALIA_KEY_F3;
        case VK_F4:
            return ALIA_KEY_F4;
        case VK_F5:
            return ALIA_KEY_F5;
        case VK_F6:
            return ALIA_KEY_F6;
        case VK_F7:
            return ALIA_KEY_F7;
        case VK_F8:
            return ALIA_KEY_F8;
        case VK_F9:
            return ALIA_KEY_F9;
        case VK_F10:
            return ALIA_KEY_F10;
        case VK_F11:
            return ALIA_KEY_F11;
        case VK_F12:
            return ALIA_KEY_F12;
        case VK_NUMPAD0:
            return ALIA_KEY_KP_0;
        case VK_NUMPAD1:
            return ALIA_KEY_KP_1;
        case VK_NUMPAD2:
            return ALIA_KEY_KP_2;
        case VK_NUMPAD3:
            return ALIA_KEY_KP_3;
        case VK_NUMPAD4:
            return ALIA_KEY_KP_4;
        case VK_NUMPAD5:
            return ALIA_KEY_KP_5;
        case VK_NUMPAD6:
            return ALIA_KEY_KP_6;
        case VK_NUMPAD7:
            return ALIA_KEY_KP_7;
        case VK_NUMPAD8:
            return ALIA_KEY_KP_8;
        case VK_NUMPAD9:
            return ALIA_KEY_KP_9;
        case VK_DECIMAL:
            return 330; // GLFW_KEY_KP_DECIMAL
        case VK_DIVIDE:
            return 331; // GLFW_KEY_KP_DIVIDE
        case VK_MULTIPLY:
            return 332; // GLFW_KEY_KP_MULTIPLY
        case VK_SUBTRACT:
            return 333; // GLFW_KEY_KP_SUBTRACT
        case VK_ADD:
            return 334; // GLFW_KEY_KP_ADD
        case VK_LWIN:
            return ALIA_KEY_LEFT_SUPER;
        case VK_RWIN:
            return ALIA_KEY_RIGHT_SUPER;
        case VK_SHIFT:
            return ALIA_KEY_LEFT_SHIFT;
        case VK_CONTROL:
            return extended ? ALIA_KEY_RIGHT_CONTROL : ALIA_KEY_LEFT_CONTROL;
        case VK_MENU:
            return extended ? ALIA_KEY_RIGHT_ALT : ALIA_KEY_LEFT_ALT;
        default:
            return ALIA_KEY_UNKNOWN;
    }
}

} // namespace

extern "C" {

alia_kmods_t
alia_win32_current_kmods(void)
{
    alia_kmods_t mods = 0;
    if (GetKeyState(VK_SHIFT) & 0x8000)
        mods |= ALIA_KMOD_SHIFT;
    if (GetKeyState(VK_CONTROL) & 0x8000)
        mods |= ALIA_KMOD_CTRL;
    if (GetKeyState(VK_MENU) & 0x8000)
        mods |= ALIA_KMOD_ALT;
    if ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000))
        mods |= ALIA_KMOD_WIN;
    return mods;
}

alia_key_info
alia_win32_key_info_from_msg(WPARAM wParam, LPARAM lParam)
{
    alia_key_info info{};
    info.logical = vk_to_logical(UINT(wParam), lParam);
    info.mods = alia_win32_current_kmods();
    info.fields_present = ALIA_KEY_FIELD_LOGICAL;
    if (info.logical != ALIA_KEY_UNKNOWN)
    {
        info.hid = logical_to_hid(info.logical);
        if (info.hid != ALIA_HID_UNKNOWN)
        {
            info.fields_present = (alia_key_fields_present_t) (
                info.fields_present | ALIA_KEY_FIELD_HID);
        }
    }
    return info;
}

void
alia_win32_enqueue_mouse_motion(alia_ui_system* ui, float x, float y)
{
    ALIA_ASSERT(ui);
    alia_ui_enqueue_mouse_motion(ui, {x, y});
}

void
alia_win32_enqueue_mouse_button(
    alia_ui_system* ui,
    float x,
    float y,
    alia_button_t button,
    bool pressed)
{
    ALIA_ASSERT(ui);
    alia_kmods_t const mods = alia_win32_current_kmods();
    if (pressed)
        alia_ui_enqueue_mouse_press(ui, {x, y}, button, mods);
    else
        alia_ui_enqueue_mouse_release(ui, {x, y}, button, mods);
}

void
alia_win32_enqueue_double_click(
    alia_ui_system* ui, float x, float y, alia_button_t button)
{
    ALIA_ASSERT(ui);
    alia_ui_enqueue_double_click(
        ui, {x, y}, button, alia_win32_current_kmods());
}

void
alia_win32_enqueue_wheel(
    alia_ui_system* ui, short wheel_delta, bool horizontal)
{
    ALIA_ASSERT(ui);
    float const notches
        = float(wheel_delta) / float(WHEEL_DELTA);
    // Match GLFW glue: positive Alia scroll Y is wheel-down / content-up.
    alia_vec2f delta{};
    if (horizontal)
        delta.x = -notches * ALIA_SCROLL_PIXELS_PER_WHEEL_NOTCH;
    else
        delta.y = -notches * ALIA_SCROLL_PIXELS_PER_WHEEL_NOTCH;
    alia_ui_enqueue_scroll(ui, delta);
}

void
alia_win32_enqueue_key(
    alia_ui_system* ui, WPARAM wParam, LPARAM lParam, bool pressed)
{
    ALIA_ASSERT(ui);
    alia_key_info const info = alia_win32_key_info_from_msg(wParam, lParam);
    if (info.logical == ALIA_KEY_UNKNOWN)
        return;
    if (pressed)
        alia_ui_enqueue_key_press(ui, info);
    else
        alia_ui_enqueue_key_release(ui, info);
}

} // extern "C"
