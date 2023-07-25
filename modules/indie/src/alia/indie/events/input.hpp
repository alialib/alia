#ifndef ALIA_INDIE_EVENTS_COMMON_HPP
#define ALIA_INDIE_EVENTS_COMMON_HPP

#include <alia/indie/common.hpp>

// This file enumerates the various input events that are delivered from the UI
// system to individual widgets and defines a base class for representing them.
// For more info on the individual events, see mouse.hpp, keyboard.hpp, etc.

namespace alia { namespace indie {

struct system;

enum class input_event_type
{
    // keyboard
    TEXT_INPUT_EVENT,
    BACKGROUND_TEXT_INPUT_EVENT,
    KEY_PRESS_EVENT,
    BACKGROUND_KEY_PRESS_EVENT,
    KEY_RELEASE_EVENT,
    BACKGROUND_KEY_RELEASE_EVENT,

    // mouse
    MOUSE_PRESS_EVENT,
    DOUBLE_CLICK_EVENT,
    MOUSE_RELEASE_EVENT,
    MOUSE_MOTION_EVENT,
    MOUSE_WHEEL_EVENT,
    MOUSE_GAIN_EVENT,
    MOUSE_LOSS_EVENT,
    MOUSE_HOVER_EVENT,

    // focus notifications
    FOCUS_GAIN_EVENT,
    FOCUS_LOSS_EVENT
};

struct input_event
{
    input_event_type type;
};

}} // namespace alia::indie

#endif
