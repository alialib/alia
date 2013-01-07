#ifndef ALIA_UI_UTILITIES_MISCELLANY_HPP
#define ALIA_UI_UTILITIES_MISCELLANY_HPP

#include <alia/ui/internals.hpp>
#include <alia/layout/utilities.hpp>
#include <alia/ui/utilities/rendering.hpp>
#include <alia/ui/utilities/keyboard.hpp>

namespace alia {

static inline counter_type
get_refresh_counter(ui_context& ctx)
{ return get_layout_traversal(ctx).refresh_counter; }

void record_content_change(ui_context& ctx);

// A simple macro that declares a reference to cached data and retrieves it.
#define ALIA_GET_CACHED_DATA(T) \
    T* alia__data_ptr; \
    get_cached_data(ctx, &alia__data_ptr); \
    T& data = *alia__data_ptr;

template<class Event>
bool detect_event(ui_context& ctx)
{
    return dynamic_cast<Event*>(ctx.event) != 0;
}

static inline bool detect_event(ui_context& ctx, ui_event_type type)
{
    return ctx.event->type == type;
}

template<class Event>
Event& get_event(ui_context& ctx)
{
    assert(detect_event<Event>(ctx));
    return static_cast<Event&>(*ctx.event);
}

widget_state get_widget_state(ui_context& ctx, widget_id id,
    bool enabled = true, bool pressed = false, bool selected = false);

routable_widget_id make_routable_widget_id(ui_context& ctx, widget_id id);

widget_id get_widget_id(ui_context& ctx);

static inline void get_widget_id_if_needed(ui_context& ctx, widget_id& id)
{
    if (!id)
        id = get_widget_id(ctx);
}

static inline void init_optional_widget_id(
    ui_context& ctx, widget_id& id, widget_id fallback)
{
    if (!id)
        id = fallback;
}

template<class T>
void set_new_value(accessor<T> const& accessor, control_result& result,
    T const& new_value)
{
    set(accessor, new_value);
    result.changed = true;
}

struct simple_display_data
{
    layout_leaf layout_node;
    caching_renderer_data rendering;
};

// The following data structure and accompanying functions implement input
// handling for simple, clickable widgets.

struct button_input_state
{
    keyboard_click_state key;
};

widget_state get_button_state(ui_context& ctx, widget_id id,
    button_input_state const& state);

// Do input processing for the button.
// This returns true iff the button was just pressed.
bool do_button_input(ui_context& ctx, widget_id id, button_input_state& state);

// VALUE EVENTS - These are used to communicate value changes through the UI
// traversal to a widget.

struct set_value_event : ui_event
{
    set_value_event(widget_id target, untyped_ui_value* value)
      : ui_event(NO_CATEGORY, SET_VALUE_EVENT)
      , value(value), target(target) {}
    alia__shared_ptr<untyped_ui_value> value;
    widget_id target;
};

template<class T>
void handle_set_value_events(ui_context& ctx, widget_id id,
    accessor<T> const& accessor)
{
    if (detect_event(ctx, SET_VALUE_EVENT))
    {
        set_value_event& e = get_event<set_value_event>(ctx);
        if (e.target == id)
        {
            // A dynamic_cast isn't really necessary here, but it's possible
            // that some bug coudlc ause an event could get sent with the wrong
            // type or to the wrong ID, and since this gets executed so
            // infrequently, it's better to just be safe.
            typed_ui_value<T>* typed_value =
                dynamic_cast<typed_ui_value<T>*>(e.value.get());
            assert(typed_value);
            if (typed_value)
                set(accessor, typed_value->value);
        }
    }
}

void issue_targeted_event(ui_system& system, ui_event& event,
    routable_widget_id const& target);

template<class T>
void issue_set_value_event(ui_context& ctx, widget_id id, T const& new_value)
{
    typed_ui_value<T>* value = new typed_ui_value<T>;
    value->value = new_value;
    set_value_event e(id, value);
    issue_targeted_event(*ctx.system, e, make_routable_widget_id(ctx, id));
}

// OVERLAYS

static inline bool is_overlay_active(ui_context& ctx, widget_id id)
{ return ctx.system->overlay_id.id == id; }

void set_active_overlay(ui_context& ctx, widget_id id);

}

#endif
