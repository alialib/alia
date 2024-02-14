#ifndef ALIA_UI_EVENTS_INPUT_HPP
#define ALIA_UI_EVENTS_INPUT_HPP

#include "alia/core/timing/ticks.hpp"
#include <alia/core/flow/events.hpp>
#include <alia/ui/common.hpp>
#include <alia/ui/system/input_constants.hpp>
#include <alia/ui/widget.hpp>

// This file enumerates the various input events that are delivered from the UI
// system to individual widgets.

namespace alia {

struct system;

enum class input_event_type
{
    // keyboard
    TEXT_INPUT,
    KEY_PRESS,
    KEY_RELEASE,
    BACKGROUND_KEY_PRESS,
    BACKGROUND_KEY_RELEASE,

    // focus
    FOCUS_SUCCESSOR,
    FOCUS_PREDECESSOR,
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

    // timing
    TIMER,
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
    vector<2, double> delta;
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
    bool acknowledged = false;
};

// FOCUS_GAIN and FOCUS_LOSS
struct focus_notification_event : input_event
{
};

// FOCUS_SUCCESSOR
struct focus_successor_event : input_event
{
    internal_element_ref target;
    internal_element_ref successor;
    bool just_saw_target = false;
};

// FOCUS_PREDECESSOR
struct focus_predecessor_event : input_event
{
    internal_element_ref target;
    internal_element_ref predecessor;
    bool saw_target = false;
};

// TIMER
struct prototype_timer_event : input_event
{
    internal_element_ref target;
    millisecond_count trigger_time;
};

} // namespace alia

#endif