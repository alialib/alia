#ifndef ALIA_CORE_FLOW_TOP_LEVEL_HPP
#define ALIA_CORE_FLOW_TOP_LEVEL_HPP

#include <alia/core/flow/events.hpp>
#include <alia/core/system/interface.hpp>

namespace alia {

void
invoke_controller(untyped_system& sys, event_traversal& events);

namespace detail {

// Set up the event traversal so that it will route the control flow to the
// given target. (And also invoke the traversal.)
//
// :target can be null, in which case the event will be routed through the
// entire component tree.
//
void
route_event(
    untyped_system& sys,
    event_traversal& traversal,
    component_container* target);

template<class Event>
void
dispatch_targeted_event(
    untyped_system& sys,
    Event& event,
    component_identity const& target,
    event_type_code type_code = 0)
{
    // `target` is a weak_ptr, so we have to acquire a lock on it. It's
    // possible that the target has actually been destroyed and we're trying to
    // deliver an event to an non-existent target, in which case the lock will
    // fail.
    if (auto target_container = target.lock())
    {
        event_traversal traversal;
        traversal.targeted = true;
        traversal.type_code = type_code;
        traversal.event_type = &typeid(Event);
        traversal.event = &event;
        route_event(sys, traversal, target_container.get());
    }
}

template<class Event>
void
dispatch_untargeted_event(
    untyped_system& sys, Event& event, event_type_code type_code = 0)
{
    event_traversal traversal;
    traversal.targeted = false;
    traversal.type_code = type_code;
    traversal.event_type = &typeid(Event);
    traversal.event = &event;
    route_event(sys, traversal, nullptr);
}

} // namespace detail

template<class Event>
void
dispatch_event(
    untyped_system& sys, Event& event, event_type_code type_code = 0)
{
    detail::dispatch_untargeted_event(sys, event, type_code);
}

template<class Event>
void
dispatch_targeted_event(
    untyped_system& sys,
    Event& event,
    external_component_id component,
    event_type_code type_code = 0)
{
    event.target_id = component.id;
    detail::dispatch_targeted_event(sys, event, component.identity, type_code);
}

} // namespace alia

#endif
