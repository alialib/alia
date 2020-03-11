#include <alia/components/timing.hpp>

#include <alia/context/interface.hpp>
#include <alia/flow/events.hpp>
#include <alia/system.hpp>

namespace alia {

void
request_animation_refresh(dataless_context ctx)
{
    // Invoke the virtual method on the external system interface.
    // And also set a flag to indicate that a refresh is needed.
    system& sys = ctx.get<system_tag>();
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
    return ctx.get<timing_tag>().tick_counter;
}

value_signal<millisecond_count>
get_animation_tick_count(dataless_context ctx)
{
    return value(get_raw_animation_tick_count(ctx));
}

millisecond_count
get_raw_animation_ticks_left(dataless_context ctx, millisecond_count end_time)
{
    int ticks_remaining = int(end_time - ctx.get<timing_tag>().tick_counter);
    if (ticks_remaining > 0)
    {
        if (is_refresh_event(ctx))
            request_animation_refresh(ctx);
        return millisecond_count(ticks_remaining);
    }
    return 0;
}

} // namespace alia
