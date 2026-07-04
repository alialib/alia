#include <alia/abi/ui/input/constants.h>

#include <emscripten/html5.h>

#include <cstring>

namespace {

alia_kmods_t
web_mods_from_keyboard_event(EmscriptenKeyboardEvent const* event)
{
    alia_kmods_t mods = 0;
    if (event->shiftKey)
        mods |= ALIA_KMOD_SHIFT;
    if (event->ctrlKey)
        mods |= ALIA_KMOD_CTRL;
    if (event->altKey)
        mods |= ALIA_KMOD_ALT;
    if (event->metaKey)
        mods |= ALIA_KMOD_WIN;
    return mods;
}

alia_key_code_t
web_logical_key_from_code(char const* code)
{
    if (!code || !code[0])
        return ALIA_KEY_UNKNOWN;

    if (code[0] == 'F' && code[1] >= '1' && code[1] <= '9')
    {
        int digit = code[1] - '0';
        if (code[2] == '\0')
            return static_cast<alia_key_code_t>(ALIA_KEY_F1 + digit - 1);
        if (digit == 1 && code[2] == '0' && code[3] == '\0')
            return ALIA_KEY_F10;
        if (digit == 1 && code[2] == '1' && code[3] == '\0')
            return ALIA_KEY_F11;
        if (digit == 1 && code[2] == '2' && code[3] == '\0')
            return ALIA_KEY_F12;
    }

    if (code[0] == 'K' && code[1] == 'e' && code[2] == 'y')
    {
        if (std::strcmp(code, "KeyEqual") == 0)
            return ALIA_KEY_EQUAL;
        char const c = code[3];
        if (code[4] == '\0' && c >= 'A' && c <= 'Z')
            return static_cast<alia_key_code_t>(ALIA_KEY_A + (c - 'A'));
        if (code[4] == '\0' && c >= '0' && c <= '9')
            return static_cast<alia_key_code_t>(ALIA_KEY_DIGIT_0 + (c - '0'));
    }

    if (std::strcmp(code, "Equal") == 0)
        return ALIA_KEY_EQUAL;
    if (std::strcmp(code, "Minus") == 0)
        return ALIA_KEY_MINUS;
    if (std::strcmp(code, "Space") == 0)
        return ALIA_KEY_SPACE;
    if (std::strcmp(code, "Escape") == 0)
        return ALIA_KEY_ESCAPE;
    if (std::strcmp(code, "Enter") == 0)
        return ALIA_KEY_ENTER;
    if (std::strcmp(code, "Tab") == 0)
        return ALIA_KEY_TAB;
    if (std::strcmp(code, "Backspace") == 0)
        return ALIA_KEY_BACKSPACE;
    if (std::strcmp(code, "ArrowLeft") == 0)
        return ALIA_KEY_LEFT_ARROW;
    if (std::strcmp(code, "ArrowRight") == 0)
        return ALIA_KEY_RIGHT_ARROW;
    if (std::strcmp(code, "ArrowUp") == 0)
        return ALIA_KEY_UP_ARROW;
    if (std::strcmp(code, "ArrowDown") == 0)
        return ALIA_KEY_DOWN_ARROW;

    return ALIA_KEY_UNKNOWN;
}

alia_key_info
web_key_info_from_keyboard_event(EmscriptenKeyboardEvent const* event)
{
    alia_kmods_t const mods = web_mods_from_keyboard_event(event);
    alia_key_code_t const logical = web_logical_key_from_code(event->code);

    alia_key_info key{};
    key.mods = mods;
    key.logical = logical;
    key.hid = ALIA_HID_UNKNOWN;
    key.fields_present = ALIA_KEY_FIELD_LOGICAL;
    if (logical != ALIA_KEY_UNKNOWN)
        return key;

    if (event->keyCode >= 32 && event->keyCode <= 126)
    {
        key.logical = static_cast<alia_key_code_t>(event->keyCode);
        key.fields_present = ALIA_KEY_FIELD_LOGICAL;
    }

    return key;
}

} // namespace

extern "C" {

alia_key_info
alia_web_key_info_from_keyboard_event(EmscriptenKeyboardEvent const* event)
{
    return web_key_info_from_keyboard_event(event);
}

} // extern "C"
