#include <alia/core/flow/events.hpp>

#include <alia/core/system/internals.hpp>
#include <alia/core/timing/ticks.hpp>

namespace alia {

void
abort_traversal(dataless_context ctx)
{
    assert(!is_refresh_event(ctx));
    get_event_traversal(ctx).aborted = true;
    throw traversal_aborted();
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
isolate_errors(untyped_system& sys, function_view<void()> const& function)
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
