#pragma once

#include <alia/context.hpp>
#include <alia/events.hpp>
#include <alia/ids.hpp>

namespace alia {

void
invoke_controller(ui_system& sys, event_traversal& events);

namespace detail {

// Set up the event traversal so that it will route the control flow to the
// given target. (And also invoke the traversal.)
//
// :target can be null, in which case the event will be routed through the
// entire component tree.
//
void
route_event(
    ui_system& sys, event_traversal& traversal, component_container* target);

inline void
dispatch_targeted_event(
    ui_system& sys, alia_event& event, component_identity const& target)
{
    // `target` is a weak_ptr, so we have to acquire a lock on it. It's
    // possible that the target has actually been destroyed and we're trying to
    // deliver an event to an non-existent target, in which case the lock will
    // fail.
    if (auto target_container = target.lock())
    {
        event_traversal traversal;
        traversal.targeted = true;
        traversal.event = &event;
        route_event(sys, traversal, target_container.get());
    }
}

inline void
dispatch_untargeted_event(ui_system& sys, alia_event& event)
{
    event_traversal traversal;
    traversal.targeted = false;
    traversal.event = &event;
    route_event(sys, traversal, nullptr);
}

} // namespace detail

inline void
dispatch_event(ui_system& sys, alia_event& event)
{
    detail::dispatch_untargeted_event(sys, event);
}

inline void
dispatch_targeted_event(
    ui_system& sys, alia_event& event, external_component_id component)
{
    detail::dispatch_targeted_event(sys, event, component.identity);
}

// TODO: Sort all this out...

inline void
dispatch_targeted_event(
    ui_system& sys, alia_event& event, routable_widget_id target)
{
    detail::dispatch_targeted_event(sys, event, target.component);
}

} // namespace alia
