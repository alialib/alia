#ifndef ALIA_INPUT_DEFS_HPP
#define ALIA_INPUT_DEFS_HPP

namespace alia {

enum mouse_button
{
    LEFT_BUTTON = 0,
    MIDDLE_BUTTON,
    RIGHT_BUTTON,
};

// TODO
#ifdef KEY_EXECUTE
#undef KEY_EXECUTE
#endif

// Valid keys include the ASCII characters and the following.
// These use the same values as wxWidgets.
// Other interface implementations will have to map to these.
enum key_code
{
    KEY_BACK    = 8,
    KEY_TAB     = 9,
    KEY_ENTER   = 13,
    KEY_ESCAPE  = 27,
    KEY_SPACE   = 32,
    KEY_DELETE  = 127,

    KEY_START   = 300,
    KEY_LBUTTON,
    KEY_RBUTTON,
    KEY_CANCEL,
    KEY_MBUTTON,
    KEY_CLEAR,
    KEY_SHIFT,
    KEY_ALT,
    KEY_CONTROL,
    KEY_MENU,
    KEY_PAUSE,
    KEY_CAPITAL,
    KEY_END,
    KEY_HOME,
    KEY_LEFT,
    KEY_UP,
    KEY_RIGHT,
    KEY_DOWN,
    KEY_SELECT,
    KEY_PRINT,
    KEY_EXECUTE,
    KEY_SNAPSHOT,
    KEY_INSERT,
    KEY_HELP,
    KEY_NUMPAD0,
    KEY_NUMPAD1,
    KEY_NUMPAD2,
    KEY_NUMPAD3,
    KEY_NUMPAD4,
    KEY_NUMPAD5,
    KEY_NUMPAD6,
    KEY_NUMPAD7,
    KEY_NUMPAD8,
    KEY_NUMPAD9,
    KEY_MULTIPLY,
    KEY_ADD,
    KEY_SEPARATOR,
    KEY_SUBTRACT,
    KEY_DECIMAL,
    KEY_DIVIDE,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_F21,
    KEY_F22,
    KEY_F23,
    KEY_F24,
    KEY_NUMLOCK,
    KEY_SCROLL,
    KEY_PAGEUP,
    KEY_PAGEDOWN,

    KEY_NUMPAD_SPACE,
    KEY_NUMPAD_TAB,
    KEY_NUMPAD_ENTER,
    KEY_NUMPAD_F1,
    KEY_NUMPAD_F2,
    KEY_NUMPAD_F3,
    KEY_NUMPAD_F4,
    KEY_NUMPAD_HOME,
    KEY_NUMPAD_LEFT,
    KEY_NUMPAD_UP,
    KEY_NUMPAD_RIGHT,
    KEY_NUMPAD_DOWN,
    KEY_NUMPAD_PAGEUP,
    KEY_NUMPAD_PAGEDOWN,
    KEY_NUMPAD_END,
    KEY_NUMPAD_BEGIN,
    KEY_NUMPAD_INSERT,
    KEY_NUMPAD_DELETE,
    KEY_NUMPAD_EQUAL,
    KEY_NUMPAD_MULTIPLY,
    KEY_NUMPAD_ADD,
    KEY_NUMPAD_SEPARATOR,
    KEY_NUMPAD_SUBTRACT,
    KEY_NUMPAD_DECIMAL,
    KEY_NUMPAD_DIVIDE,

    KEY_WINDOWS_LEFT,
    KEY_WINDOWS_RIGHT,
    KEY_WINDOWS_MENU,
    KEY_COMMAND,
};

#ifdef MOD_NONE
  #undef MOD_NONE
#endif
#ifdef MOD_ALT
  #undef MOD_ALT
#endif
#ifdef MOD_CONTROL
  #undef MOD_CONTROL
#endif
#ifdef MOD_ALTGR
  #undef MOD_ALTGR
#endif
#ifdef MOD_SHIFT
  #undef MOD_SHIFT
#endif
#ifdef MOD_META
  #undef MOD_META
#endif
#ifdef MOD_ALL
  #undef MOD_ALL
#endif

// Again, the same as wxWidgets.
static unsigned const MOD_NONE      = 0x0000;
static unsigned const MOD_ALT       = 0x0001;
static unsigned const MOD_CONTROL   = 0x0002;
static unsigned const MOD_ALTGR     = MOD_ALT | MOD_CONTROL;
static unsigned const MOD_SHIFT     = 0x0004;
static unsigned const MOD_META      = 0x0008;
static unsigned const MOD_ALL       = 0xffff;

// key_event_info combines a key_code and key_modifiers
struct key_event_info
{
    key_code code;
    unsigned mods;
};

}

#endif
