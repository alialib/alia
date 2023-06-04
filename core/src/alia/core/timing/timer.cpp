#include <alia/core/timing/timer.hpp>

namespace alia {

void
schedule_timer_event(
    dataless_context ctx,
    external_component_id id,
    millisecond_count trigger_time)
{
    auto& sys = get<system_tag>(ctx);
    sys.external->schedule_timer_event(id, trigger_time);
}

bool
detect_timer_event(dataless_context ctx, timer_data& data)
{
    timer_event* event;
    return detect_targeted_event(ctx, &data.identity, &event)
           && event->trigger_time == data.expected_trigger_time;
}

void
start_timer(dataless_context ctx, timer_data& data, millisecond_count duration)
{
    auto now = get<timing_tag>(ctx).tick_counter;
    auto trigger_time = now + duration;
    data.expected_trigger_time = trigger_time;
    schedule_timer_event(ctx, externalize(&data.identity), trigger_time);
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
        schedule_timer_event(ctx, externalize(&data.identity), trigger_time);
    }
}

void
timer::update()
{
    triggered_ = data_->active && detect_timer_event(ctx_, *data_);
    if (triggered_)
        data_->active = false;
    refresh_handler(ctx_, [&](auto ctx) {
        refresh_component_identity(ctx, data_->identity);
    });
}

void
timer::start(unsigned duration)
{
    if (triggered_)
        restart_timer(ctx_, *data_, duration);
    else
        start_timer(ctx_, *data_, duration);
    data_->active = true;
}

} // namespace alia
