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

typedef uint8_t alia_key_code_t;

#define ALIA_KEY_CODES(X)                                                     \
    X(0, UNKNOWN, unknown)                                                    \
    X(32, SPACE, space)                                                       \
    X(39, APOSTROPHE, apostrophe)                                             \
    X(44, COMMA, comma)                                                       \
    X(45, MINUS, minus)                                                       \
    X(46, PERIOD, period)                                                     \
    X(47, SLASH, slash)                                                       \
    X(48, 0, 0)                                                               \
    X(49, 1, 1)                                                               \
    X(50, 2, 2)                                                               \
    X(51, 3, 3)                                                               \
    X(52, 4, 4)                                                               \
    X(53, 5, 5)                                                               \
    X(54, 6, 6)                                                               \
    X(55, 7, 7)                                                               \
    X(56, 8, 8)                                                               \
    X(57, 9, 9)                                                               \
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
    X(259, BACKSPACE, backspace)                                              \
    X(260, INSERT, insert)                                                    \
    X(261, DEL, del)                                                          \
    X(262, RIGHT, right)                                                      \
    X(263, LEFT, left)                                                        \
    X(264, DOWN, down)                                                        \
    X(265, UP, up)                                                            \
    X(266, PAGE_UP, page_up)                                                  \
    X(267, PAGE_DOWN, page_down)                                              \
    X(268, HOME, home)                                                        \
    X(269, END, end)                                                          \
    X(280, CAPS_LOCK, caps_lock)                                              \
    X(291, F2, f2)                                                            \
    X(292, F3, f3)                                                            \
    X(293, F4, f4)                                                            \
    X(294, F5, f5)                                                            \
    X(295, F6, f6)                                                            \
    X(305, F16, f16)                                                          \
    X(306, F17, f17)                                                          \
    X(307, F18, f18)                                                          \
    X(308, F19, f19)                                                          \
    X(309, F20, f20)                                                          \
    X(310, F21, f21)                                                          \
    X(311, F22, f22)                                                          \
    X(312, F23, f23)                                                          \
    X(313, F24, f24)                                                          \
    X(314, F25, f25)                                                          \
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

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_INPUT_CONSTANTS_H */
