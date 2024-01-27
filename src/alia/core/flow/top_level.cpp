#include <alia/core/flow/top_level.hpp>

#include <alia/core/system/internals.hpp>

namespace alia {

void
invoke_controller(untyped_system& sys, event_traversal& events)
{
    events.is_refresh = (events.event_type == &typeid(refresh_event));

    data_traversal data;
    scoped_data_traversal sdt(sys.data, data);
    // Only use refresh events to decide when data is no longer needed.
    data.gc_enabled = data.cache_clearing_enabled = events.is_refresh;

    timing_subsystem timing;
    timing.tick_counter = sys.external->get_tick_count();

    sys.create_core_context_and_invoke_controller(events, data, timing);
}

namespace {

void
route_event_(
    untyped_system& sys,
    event_traversal& traversal,
    component_container* target)
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

} // namespace

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
    component_container* target)
{
    try
    {
        route_event_(sys, traversal, target);
    }
    catch (traversal_aborted&)
    {
    }
}

} // namespace detail

} // namespace alia
