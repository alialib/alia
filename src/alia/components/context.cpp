#include <alia/components/context.hpp>

#include <alia/components/system.hpp>
#include <alia/flow/data_graph.hpp>
#include <alia/flow/events.hpp>

namespace alia {

context
make_context(
    context_component_storage* storage,
    system* sys,
    event_traversal* event,
    data_traversal* data)
{
    return add_component<data_traversal_tag>(
        add_component<event_traversal_tag>(
            add_component<system_tag>(
                make_empty_component_collection(storage), sys),
            event),
        data);
}

void
request_animation_refresh(dataless_context ctx)
{
    // invoke virtual method on system interface
    // (Also set a flag indicating that a refresh is needed.)
    system& sys = *get_component<system_tag>(ctx);
    if (!sys.refresh_needed)
    {
        if (sys.external)
            sys.external->request_animation_refresh();
        sys.refresh_needed = true;
    }
}

millisecond_count
get_raw_animation_tick_count(dataless_context ctx)
{
    request_animation_refresh(ctx);
    return get_component<system_tag>(ctx)->tick_counter;
}

value_signal<millisecond_count>
get_animation_tick_count(dataless_context ctx)
{
    return val(get_raw_animation_tick_count(ctx));
}

millisecond_count
get_raw_animation_ticks_left(dataless_context ctx, millisecond_count end_time)
{
    int ticks_remaining
        = int(end_time - get_component<system_tag>(ctx)->tick_counter);
    if (ticks_remaining > 0)
    {
        if (is_refresh_event(ctx))
            request_animation_refresh(ctx);
        return millisecond_count(ticks_remaining);
    }
    return 0;
}

} // namespace alia
