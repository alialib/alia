#ifndef ALIA_INDIE_EVENTS_INPUT_HPP
#define ALIA_INDIE_EVENTS_INPUT_HPP

#include <alia/core/flow/events.hpp>
#include <alia/indie/common.hpp>
#include <alia/indie/system/input_constants.hpp>

// This file enumerates the various input events that are delivered from the UI
// system to individual widgets.

namespace alia { namespace indie {

struct system;

enum class input_event_type
{
    // keyboard
    TEXT_INPUT,
    KEY_PRESS,
    KEY_RELEASE,

    // focus notifications
    FOCUS_GAIN,
    FOCUS_LOSS,

    // mouse
    MOUSE_PRESS,
    DOUBLE_CLICK,
    MOUSE_RELEASE,
    MOUSE_MOTION,
    MOUSE_GAIN,
    MOUSE_LOSS,
    MOUSE_HOVER,

    // scrolling (via the mouse wheel, gesture, etc.)
    SCROLL,
};

struct input_event : targeted_event
{
    input_event_type type;
};

// MOUSE_PRESS, DOUBLE_CLICK, and MOUSE_RELEASE
struct mouse_button_event : input_event
{
    mouse_button button;
};

// MOUSE_MOTION
struct mouse_motion_event : input_event
{
    vector<2, double> position;
};

// MOUSE_GAIN, MOUSE_LOSS, and MOUSE_HOVER
struct mouse_notification_event : input_event
{
};

// SCROLL
struct scroll_event : input_event
{
    double movement;
};

// TEXT_INPUT
struct text_input_event : input_event
{
    std::string text;
};

// KEY_PRESS and KEY_RELEASE
struct key_event : input_event
{
    modded_key key;
};

// FOCUS_GAIN and FOCUS_LOSS
struct focus_notification_event : input_event
{
};

}} // namespace alia::indie

#endif
