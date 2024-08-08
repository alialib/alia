#ifndef ALIA_UI_EVENTS_HPP
#define ALIA_UI_EVENTS_HPP

#include <alia/core/flow/events.hpp>
#include <alia/core/timing/ticks.hpp>
#include <alia/ui/common.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/ids.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/system/input_constants.hpp>

#include <cstdint>

class SkCanvas;

namespace alia {

struct targeted_ui_event
{
    widget_id target = nullptr;
};

template<class Event>
void
dispatch_targeted_event(
    untyped_system& sys,
    Event& event,
    routable_widget_id const& target,
    event_type_code type_code = 0)
{
    event.target = target.id;
    detail::dispatch_targeted_event(sys, event, target.component, type_code);
}

ALIA_DEFINE_EVENT_CATEGORY(REGION_CATEGORY, 0x10)

ALIA_DEFINE_EVENT_TYPE_CODE(REGION_CATEGORY, MAKE_WIDGET_VISIBLE_EVENT, 0)

ALIA_DEFINE_FLAG_TYPE(widget_visibility_request)
// If this is set, the UI will jump abruptly instead of smoothly scrolling.
ALIA_DEFINE_FLAG(widget_visibility_request, 0x1, ABRUPT)
// If this is set, the widget will be moved to the top of the UI instead of
// just being made visible.
ALIA_DEFINE_FLAG(widget_visibility_request, 0x2, MOVE_TO_TOP)

struct widget_visibility_request
{
    routable_widget_id widget;
    widget_visibility_request_flag_set flags = NO_FLAGS;
};

// MAKE_WIDGET_VISIBLE_EVENT
struct make_widget_visible_event : targeted_ui_event
{
    widget_visibility_request_flag_set flags = NO_FLAGS;

    // This gets filled in once we find the widget in question.
    std::optional<box<2, double>> region;
};

ALIA_DEFINE_EVENT_TYPE_CODE(REGION_CATEGORY, MOUSE_HIT_TEST_EVENT, 1)

struct mouse_hit_test_result
{
    routable_widget_id id;
    mouse_cursor cursor;
    layout_box hit_box;
    std::string tooltip_message;
};

struct mouse_hit_test_event
{
    // the point to test
    vector<2, double> point;

    std::optional<mouse_hit_test_result> result;
};

ALIA_DEFINE_EVENT_TYPE_CODE(REGION_CATEGORY, WHEEL_HIT_TEST_EVENT, 2)

// WHEEL_HIT_TEST_EVENT
struct wheel_hit_test_event
{
    // the point to test
    vector<2, double> point;

    std::optional<routable_widget_id> result;
};

ALIA_DEFINE_EVENT_CATEGORY(INPUT_CATEGORY, 0x11)

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, TEXT_INPUT_EVENT, 0x00)

struct text_input_event : targeted_ui_event
{
    std::string text;
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, KEY_PRESS_EVENT, 0x01)
ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, KEY_RELEASE_EVENT, 0x02)

struct key_event : targeted_ui_event
{
    modded_key key;
    bool acknowledged = false;
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, BACKGROUND_KEY_PRESS_EVENT, 0x03)
ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, BACKGROUND_KEY_RELEASE_EVENT, 0x04)

struct background_key_event
{
    modded_key key;
    bool acknowledged = false;
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, FOCUS_GAIN_EVENT, 0x10)
ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, FOCUS_LOSS_EVENT, 0x11)

struct focus_notification_event : targeted_ui_event
{
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, FOCUS_SUCCESSOR_EVENT, 0x12)

struct focus_successor_event : targeted_ui_event
{
    widget_id target;
    routable_widget_id successor;
    bool just_saw_target = false;
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, FOCUS_PREDECESSOR_EVENT, 0x13)

struct focus_predecessor_event : targeted_ui_event
{
    widget_id target;
    routable_widget_id predecessor;
    bool saw_target = false;
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, MOUSE_PRESS_EVENT, 0x20)
ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, DOUBLE_CLICK_EVENT, 0x21)
ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, MOUSE_RELEASE_EVENT, 0x22)

struct mouse_button_event : targeted_ui_event
{
    mouse_button button;
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, MOUSE_MOTION_EVENT, 0x23)

struct mouse_motion_event : targeted_ui_event
{
    vector<2, double> position;
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, MOUSE_GAIN_EVENT, 0x24)
ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, MOUSE_LOSS_EVENT, 0x25)
ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, MOUSE_HOVER_EVENT, 0x26)

struct mouse_notification_event : targeted_ui_event
{
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, MOUSE_CURSOR_QUERY_EVENT, 0x27)

// If there is an active widget and it's not the one under the mouse cursor,
// we have to query it to see what cursor it wants.
struct mouse_cursor_query : targeted_ui_event
{
    mouse_cursor cursor = mouse_cursor::DEFAULT;
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, SCROLL_EVENT, 0x30)

struct scroll_event : targeted_ui_event
{
    vector<2, double> delta;
};

ALIA_DEFINE_EVENT_CATEGORY(RENDER_CATEGORY, 0x12)
ALIA_DEFINE_EVENT_TYPE_CODE(RENDER_CATEGORY, RENDER_EVENT, 0)

struct render_event
{
    SkCanvas* canvas = nullptr;
};

} // namespace alia

#endif
