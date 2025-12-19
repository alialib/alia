#pragma once

#include <stdint.h>

typedef uint8_t alia_mouse_button_t;

#define ALIA_MAX_SUPPORTED_MOUSE_BUTTONS 8

#define ALIA_MOUSE_BUTTONS(X)                                                 \
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

#define X(code, NAME, name) ALIA_MOUSE_BUTTON_##NAME = (code),
enum
{
    ALIA_MOUSE_BUTTONS(X)
};
#undef X

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

typedef uint8_t alia_kmod_t;
#define ALIA_KMOD_NONE 0x00
#define ALIA_KMOD_SHIFT 0x01
#define ALIA_KMOD_CTRL 0x02
#define ALIA_KMOD_ALT 0x04
#define ALIA_KMOD_WIN 0x08
#define ALIA_KMOD_META 0x10
