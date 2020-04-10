#include <alia/timing/timer.hpp>

namespace alia {

void
schedule_timer_event(
    dataless_context ctx, node_id id, millisecond_count trigger_time)
{
    auto& sys = get<system_tag>(ctx);
    sys.external->schedule_timer_event(
        make_routable_node_id(ctx, id), trigger_time);
}

void
start_timer(dataless_context ctx, node_id id, millisecond_count duration)
{
    auto now = ctx.get<timing_tag>().tick_counter;
    schedule_timer_event(ctx, id, now + duration);
}

bool
detect_timer_event(dataless_context ctx, node_id id)
{
    timer_event* event;
    return detect_targeted_event(ctx, id, &event);
}

void
restart_timer(dataless_context ctx, node_id id, millisecond_count duration)
{
    timer_event* event;
    assert(detect_event(ctx, &event));
    schedule_timer_event(ctx, id, event->trigger_time + duration);
}

void
raw_timer::update()
{
    triggered_ = data_->active && detect_timer_event(ctx_, data_);
    if (triggered_)
        data_->active = false;
}

void
raw_timer::start(unsigned duration)
{
    if (triggered_)
        restart_timer(ctx_, data_, duration);
    else
        start_timer(ctx_, data_, duration);
    data_->active = true;
}

} // namespace alia
