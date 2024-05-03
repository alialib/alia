#ifndef ALIA_UI_EVENTS_HPP
#define ALIA_UI_EVENTS_HPP

#include <alia/core/flow/events.hpp>
#include <alia/core/timing/ticks.hpp>
#include <alia/ui/common.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/system/input_constants.hpp>

class SkCanvas;

namespace alia {

struct internal_element_id
{
    component_id component;
    int index;

    explicit
    operator bool() const
    {
        return component ? true : false;
    }

    auto
    operator<=>(internal_element_id const&) const
        = default;
};

struct external_element_id
{
    external_component_id component;
    int index;

    explicit
    operator bool() const
    {
        return component ? true : false;
    }

    bool
    matches(internal_element_id internal) const noexcept
    {
        return this->component.id == internal.component
               && this->index == internal.index;
    }
};

inline bool
operator==(external_element_id const& a, external_element_id const& b)
{
    return a.component.id == b.component.id && a.index == b.index;
}
inline bool
operator!=(external_element_id const& a, external_element_id const& b)
{
    return !(a == b);
}

inline external_element_id
externalize(internal_element_id element)
{
    return external_element_id{externalize(element.component), element.index};
}

ALIA_DEFINE_EVENT_CATEGORY(REGION_CATEGORY, 0x10)

ALIA_DEFINE_EVENT_TYPE_CODE(REGION_CATEGORY, MAKE_WIDGET_VISIBLE_EVENT, 0)

ALIA_DEFINE_EVENT_TYPE_CODE(REGION_CATEGORY, MOUSE_HIT_TEST_EVENT, 1)

struct mouse_hit_test_result
{
    external_element_id element;
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

// WHEEL_HIT_TEST
struct wheel_hit_test_event
{
    // the point to test
    vector<2, double> point;

    std::optional<external_element_id> result;
};

ALIA_DEFINE_EVENT_CATEGORY(INPUT_CATEGORY, 0x11)

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, TEXT_INPUT_EVENT, 0x00)

struct text_input_event : targeted_event
{
    std::string text;
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, KEY_PRESS_EVENT, 0x01)
ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, KEY_RELEASE_EVENT, 0x02)

struct key_event : targeted_event
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

struct focus_notification_event : targeted_event
{
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, FOCUS_SUCCESSOR_EVENT, 0x12)

struct focus_successor_event : targeted_event
{
    internal_element_id target;
    internal_element_id successor;
    bool just_saw_target = false;
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, FOCUS_PREDECESSOR_EVENT, 0x13)

struct focus_predecessor_event : targeted_event
{
    internal_element_id target;
    internal_element_id predecessor;
    bool saw_target = false;
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, MOUSE_PRESS_EVENT, 0x20)
ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, DOUBLE_CLICK_EVENT, 0x21)
ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, MOUSE_RELEASE_EVENT, 0x22)

struct mouse_button_event : targeted_event
{
    mouse_button button;
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, MOUSE_MOTION_EVENT, 0x23)

struct mouse_motion_event : targeted_event
{
    vector<2, double> position;
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, MOUSE_GAIN_EVENT, 0x24)
ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, MOUSE_LOSS_EVENT, 0x25)
ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, MOUSE_HOVER_EVENT, 0x26)

struct mouse_notification_event : targeted_event
{
};

ALIA_DEFINE_EVENT_TYPE_CODE(INPUT_CATEGORY, SCROLL_EVENT, 0x30)

struct scroll_event : targeted_event
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
