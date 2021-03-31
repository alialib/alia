#ifndef ALIA_FLOW_EVENTS_HPP
#define ALIA_FLOW_EVENTS_HPP

#include <alia/flow/actions.hpp>
#include <alia/flow/components.hpp>
#include <alia/flow/data_graph.hpp>
#include <alia/flow/macros.hpp>
#include <alia/system/interface.hpp>

// This file implements utilities for routing events through an alia content
// traversal function.
//
// In alia, the application defines the contents of the scene dynamically by
// iterating through the current objects in the scene and calling a
// library-provided function to specify the existence of each of them.
//
// This traversal function serves as a way to dispatch and handle events.
// However, in cases where the event only needs to go to a single object in the
// scene, the overhead of having the traversal function visit every other
// object can be problematic. The overhead can be reduced by having the
// traversal function skip over subregions of the scene when they're not
// relevant to the event.
//
// This file provides a system for defining a hierarchy of such subregions in
// the scene, identifying which subregion an event is targeted at, and culling
// out irrelevant subregions.

namespace alia {

struct system;

struct event_routing_path
{
    component_container* node;
    event_routing_path* rest;
};

struct event_traversal
{
    component_container_ptr* active_container = nullptr;
    bool targeted;
    event_routing_path* path_to_target = nullptr;
    bool is_refresh;
    std::type_info const* event_type;
    void* event;
    bool aborted = false;
};

template<class Context>
component_container_ptr const&
get_active_component_container(Context ctx)
{
    return *get_event_traversal(ctx).active_container;
}

typedef component_container_ptr::weak_type component_identity;

namespace detail {

// Set up the event traversal so that it will route the control flow to the
// given target. (And also invoke the traversal.)
//
// :target can be null, in which case the event will be routed through the
// entire component tree.
//
void
route_event(
    system& sys, event_traversal& traversal, component_container* target);

template<class Event>
void
dispatch_targeted_event(
    system& sys, Event& event, component_identity const& target)
{
    // `target` is a weak_ptr, so we have to acquire a lock on it. It's
    // possible that the target has actually been destroyed and we're trying to
    // deliver an event to an non-existent target, in which case the lock will
    // fail.
    if (auto target_container = target.lock())
    {
        event_traversal traversal;
        traversal.targeted = true;
        traversal.event_type = &typeid(Event);
        traversal.event = &event;
        route_event(sys, traversal, target_container.get());
    }
}

template<class Event>
void
dispatch_event(system& sys, Event& event)
{
    event_traversal traversal;
    traversal.targeted = false;
    traversal.event_type = &typeid(Event);
    traversal.event = &event;
    route_event(sys, traversal, nullptr);
}

} // namespace detail

template<class Event>
void
dispatch_event(system& sys, Event& event)
{
    detail::dispatch_event(sys, event);
    refresh_system(sys);
}

struct traversal_abortion
{
};

void
abort_traversal(dataless_context ctx);

inline bool
traversal_aborted(dataless_context ctx)
{
    return get_event_traversal(ctx).aborted;
}

template<class Event>
bool
detect_event(dataless_context ctx, Event** event)
{
    event_traversal& traversal = get_event_traversal(ctx);
    if (*traversal.event_type == typeid(Event))
    {
        *event = reinterpret_cast<Event*>(traversal.event);
        return true;
    }
    return false;
}

template<class Event, class Context, class Handler>
void
event_handler(Context ctx, Handler&& handler)
{
    Event* e;
    ALIA_UNTRACKED_IF(detect_event(ctx, &e))
    {
        handler(ctx, *e);
    }
    ALIA_END
}

typedef component_identity* component_id;

void
refresh_component_identity(dataless_context ctx, component_identity& identity);

component_id
get_component_id(context ctx);

// external_component_id identifies a component in a form that can be safely
// stored outside of the alia data graph.
struct external_component_id
{
    component_id id = nullptr;
    component_identity identity;
};

static external_component_id const null_component_id;

// Is the given external_component_id valid?
// (Only the null_component_id is invalid.)
inline bool
is_valid(external_component_id const& id)
{
    return id.id != nullptr;
}

inline external_component_id
externalize(component_id id)
{
    external_component_id external;
    external.id = id;
    external.identity = *id;
    return external;
}

struct targeted_event
{
    component_id target_id;
};

template<class Event>
void
dispatch_targeted_event(
    system& sys, Event& event, external_component_id component)
{
    event.target_id = component.id;
    detail::dispatch_targeted_event(sys, event, component.identity);
    refresh_system(sys);
}

template<class Event>
bool
detect_targeted_event(dataless_context ctx, component_id id, Event** event)
{
    return detect_event(ctx, event) && (*event)->target_id == id;
}

template<class Event, class Context, class Handler>
void
targeted_event_handler(Context ctx, component_id id, Handler&& handler)
{
    Event* e;
    ALIA_UNTRACKED_IF(detect_targeted_event(ctx, id, &e))
    {
        handler(ctx, *e);
        abort_traversal(ctx);
    }
    ALIA_END
}

// the refresh event...

struct refresh_event
{
};

inline bool
is_refresh_event(dataless_context ctx)
{
    return get_event_traversal(ctx).is_refresh;
}

template<class Context, class Handler>
void
refresh_handler(Context ctx, Handler handler)
{
    ALIA_UNTRACKED_IF(is_refresh_event(ctx))
    {
        handler(ctx);
    }
    ALIA_END
}

void
isolate_errors(system& sys, function_view<void()> const& function);

template<class Context>
void
isolate_errors(Context ctx, function_view<void()> const& function)
{
    isolate_errors(get<system_tag>(ctx), function);
}

void
on_init(context ctx, action<> on_init);

void
on_activate(context ctx, action<> on_activate);

namespace detail {

template<class Value>
struct value_change_detection_data
{
    captured_id id;
    bool has_value;
    Value value;
};

template<class Value, class Signal>
void
common_value_change_logic(
    context ctx,
    value_change_detection_data<Value>& data,
    Signal const& signal,
    action<> on_change)
{
    refresh_signal_view(
        data.id,
        signal,
        [&](auto const& new_value) {
            if (!data.has_value || data.value != new_value)
            {
                isolate_errors(ctx, [&] { perform_action(on_change); });
                mark_dirty_component(ctx);
                data.has_value = true;
                data.value = new_value;
            }
        },
        [&] {
            if (data.has_value)
            {
                isolate_errors(ctx, [&] { perform_action(on_change); });
                mark_dirty_component(ctx);
                data.has_value = false;
            }
        });
}

} // namespace detail

template<class Signal>
void
on_value_change(context ctx, Signal const& signal, action<> on_change)
{
    detail::value_change_detection_data<typename Signal::value_type>* data;
    if (get_cached_data(ctx, &data))
    {
        isolate_errors(ctx, [&] { perform_action(on_change); });
        mark_dirty_component(ctx);
        data->has_value = signal_has_value(signal);
        if (data->has_value)
            data->value = read_signal(signal);
    }
    detail::common_value_change_logic(ctx, *data, signal, on_change);
}

template<class Signal>
void
on_observed_value_change(context ctx, Signal const& signal, action<> on_change)
{
    detail::value_change_detection_data<typename Signal::value_type>* data;
    if (get_cached_data(ctx, &data))
    {
        data->has_value = signal_has_value(signal);
        if (data->has_value)
            data->value = read_signal(signal);
    }
    detail::common_value_change_logic(ctx, *data, signal, on_change);
}

namespace detail {

template<bool TriggeringState, class Signal>
void
common_value_edge_logic(
    context ctx, bool* saved_state, Signal const& signal, action<> on_edge)
{
    if (is_refresh_event(ctx))
    {
        bool current_state = signal_has_value(signal);
        if (current_state == TriggeringState
            && *saved_state != TriggeringState)
        {
            isolate_errors(ctx, [&] { perform_action(on_edge); });
            mark_dirty_component(ctx);
        }
        *saved_state = current_state;
    }
}

} // namespace detail

template<class Signal>
void
on_value_gain(context ctx, Signal const& signal, action<> on_gain)
{
    bool* saved_state;
    if (get_cached_data(ctx, &saved_state))
        *saved_state = false;
    detail::common_value_edge_logic<true>(ctx, saved_state, signal, on_gain);
}

template<class Signal>
void
on_observed_value_gain(context ctx, Signal const& signal, action<> on_gain)
{
    bool* saved_state;
    if (get_cached_data(ctx, &saved_state))
        *saved_state = true;
    detail::common_value_edge_logic<true>(ctx, saved_state, signal, on_gain);
}

template<class Signal>
void
on_value_loss(context ctx, Signal const& signal, action<> on_loss)
{
    bool* saved_state;
    if (get_cached_data(ctx, &saved_state))
        *saved_state = true;
    detail::common_value_edge_logic<false>(ctx, saved_state, signal, on_loss);
}

template<class Signal>
void
on_observed_value_loss(context ctx, Signal const& signal, action<> on_loss)
{
    bool* saved_state;
    if (get_cached_data(ctx, &saved_state))
        *saved_state = false;
    detail::common_value_edge_logic<false>(ctx, saved_state, signal, on_loss);
}

} // namespace alia

#endif
