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

// general categories of UI events recognized by alia
// (This can be useful as a primary dispatching criteria in widget
// implementations.)
enum class ui_event_category
{
    REGION,
    INPUT,
    RENDER
};

enum class ui_event_type
{
    // rendering
    RENDER,

    // regions
    MAKE_WIDGET_VISIBLE,
    MOUSE_HIT_TEST,
    WHEEL_HIT_TEST,

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
};

struct ui_event : targeted_event
{
    ui_event_category category;
    ui_event_type type;
};

// MOUSE_PRESS, DOUBLE_CLICK, and MOUSE_RELEASE
struct mouse_button_event : ui_event
{
    mouse_button button;
};

// MOUSE_MOTION
struct mouse_motion_event : ui_event
{
    vector<2, double> position;
};

// MOUSE_GAIN, MOUSE_LOSS, and MOUSE_HOVER
struct mouse_notification_event : ui_event
{
};

// SCROLL
struct scroll_event : ui_event
{
    vector<2, double> delta;
};

// TEXT_INPUT
struct text_ui_event : ui_event
{
    std::string text;
};

// KEY_PRESS and KEY_RELEASE
struct key_event : ui_event
{
    modded_key key;
    bool acknowledged = false;
};

// FOCUS_GAIN and FOCUS_LOSS
struct focus_notification_event : ui_event
{
};

// FOCUS_SUCCESSOR
struct focus_successor_event : ui_event
{
    internal_element_id target;
    internal_element_id successor;
    bool just_saw_target = false;
};

// FOCUS_PREDECESSOR
struct focus_predecessor_event : ui_event
{
    internal_element_id target;
    internal_element_id predecessor;
    bool saw_target = false;
};

struct hit_test_event : ui_event
{
    // the point to test
    vector<2, double> point;
};

struct mouse_hit_test_result
{
    external_element_id element;
    mouse_cursor cursor;
    layout_box hit_box;
    std::string tooltip_message;
};

// MOUSE_HIT_TEST
struct mouse_hit_test_event : hit_test_event
{
    std::optional<mouse_hit_test_result> result;

    mouse_hit_test_event(vector<2, double> point)
        : hit_test_event{
              {ui_event_category::REGION, ui_event_type::MOUSE_HIT_TEST},
              point}
    {
    }
};

// WHEEL_HIT_TEST
struct wheel_hit_test_event : hit_test_event
{
    std::optional<external_element_id> result;

    wheel_hit_test_event(vector<2, double> point)
        : hit_test_event{
              {ui_event_category::REGION, ui_event_type::WHEEL_HIT_TEST},
              point}
    {
    }
};

struct render_event
{
    SkCanvas* canvas = nullptr;
};

} // namespace alia

#endif
