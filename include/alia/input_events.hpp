#ifndef ALIA_INPUT_EVENTS_HPP
#define ALIA_INPUT_EVENTS_HPP

#include <alia/point.hpp>
#include <alia/input_defs.hpp>
#include <alia/event.hpp>
#include <alia/box.hpp>

namespace alia {

struct targeted_input_event : targeted_event
{
    targeted_input_event(event_type type, region_id id)
      : targeted_event(INPUT_CATEGORY, type, id)
      , processed(false)
    {}

    // When a widget processes an event, it will set this flag.
    bool processed;

    // Whoever processes the event can set this flag if the event hasn't caused
    // any changes that would require a refresh.  Note that this is only
    // relevant if the event is processed.  If it's not processed, there's no
    // refresh anyway.
    //bool skip_refresh;
};

struct hit_test_event : point_event
{
    hit_test_event(point2i const& p)
      : point_event(REGION_CATEGORY, HIT_TEST_EVENT, p) {}
    region_id id;
};
region_id do_hit_test(context& ctx, point2i const& position);
void update_hot_id(context& ctx);

struct mouse_motion_event : targeted_input_event
{
    mouse_motion_event(region_id id, point2i const& old_p,
        point2i const& new_p)
      : targeted_input_event(MOUSE_MOTION_EVENT, id)
      , old_position(old_p)
      , new_position(new_p)
    {}
    point2i old_position;
    point2i new_position;
};
void clear_mouse_position(context& ctx);
void set_mouse_position(context& ctx, point2i const& position);
vector2d get_drag_delta(context& ctx, mouse_motion_event& e);

struct mouse_button_event : targeted_input_event
{
    mouse_button_event(event_type type, mouse_button button, region_id id)
      : targeted_input_event(type, id)
      , button(button)
    {}
    mouse_button button;
};
void process_mouse_press(context& ctx, mouse_button button);
void process_mouse_release(context& ctx, mouse_button button);
void process_double_click(context& ctx, mouse_button button);

struct scroll_wheel_event : targeted_input_event
{
    scroll_wheel_event(int amount, region_id id)
      : targeted_input_event(SCROLL_WHEEL_EVENT, id)
      , amount(amount)
    {}
    int amount;
};
void process_scroll_wheel_movement(context& ctx, int amount);

struct key_event : targeted_input_event
{
    key_event(event_type type, key_event_info const& info, region_id id)
      : targeted_input_event(type, id)
      , info(info)
    {}
    key_event_info info;
};

struct char_event : targeted_input_event
{
    char_event(int character, region_id id)
      : targeted_input_event(CHAR_EVENT, id)
      , character(character)
    {}
    int character;
};

struct focus_gain_event : targeted_event
{
    focus_gain_event(region_id id)
      : targeted_event(INPUT_CATEGORY, FOCUS_GAIN_EVENT, id) {}
};

struct focus_loss_event : targeted_event
{
    focus_loss_event(region_id id)
      : targeted_event(INPUT_CATEGORY, FOCUS_LOSS_EVENT, id) {}
};

struct focus_predecessor_event : event
{
    focus_predecessor_event()
      : event(INPUT_CATEGORY, FOCUS_PREDECESSOR_EVENT, NO_CULLING)
      , id(0)
      , saw_focus(false)
    {}
    region_id id;
    bool saw_focus;
};

struct focus_successor_event : event
{
    focus_successor_event()
      : event(INPUT_CATEGORY, FOCUS_SUCCESSOR_EVENT, NO_CULLING)
      , id(0)
      , just_saw_focus(true)
    {}
    region_id id;
    bool just_saw_focus;
};

struct make_region_visible_event : targeted_event
{
    make_region_visible_event(region_id id)
      : targeted_event(REGION_CATEGORY, MAKE_REGION_VISIBLE, id)
    {}
    box2i region;
};

struct timer_event : targeted_input_event
{
    timer_event(region_id id, unsigned time_requested, unsigned duration,
        unsigned now)
      : targeted_input_event(TIMER_EVENT, id)
      , time_requested(time_requested)
      , duration(duration)
      , now(now)
    {}
    unsigned time_requested, duration, now;
};

struct idle_event : event
{
    idle_event()
      : event(INPUT_CATEGORY, IDLE_EVENT, NO_CULLING)
      , request_more(false)
      , refresh(false)
    {}
    bool request_more, refresh;
};

}

#endif
