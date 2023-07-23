#ifndef ALIA_INDIE_EVENTS_HPP
#define ALIA_INDIE_EVENTS_HPP

#include <alia/core/timing/ticks.hpp>

namespace alia {

// UI events recognized by alia
enum ui_event_type
{
    // regions
    MAKE_WIDGET_VISIBLE_EVENT,
    MOUSE_HIT_TEST_EVENT,
    WHEEL_HIT_TEST_EVENT,

    // keyboard
    TEXT_INPUT_EVENT,
    BACKGROUND_TEXT_INPUT_EVENT,
    KEY_PRESS_EVENT,
    BACKGROUND_KEY_PRESS_EVENT,
    KEY_RELEASE_EVENT,
    BACKGROUND_KEY_RELEASE_EVENT,

    // focus notifications
    FOCUS_GAIN_EVENT,
    FOCUS_LOSS_EVENT,

    // focus queries
    FOCUS_PREDECESSOR_EVENT,
    FOCUS_SUCCESSOR_EVENT,
    FOCUS_RECOVERY_EVENT,

    // mouse
    MOUSE_PRESS_EVENT,
    DOUBLE_CLICK_EVENT,
    MOUSE_RELEASE_EVENT,
    MOUSE_MOTION_EVENT,
    MOUSE_WHEEL_EVENT,
    MOUSE_CURSOR_QUERY_EVENT,
    MOUSE_GAIN_EVENT,
    MOUSE_LOSS_EVENT,
    MOUSE_HOVER_EVENT,
};

struct ui_event
{
    ui_event_type type;
};

struct input_event : ui_event
{
    bool acknowledged;
    millisecond_count time;

    input_event(ui_event_type type, millisecond_count time)
        : ui_event{type}, acknowledged(false), time(time)
    {
    }
};

// // keyboard events
// struct text_input_event : input_event
// {
//     utf8_string text;

//     text_input_event(ui_time_type time, utf8_string const& text)
//         : input_event(TEXT_INPUT_EVENT, time), text(text)
//     {
//     }
// };
// struct key_event_info
// {
//     key_code code;
//     key_modifiers mods;

//     key_event_info()
//     {
//     }
//     key_event_info(key_code code, key_modifiers mods) : code(code),
//     mods(mods)
//     {
//     }
// };
// struct key_event : input_event
// {
//     key_event_info info;

//     key_event(
//         ui_event_type event, ui_time_type time, key_event_info const& info)
//         : input_event(event, time), info(info)
//     {
//     }
// };

// // mouse events
// struct mouse_button_event : input_event
// {
//     mouse_button button;

//     mouse_button_event(
//         ui_event_type event, ui_time_type time, mouse_button button)
//         : input_event(event, time), button(button)
//     {
//     }
// };
// struct mouse_motion_event : input_event
// {
//     vector<2, int> last_mouse_position;
//     bool mouse_was_in_window;
//     mouse_motion_event(
//         ui_time_type time,
//         vector<2, int> const& last_mouse_position,
//         bool mouse_was_in_window)
//         : input_event(MOUSE_MOTION_EVENT, time),
//           last_mouse_position(last_mouse_position),
//           mouse_was_in_window(mouse_was_in_window)
//     {
//     }
// };
// struct mouse_notification_event : ui_event
// {
//     mouse_notification_event(ui_event_type type, widget_id target)
//         : ui_event(INPUT_CATEGORY, type), target(target)
//     {
//     }
//     widget_id target;
// };
// struct mouse_hit_test_event : ui_event
// {
//     routable_widget_id id;
//     mouse_cursor cursor;
//     layout_box hit_box;
//     string tooltip_message;

//     mouse_hit_test_event()
//         : ui_event(REGION_CATEGORY, MOUSE_HIT_TEST_EVENT),
//           id(null_widget_id),
//           cursor(DEFAULT_CURSOR)
//     {
//     }
// };

// struct mouse_wheel_event : input_event
// {
//     widget_id target;
//     float movement;
//     mouse_wheel_event(ui_time_type time, widget_id target, float movement)
//         : input_event(MOUSE_WHEEL_EVENT, time),
//           movement(movement),
//           target(target)
//     {
//     }
// };
// struct wheel_hit_test_event : ui_event
// {
//     routable_widget_id id;

//     wheel_hit_test_event()
//         : ui_event(REGION_CATEGORY, WHEEL_HIT_TEST_EVENT),
//         id(null_widget_id)
//     {
//     }
// };

// // If there is an active widget and it's not the one under the mouse cursor,
// // we have to query it to see what cursor it wants.
// struct mouse_cursor_query : ui_event
// {
//     widget_id id;
//     mouse_cursor cursor;

//     mouse_cursor_query(widget_id id)
//         : ui_event(REGION_CATEGORY, MOUSE_CURSOR_QUERY_EVENT),
//           id(id),
//           cursor(DEFAULT_CURSOR)
//     {
//     }
// };

// struct widget_visibility_request
// {
//     routable_widget_id widget;
//     // If this is set, the UI will jump abruptly instead of smoothly
//     scrolling. bool abrupt;
//     // If this is set, the widget will be moved to the top of the UI instead
//     // of just being made visible.
//     bool move_to_top;
// };

// struct make_widget_visible_event : ui_event
// {
//     make_widget_visible_event(widget_visibility_request const& request)
//         : ui_event(REGION_CATEGORY, MAKE_WIDGET_VISIBLE_EVENT),
//           request(request),
//           acknowledged(false)
//     {
//     }
//     widget_visibility_request request;
//     // This gets filled in once we find the widget in question.
//     box<2, double> region;
//     bool acknowledged;
// };

// struct resolve_location_event : ui_event
// {
//     resolve_location_event(owned_id const& id)
//         : ui_event(NO_CATEGORY, RESOLVE_LOCATION_EVENT),
//           id(id),
//           acknowledged(false)
//     {
//     }
//     owned_id id;
//     routable_widget_id routable_id;
//     bool acknowledged;
// };

// struct focus_notification_event : ui_event
// {
//     focus_notification_event(ui_event_type type, widget_id target)
//         : ui_event(INPUT_CATEGORY, type), target(target)
//     {
//     }
//     widget_id target;
// };

} // namespace alia

#endif
