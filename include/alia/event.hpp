#ifndef ALIA_EVENT_HPP
#define ALIA_EVENT_HPP

#include <alia/point.hpp>
#include <list>

namespace alia {

enum event_type
{
    // layout category

    REFRESH_EVENT,

    LAYOUT_PASS_0,
    LAYOUT_PASS_1,
    LAYOUT_PASS_2,

    // render category

    RENDER_EVENT,

    // region category

    HIT_TEST_EVENT,
    MAKE_REGION_VISIBLE,

    // input category

    FOCUS_GAIN_EVENT,
    FOCUS_LOSS_EVENT,

    FOCUS_PREDECESSOR_EVENT,
    FOCUS_SUCCESSOR_EVENT,

    JUMP_TO_LOCATION,
    MOVE_FOCUS_TO_LOCATION,

    MOUSE_MOTION_EVENT,

    BUTTON_DOWN_EVENT,
    BUTTON_UP_EVENT,
    DOUBLE_CLICK_EVENT,

    SCROLL_WHEEL_EVENT,

    CHAR_EVENT,
    KEY_DOWN_EVENT,
    KEY_UP_EVENT,

    TIMER_EVENT,

    // This is sent when the event loop is idle.
    IDLE_EVENT,

    // other category

    INIT_EVENT,

    SET_VALUE_EVENT,
    GET_CONTENTS_EVENT,

    WRAPPED_EVENT,

    // If you need to define custom events, you can specify this as the event
    // type and use dynamic casting to test for your specific event type.
    OTHER_EVENT,
};

enum event_category
{
    LAYOUT_CATEGORY,
    RENDER_CATEGORY,
    REGION_CATEGORY,
    INPUT_CATEGORY,
    OTHER_CATEGORY,
};

enum event_culling_type
{
    NO_CULLING,
    LAYOUT_CULLING,
    POINT_CULLING,
    SCREEN_CULLING,
    TARGETED_CULLING,
};

struct event
{
    event(event_category category, event_type type,
        event_culling_type culling_type)
      : category(category), type(type), culling_type(culling_type) {}
    virtual ~event() {}
    event_category category;
    event_type type;
    event_culling_type culling_type;
};

struct init_event : event
{
    init_event() : event(OTHER_CATEGORY, INIT_EVENT, NO_CULLING) {}
};

struct targeted_event : event
{
    targeted_event(event_category category, event_type type, region_id id)
      : event(category, type, TARGETED_CULLING)
      , target_id(id)
      , saw_target(false) {}
    region_id target_id;
    bool saw_target;
};

struct point_event : event
{
    point_event(event_category category, event_type type, point2i const& p)
      : event(category, type, POINT_CULLING)
      , p(p) {}
    point2i p, integer_untransformed_p;
    point2d untransformed_p;
};

}

#endif
