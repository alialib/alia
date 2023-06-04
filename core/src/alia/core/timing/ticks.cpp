#include <alia/core/timing/ticks.hpp>

#include <alia/core/context/interface.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/core/system/internals.hpp>

namespace alia {

void
schedule_animation_refresh(dataless_context ctx)
{
    // Invoke the virtual method on the external system interface.
    // And also set a flag to indicate that a refresh is needed.
    system& sys = get<system_tag>(ctx);
    if (!sys.refresh_needed)
    {
        if (sys.external)
            sys.external->schedule_animation_refresh();
        sys.refresh_needed = true;
    }
    // Ensure that this component gets visited on the next refresh pass.
    mark_animating_component(ctx);
}

millisecond_count
get_raw_animation_tick_count(dataless_context ctx)
{
    schedule_animation_refresh(ctx);
    return get<timing_tag>(ctx).tick_counter;
}

value_signal<millisecond_count>
get_animation_tick_count(dataless_context ctx)
{
    return value(get_raw_animation_tick_count(ctx));
}

millisecond_count
get_raw_animation_ticks_left(dataless_context ctx, millisecond_count end_time)
{
    int ticks_remaining = int(end_time - get<timing_tag>(ctx).tick_counter);
    if (ticks_remaining > 0)
    {
        if (is_refresh_event(ctx))
            schedule_animation_refresh(ctx);
        return millisecond_count(ticks_remaining);
    }
    return 0;
}

} // namespace alia
