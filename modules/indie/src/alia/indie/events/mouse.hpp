#ifndef ALIA_INDIE_EVENTS_MOUSE_HPP
#define ALIA_INDIE_EVENTS_MOUSE_HPP

#include <alia/indie/events/input.hpp>

namespace alia { namespace indie {

// All mouse buttons have a code.
using mouse_button_code = int;

// The 'standard' mouse buttons have names.
// (Other buttons may exist, but they don't have names.)
enum class mouse_button
{
    LEFT,
    MIDDLE,
    RIGHT
};

// standard mouse cursors that are expected to be supplied by the backend
enum class mouse_cursor
{
    DEFAULT,
    CROSSHAIR,
    NONE,
    TEXT,
    POINTER,
    MOVE,
    EW_RESIZE,
    NS_RESIZE,
};

}} // namespace alia::indie

#endif
