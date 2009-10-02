#include <alia/timer.hpp>
#include <alia/context.hpp>
#include <alia/input_events.hpp>
#include <alia/surface.hpp>

namespace alia {

void start_timer(context& ctx, region_id id, unsigned duration)
{
    ctx.surface->request_timer_event(id, duration);
}

bool is_timer_done(context& ctx, region_id id)
{
    return ctx.event->type == TIMER_EVENT &&
        get_event<timer_event>(ctx).target_id == id;
}

void restart_timer(context& ctx, region_id id, unsigned duration)
{
    timer_event& e = get_event<timer_event>(ctx);
    start_timer(ctx, id, duration - (e.now - (e.time_requested + e.duration)));
}

}
