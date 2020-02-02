#include <alia/components/system.hpp>

#include <chrono>

#include <alia/flow/events.hpp>

namespace alia {

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
    if (is_refresh_event(ctx))
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

namespace {

millisecond_count
get_tick_count_now()
{
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<
               std::chrono::duration<millisecond_count, std::milli>>(
               now - start)
        .count();
}

} // namespace

void
refresh_system(system& sys)
{
    sys.tick_counter = get_tick_count_now();

    sys.refresh_needed = false;

    refresh_event refresh;
    dispatch_event(sys, refresh);
}

} // namespace alia
