#ifndef ALIA_CORE_FLOW_TOP_LEVEL_HPP
#define ALIA_CORE_FLOW_TOP_LEVEL_HPP

#include <alia/core/flow/events.hpp>

namespace alia {

// TODO: Merge this in with system functions.

template<class System>
void
invoke_controller(System& sys, event_traversal& events)
{
    events.is_refresh = (events.event_type == &typeid(refresh_event));

    data_traversal data;
    scoped_data_traversal sdt(sys.data, data);
    // Only use refresh events to decide when data is no longer needed.
    data.gc_enabled = data.cache_clearing_enabled = events.is_refresh;

    timing_subsystem timing;
    timing.tick_counter = sys.external->get_tick_count();

    typename System::context_type::contents_type::storage_type storage;
    auto ctx = make_context(&storage, sys, events, data, timing);

    scoped_component_container root(ctx, &sys.root_component);

    sys.invoke_controller(ctx);
}

namespace detail {

template<class System>
void
route_event_(
    System& sys, event_traversal& traversal, component_container* target)
{
    // In order to construct the path to the target, we start at the target
    // and follow the 'parent' pointers until we reach the root. We do this
    // via recursion so that the path can be constructed entirely on the
    // stack.
    if (target)
    {
        event_routing_path path_node;
        path_node.rest = traversal.path_to_target;
        path_node.node = target;
        traversal.path_to_target = &path_node;
        route_event_(sys, traversal, target->parent.get());
    }
    else
    {
        invoke_controller(sys, traversal);
    }
}

// Set up the event traversal so that it will route the control flow to the
// given target. (And also invoke the traversal.)
//
// :target can be null, in which case the event will be routed through the
// entire component tree.
//
template<class System>
void
route_event(
    System& sys, event_traversal& traversal, component_container* target)
{
    try
    {
        route_event_(sys, traversal, target);
    }
    catch (traversal_aborted&)
    {
    }
}

template<class System, class Event>
void
dispatch_targeted_event(
    System& sys, Event& event, component_identity const& target)
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

template<class System, class Event>
void
dispatch_untargeted_event(System& sys, Event& event)
{
    event_traversal traversal;
    traversal.targeted = false;
    traversal.event_type = &typeid(Event);
    traversal.event = &event;
    route_event(sys, traversal, nullptr);
}

} // namespace detail

template<class System, class Event>
void
dispatch_event(System& sys, Event& event)
{
    detail::dispatch_untargeted_event(sys, event);
    sys.refresh();
}

template<class System, class Event>
void
dispatch_targeted_event(
    System& sys, Event& event, external_component_id component)
{
    event.target_id = component.id;
    detail::dispatch_targeted_event(sys, event, component.identity);
    sys.refresh();
}

} // namespace alia

#endif
