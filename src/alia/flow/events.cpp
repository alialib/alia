#include <alia/flow/events.hpp>

#include <alia/system/internals.hpp>
#include <alia/timing/ticks.hpp>

namespace alia {

void
mark_component_dirty(routing_region_ptr const& region)
{
    routing_region* r = region.get();
    while (r && !r->dirty)
    {
        r->dirty = true;
        r = r->parent.get();
    }
}

void
mark_component_dirty(dataless_context ctx)
{
    event_traversal& traversal = get_event_traversal(ctx);
    mark_component_dirty(*traversal.active_region);
}

void
scoped_routing_region::begin(context ctx)
{
    event_traversal& traversal = get_event_traversal(ctx);

    ctx_.reset(ctx);

    routing_region_ptr* region;
    if (get_data(ctx, &region))
        region->reset(new routing_region);
    region_ = region;

    if (traversal.active_region)
    {
        if ((*region)->parent != *traversal.active_region)
            (*region)->parent = *traversal.active_region;
    }
    else
        (*region)->parent.reset();

    parent_ = traversal.active_region;
    traversal.active_region = region;

    is_dirty_ = (*region)->dirty;

    if (traversal.targeted)
    {
        if (traversal.path_to_target
            && traversal.path_to_target->node == region->get())
        {
            traversal.path_to_target = traversal.path_to_target->rest;
            is_relevant_ = true;
        }
        else
            is_relevant_ = false;
    }
    else
        is_relevant_ = true;
}

void
scoped_routing_region::end()
{
    if (ctx_)
    {
        auto ctx = *ctx_;
        if (!traversal_aborted(ctx))
        {
            if (is_refresh_event(ctx))
                (*region_)->dirty = false;
            get_event_traversal(ctx).active_region = parent_;
        }
        ctx_.reset();
    }
}

static void
invoke_controller(system& sys, event_traversal& events)
{
    bool is_refresh = (events.event_type == &typeid(refresh_event));

    data_traversal data;
    scoped_data_traversal sdt(sys.data, data);
    // Only use refresh events to decide when data is no longer needed.
    data.gc_enabled = data.cache_clearing_enabled = is_refresh;

    timing_subsystem timing;
    timing.tick_counter = sys.external->get_tick_count();

    context_storage storage;
    context ctx = make_context(&storage, sys, events, data, timing);

    scoped_routing_region root(ctx);

    sys.controller(ctx);
}

namespace impl {

static void
route_event_(system& sys, event_traversal& traversal, routing_region* target)
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
route_event(system& sys, event_traversal& traversal, routing_region* target)
{
    try
    {
        route_event_(sys, traversal, target);
    }
    catch (traversal_abortion&)
    {
    }
}

} // namespace impl

void
abort_traversal(dataless_context ctx)
{
    get_event_traversal(ctx).aborted = true;
    throw traversal_abortion();
}

void
mark_refresh_incomplete(dataless_context ctx)
{
    assert(is_refresh_event(ctx));
    refresh_event* e = nullptr;
    detect_event(ctx, &e);
    e->incomplete = true;
}

void
refresh_component_identity(dataless_context ctx, component_identity& identity)
{
    auto const& active_region = get_active_routing_region(ctx);
    if (identity != active_region)
        identity = active_region;
}

component_id
get_component_id(context ctx)
{
    component_id id;
    get_cached_data(ctx, &id);
    on_refresh(ctx, [&](auto ctx) { refresh_component_identity(ctx, *id); });
    return id;
}

} // namespace alia
