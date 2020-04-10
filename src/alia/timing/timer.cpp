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

bool
detect_timer_event(dataless_context ctx, timer_data& data)
{
    timer_event* event;
    return detect_targeted_event(ctx, &data, &event)
           && event->trigger_time == data.expected_trigger_time;
}

void
start_timer(dataless_context ctx, timer_data& data, millisecond_count duration)
{
    auto now = ctx.get<timing_tag>().tick_counter;
    auto trigger_time = now + duration;
    data.expected_trigger_time = trigger_time;
    schedule_timer_event(ctx, &data, trigger_time);
}

void
restart_timer(
    dataless_context ctx, timer_data& data, millisecond_count duration)
{
    timer_event* event;
    bool detected = detect_event(ctx, &event);
    assert(detected);
    if (detected)
    {
        auto trigger_time = event->trigger_time + duration;
        data.expected_trigger_time = trigger_time;
        schedule_timer_event(ctx, &data, trigger_time);
    }
}

void
raw_timer::update()
{
    triggered_ = data_->active && detect_timer_event(ctx_, *data_);
    if (triggered_)
        data_->active = false;
}

void
raw_timer::start(unsigned duration)
{
    if (triggered_)
        restart_timer(ctx_, *data_, duration);
    else
        start_timer(ctx_, *data_, duration);
    data_->active = true;
}

} // namespace alia
