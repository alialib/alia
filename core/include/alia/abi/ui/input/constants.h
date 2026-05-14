#ifndef ALIA_ABI_UI_INPUT_CONSTANTS_H
#define ALIA_ABI_UI_INPUT_CONSTANTS_H

#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

// MOUSE/POINTER BUTTONS

typedef uint8_t alia_button_t;

#define ALIA_MAX_SUPPORTED_BUTTONS 8

#define ALIA_BUTTONS(X)                                                       \
    X(0x0, 1, b1)                                                             \
    X(0x1, 2, b2)                                                             \
    X(0x2, 3, b3)                                                             \
    X(0x3, 4, b4)                                                             \
    X(0x4, 5, b5)                                                             \
    X(0x5, 6, b6)                                                             \
    X(0x6, 7, b7)                                                             \
    X(0x7, 8, b8)                                                             \
    X(0x0, LEFT, left)                                                        \
    X(0x1, MIDDLE, middle)                                                    \
    X(0x2, RIGHT, right)

#define X(code, NAME, name) ALIA_BUTTON_##NAME = (code),
enum
{
    ALIA_BUTTONS(X)
};
#undef X

// CURSORS

typedef uint8_t alia_cursor_t;

// list of standard cursors that are expected to be supplied by the backend -
// These are named after the web standard.
#define ALIA_CURSORS(X)                                                       \
    X(NONE)                                                                   \
    X(DEFAULT)                                                                \
    X(WAIT)                                                                   \
    X(CROSSHAIR)                                                              \
    X(TEXT)                                                                   \
    X(NOT_ALLOWED)                                                            \
    X(POINTER)                                                                \
    X(MOVE)                                                                   \
    X(EW_RESIZE)                                                              \
    X(NS_RESIZE)                                                              \
    X(NESW_RESIZE)                                                            \
    X(NWSE_RESIZE)                                                            \
    X(ZOOM_IN)                                                                \
    X(ZOOM_OUT)

// Define the enum for the standard cursors.
enum
{
#define X(name) ALIA_CURSOR_##name,
    ALIA_CURSORS(X)
#undef X
};

// KEYS
//
// `alia_key_code_t` is the logical key identity (stable Alia enum; GLFW and
// other hosts map into these values in their glue layers).
//
// `alia_hid_key_t` stores USB HID usage page 0x07 (keyboard / keypad)
// usage codes. `ALIA_HID_UNKNOWN` (0) means unmapped / not supplied.

typedef uint16_t alia_key_code_t;
typedef uint16_t alia_hid_key_t;
typedef uint8_t alia_key_fields_present_t;

#define ALIA_HID_UNKNOWN ((alia_hid_key_t) 0)

// which fields in `alia_key_info` are authoritative
#define ALIA_KEY_FIELD_HID ((alia_key_fields_present_t) 1u)
#define ALIA_KEY_FIELD_LOGICAL ((alia_key_fields_present_t) 2u)

// USB HID usage page 0x07 — US 104-key style coverage (names follow HID)
#define ALIA_HID_KEYS(X)                                                      \
    X(0x04, A, a)                                                             \
    X(0x05, B, b)                                                             \
    X(0x06, C, c)                                                             \
    X(0x07, D, d)                                                             \
    X(0x08, E, e)                                                             \
    X(0x09, F, f)                                                             \
    X(0x0A, G, g)                                                             \
    X(0x0B, H, h)                                                             \
    X(0x0C, I, i)                                                             \
    X(0x0D, J, j)                                                             \
    X(0x0E, K, k)                                                             \
    X(0x0F, L, l)                                                             \
    X(0x10, M, m)                                                             \
    X(0x11, N, n)                                                             \
    X(0x12, O, o)                                                             \
    X(0x13, P, p)                                                             \
    X(0x14, Q, q)                                                             \
    X(0x15, R, r)                                                             \
    X(0x16, S, s)                                                             \
    X(0x17, T, t)                                                             \
    X(0x18, U, u)                                                             \
    X(0x19, V, v)                                                             \
    X(0x1A, W, w)                                                             \
    X(0x1B, X, x)                                                             \
    X(0x1C, Y, y)                                                             \
    X(0x1D, Z, z)                                                             \
    X(0x1E, DIGIT_1, digit_1)                                                 \
    X(0x1F, DIGIT_2, digit_2)                                                 \
    X(0x20, DIGIT_3, digit_3)                                                 \
    X(0x21, DIGIT_4, digit_4)                                                 \
    X(0x22, DIGIT_5, digit_5)                                                 \
    X(0x23, DIGIT_6, digit_6)                                                 \
    X(0x24, DIGIT_7, digit_7)                                                 \
    X(0x25, DIGIT_8, digit_8)                                                 \
    X(0x26, DIGIT_9, digit_9)                                                 \
    X(0x27, DIGIT_0, digit_0)                                                 \
    X(0x28, ENTER, enter)                                                     \
    X(0x29, ESCAPE, escape)                                                   \
    X(0x2A, BACKSPACE, backspace)                                             \
    X(0x2B, TAB, tab)                                                         \
    X(0x2C, SPACE, space)                                                     \
    X(0x2D, MINUS, minus)                                                     \
    X(0x2E, EQUAL, equal)                                                     \
    X(0x2F, LEFT_BRACKET, left_bracket)                                       \
    X(0x30, RIGHT_BRACKET, right_bracket)                                     \
    X(0x31, BACKSLASH, backslash)                                             \
    X(0x32, INTL_BACKSLASH, intl_backslash)                                   \
    X(0x33, SEMICOLON, semicolon)                                             \
    X(0x34, APOSTROPHE, apostrophe)                                           \
    X(0x35, GRAVE_ACCENT, grave_accent)                                       \
    X(0x36, COMMA, comma)                                                     \
    X(0x37, PERIOD, period)                                                   \
    X(0x38, SLASH, slash)                                                     \
    X(0x39, CAPS_LOCK, caps_lock)                                             \
    X(0x3A, F1, f1)                                                           \
    X(0x3B, F2, f2)                                                           \
    X(0x3C, F3, f3)                                                           \
    X(0x3D, F4, f4)                                                           \
    X(0x3E, F5, f5)                                                           \
    X(0x3F, F6, f6)                                                           \
    X(0x40, F7, f7)                                                           \
    X(0x41, F8, f8)                                                           \
    X(0x42, F9, f9)                                                           \
    X(0x43, F10, f10)                                                         \
    X(0x44, F11, f11)                                                         \
    X(0x45, F12, f12)                                                         \
    X(0x46, PRINT_SCREEN, print_screen)                                       \
    X(0x47, SCROLL_LOCK, scroll_lock)                                         \
    X(0x48, PAUSE, pause)                                                     \
    X(0x49, INSERT, insert)                                                   \
    X(0x4A, HOME, home)                                                       \
    X(0x4B, PAGE_UP, page_up)                                                 \
    X(0x4C, DELETE_FORWARD, delete_forward)                                   \
    X(0x4D, END, end)                                                         \
    X(0x4E, PAGE_DOWN, page_down)                                             \
    X(0x4F, RIGHT_ARROW, right_arrow)                                         \
    X(0x50, LEFT_ARROW, left_arrow)                                           \
    X(0x51, DOWN_ARROW, down_arrow)                                           \
    X(0x52, UP_ARROW, up_arrow)                                               \
    X(0x53, NUM_LOCK, num_lock)                                               \
    X(0x54, KP_DIVIDE, kp_divide)                                             \
    X(0x55, KP_MULTIPLY, kp_multiply)                                         \
    X(0x56, KP_MINUS, kp_minus)                                               \
    X(0x57, KP_PLUS, kp_plus)                                                 \
    X(0x58, KP_ENTER, kp_enter)                                               \
    X(0x59, KP_1, kp_1)                                                       \
    X(0x5A, KP_2, kp_2)                                                       \
    X(0x5B, KP_3, kp_3)                                                       \
    X(0x5C, KP_4, kp_4)                                                       \
    X(0x5D, KP_5, kp_5)                                                       \
    X(0x5E, KP_6, kp_6)                                                       \
    X(0x5F, KP_7, kp_7)                                                       \
    X(0x60, KP_8, kp_8)                                                       \
    X(0x61, KP_9, kp_9)                                                       \
    X(0x62, KP_0, kp_0)                                                       \
    X(0x63, KP_DECIMAL, kp_decimal)                                           \
    X(0x67, KEYPAD_EQUAL, keypad_equal)                                       \
    X(0x65, APPLICATION, application)                                         \
    X(0xE0, LEFT_CTRL, left_ctrl)                                             \
    X(0xE1, LEFT_SHIFT, left_shift)                                           \
    X(0xE2, LEFT_ALT, left_alt)                                               \
    X(0xE3, LEFT_GUI, left_gui)                                               \
    X(0xE4, RIGHT_CTRL, right_ctrl)                                           \
    X(0xE5, RIGHT_SHIFT, right_shift)                                         \
    X(0xE6, RIGHT_ALT, right_alt)                                             \
    X(0xE7, RIGHT_GUI, right_gui)

enum
{
#define X(code, NAME, name) ALIA_HID_##NAME = (code),
    ALIA_HID_KEYS(X)
#undef X
};

#define ALIA_KEY_CODES(X)                                                     \
    X(0, UNKNOWN, unknown)                                                    \
    X(32, SPACE, space)                                                       \
    X(39, APOSTROPHE, apostrophe)                                             \
    X(44, COMMA, comma)                                                       \
    X(45, MINUS, minus)                                                       \
    X(46, PERIOD, period)                                                     \
    X(47, SLASH, slash)                                                       \
    X(48, DIGIT_0, digit_0)                                                   \
    X(49, DIGIT_1, digit_1)                                                   \
    X(50, DIGIT_2, digit_2)                                                   \
    X(51, DIGIT_3, digit_3)                                                   \
    X(52, DIGIT_4, digit_4)                                                   \
    X(53, DIGIT_5, digit_5)                                                   \
    X(54, DIGIT_6, digit_6)                                                   \
    X(55, DIGIT_7, digit_7)                                                   \
    X(56, DIGIT_8, digit_8)                                                   \
    X(57, DIGIT_9, digit_9)                                                   \
    X(59, SEMICOLON, semicolon)                                               \
    X(61, EQUAL, equal)                                                       \
    X(65, A, a)                                                               \
    X(66, B, b)                                                               \
    X(67, C, c)                                                               \
    X(68, D, d)                                                               \
    X(69, E, e)                                                               \
    X(70, F, f)                                                               \
    X(71, G, g)                                                               \
    X(72, H, h)                                                               \
    X(73, I, i)                                                               \
    X(74, J, j)                                                               \
    X(75, K, k)                                                               \
    X(76, L, l)                                                               \
    X(77, M, m)                                                               \
    X(78, N, n)                                                               \
    X(79, O, o)                                                               \
    X(80, P, p)                                                               \
    X(81, Q, q)                                                               \
    X(82, R, r)                                                               \
    X(83, S, s)                                                               \
    X(84, T, t)                                                               \
    X(85, U, u)                                                               \
    X(86, V, v)                                                               \
    X(87, W, w)                                                               \
    X(88, X, x)                                                               \
    X(89, Y, y)                                                               \
    X(90, Z, z)                                                               \
    X(256, ESCAPE, escape)                                                    \
    X(257, ENTER, enter)                                                      \
    X(258, TAB, tab)                                                          \
    X(259, BACKSPACE, backspace)                                              \
    X(260, INSERT, insert)                                                    \
    X(261, DEL, del)                                                          \
    X(262, RIGHT_ARROW, right_arrow)                                          \
    X(263, LEFT_ARROW, left_arrow)                                            \
    X(264, DOWN_ARROW, down_arrow)                                            \
    X(265, UP_ARROW, up_arrow)                                                \
    X(266, PAGE_UP, page_up)                                                  \
    X(267, PAGE_DOWN, page_down)                                              \
    X(268, HOME, home)                                                        \
    X(269, END, end)                                                          \
    X(280, CAPS_LOCK, caps_lock)                                              \
    X(290, F1, f1)                                                            \
    X(291, F2, f2)                                                            \
    X(292, F3, f3)                                                            \
    X(293, F4, f4)                                                            \
    X(294, F5, f5)                                                            \
    X(295, F6, f6)                                                            \
    X(296, F7, f7)                                                            \
    X(297, F8, f8)                                                            \
    X(298, F9, f9)                                                            \
    X(299, F10, f10)                                                          \
    X(300, F11, f11)                                                          \
    X(301, F12, f12)                                                          \
    X(302, F13, f13)                                                          \
    X(303, F14, f14)                                                          \
    X(304, F15, f15)                                                          \
    X(305, F16, f16)                                                          \
    X(306, F17, f17)                                                          \
    X(307, F18, f18)                                                          \
    X(308, F19, f19)                                                          \
    X(309, F20, f20)                                                          \
    X(310, F21, f21)                                                          \
    X(311, F22, f22)                                                          \
    X(312, F23, f23)                                                          \
    X(313, F24, f24)                                                          \
    X(320, KP_0, kp_0)                                                        \
    X(321, KP_1, kp_1)                                                        \
    X(322, KP_2, kp_2)                                                        \
    X(323, KP_3, kp_3)                                                        \
    X(324, KP_4, kp_4)                                                        \
    X(325, KP_5, kp_5)                                                        \
    X(326, KP_6, kp_6)                                                        \
    X(327, KP_7, kp_7)                                                        \
    X(328, KP_8, kp_8)                                                        \
    X(329, KP_9, kp_9)                                                        \
    X(336, KP_EQUAL, kp_equal)                                                \
    X(340, LEFT_SHIFT, left_shift)                                            \
    X(341, LEFT_CONTROL, left_control)                                        \
    X(342, LEFT_ALT, left_alt)                                                \
    X(343, LEFT_SUPER, left_super)                                            \
    X(344, RIGHT_SHIFT, right_shift)                                          \
    X(345, RIGHT_CONTROL, right_control)                                      \
    X(346, RIGHT_ALT, right_alt)                                              \
    X(347, RIGHT_SUPER, right_super)                                          \
    X(348, MENU, menu)                                                        \
    X(1024, CLEAR, clear)                                                     \
    X(1025, HELP, help)

enum
{
#define X(code, NAME, name) ALIA_KEY_##NAME = (code),
    ALIA_KEY_CODES(X)
#undef X
};

// KEYBOARD MODIFIERS

typedef uint8_t alia_kmods_t;
#define ALIA_KEY_MODS(X)                                                      \
    X(0x00, NONE, none)                                                       \
    X(0x01, SHIFT, shift)                                                     \
    X(0x02, CTRL, ctrl)                                                       \
    X(0x04, ALT, alt)                                                         \
    X(0x08, WIN, win)                                                         \
    X(0x10, META, meta)

#define X(code, NAME, name) ALIA_KMOD_##NAME = (code),
enum
{
    ALIA_KEY_MODS(X)
};
#undef X

// Logical + HID identity for routing and host enqueue (`alia_key_event_desc`
// is the same layout).
typedef struct alia_key_info
{
    alia_hid_key_t hid;
    alia_key_code_t logical;
    alia_key_fields_present_t fields_present;
    alia_kmods_t mods;
} alia_key_info;

typedef alia_key_info alia_key_event_desc;

static inline bool
alia_key_info_has_hid(alia_key_info k)
{
    return (k.fields_present & ALIA_KEY_FIELD_HID) != 0;
}

static inline bool
alia_key_info_has_logical(alia_key_info k)
{
    return (k.fields_present & ALIA_KEY_FIELD_LOGICAL) != 0;
}

static inline alia_key_info
alia_key_info_make_logical(alia_key_code_t logical, alia_kmods_t mods)
{
    alia_key_info k;
    k.hid = ALIA_HID_UNKNOWN;
    k.logical = logical;
    k.fields_present = ALIA_KEY_FIELD_LOGICAL;
    k.mods = mods;
    return k;
}

static inline alia_key_info
alia_key_info_make_hid(alia_hid_key_t hid, alia_kmods_t mods)
{
    alia_key_info k;
    k.hid = hid;
    k.logical = ALIA_KEY_UNKNOWN;
    k.fields_present = ALIA_KEY_FIELD_HID;
    k.mods = mods;
    return k;
}

static inline alia_key_info
alia_key_info_make_both(
    alia_hid_key_t hid, alia_key_code_t logical, alia_kmods_t mods)
{
    alia_key_info k;
    k.hid = hid;
    k.logical = logical;
    k.fields_present = (alia_key_fields_present_t) (ALIA_KEY_FIELD_HID
                                                    | ALIA_KEY_FIELD_LOGICAL);
    k.mods = mods;
    return k;
}

// `spec` uses `fields_present` to select which axes must match. For each set
// bit, `incoming` must have that bit set too and the same value. Modifiers
// must always match exactly. Returns false if `spec` selects no axis.
static inline bool
alia_key_info_matches_spec(alia_key_info incoming, alia_key_info spec)
{
    if (incoming.mods != spec.mods)
        return false;
    alia_key_fields_present_t const want = spec.fields_present;
    if (want == 0)
        return false;
    if ((want & ALIA_KEY_FIELD_HID) != 0)
    {
        if ((incoming.fields_present & ALIA_KEY_FIELD_HID) == 0)
            return false;
        if (incoming.hid != spec.hid)
            return false;
    }
    if ((want & ALIA_KEY_FIELD_LOGICAL) != 0)
    {
        if ((incoming.fields_present & ALIA_KEY_FIELD_LOGICAL) == 0)
            return false;
        if (incoming.logical != spec.logical)
            return false;
    }
    return true;
}

// True if `k` reports HID and it equals `hid`.
static inline bool
alia_key_info_hid_equals(alia_key_info k, alia_hid_key_t hid)
{
    return alia_key_info_has_hid(k) && k.hid == hid;
}

// True if `k.logical` equals `logical` (ignores `fields_present`; matches
// backends that always populate logical alongside HID).
static inline bool
alia_key_info_logical_value_equals(alia_key_info k, alia_key_code_t logical)
{
    return k.logical == logical;
}

// True if the key matches `hid` when the HID axis is present, otherwise if the
// logical value matches (typical for navigation that prefers HID when known).
static inline bool
alia_key_info_matches_hid_or_logical(
    alia_key_info k, alia_hid_key_t hid, alia_key_code_t logical)
{
    return alia_key_info_hid_equals(k, hid)
        || alia_key_info_logical_value_equals(k, logical);
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_INPUT_CONSTANTS_H */
