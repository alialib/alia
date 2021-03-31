#include <alia/flow/events.hpp>

#include <alia/system/internals.hpp>
#include <alia/timing/ticks.hpp>

namespace alia {

static void
invoke_controller(system& sys, event_traversal& events)
{
    events.is_refresh = (events.event_type == &typeid(refresh_event));

    data_traversal data;
    scoped_data_traversal sdt(sys.data, data);
    // Only use refresh events to decide when data is no longer needed.
    data.gc_enabled = data.cache_clearing_enabled = events.is_refresh;

    timing_subsystem timing;
    timing.tick_counter = sys.external->get_tick_count();

    context_storage storage;
    context ctx = make_context(&storage, sys, events, data, timing);

    scoped_component_container root(ctx, &sys.root_component);

    sys.controller(ctx);
}

namespace detail {

static void
route_event_(
    system& sys, event_traversal& traversal, component_container* target)
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

void
route_event(
    system& sys, event_traversal& traversal, component_container* target)
{
    try
    {
        route_event_(sys, traversal, target);
    }
    catch (traversal_abortion&)
    {
    }
}

} // namespace detail

void
abort_traversal(dataless_context ctx)
{
    assert(!is_refresh_event(ctx));
    get_event_traversal(ctx).aborted = true;
    throw traversal_abortion();
}

void
refresh_component_identity(dataless_context ctx, component_identity& identity)
{
    auto const& active_container = get_active_component_container(ctx);
    // Only update to the active container if it's actually different.
    if (identity.owner_before(active_container)
        || active_container.owner_before(identity))
    {
        identity = active_container;
    }
}

component_id
get_component_id(context ctx)
{
    component_id id;
    get_cached_data(ctx, &id);
    refresh_handler(
        ctx, [&](auto ctx) { refresh_component_identity(ctx, *id); });
    return id;
}

struct initialization_detection
{
    bool initialized = false;
};

void
on_init(context ctx, action<> on_init)
{
    initialization_detection& data = get_data<initialization_detection>(ctx);
    refresh_handler(ctx, [&](auto) {
        if (!data.initialized && on_init.is_ready())
        {
            isolate_errors(ctx, [&] { perform_action(on_init); });
            mark_dirty_component(ctx);
            data.initialized = true;
        }
    });
}

void
on_activate(context ctx, action<> on_activate)
{
    initialization_detection& data
        = get_cached_data<initialization_detection>(ctx);
    refresh_handler(ctx, [&](auto) {
        if (!data.initialized && on_activate.is_ready())
        {
            isolate_errors(ctx, [&] { perform_action(on_activate); });
            mark_dirty_component(ctx);
            data.initialized = true;
        }
    });
}

void
isolate_errors(system& sys, function_view<void()> const& function)
{
    try
    {
        function();
    }
    catch (...)
    {
        if (sys.error_handler)
            sys.error_handler(std::current_exception());
    }
}

} // namespace alia
