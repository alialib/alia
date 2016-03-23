#include <alia/ui/utilities/timing.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

void request_refresh(dataless_ui_context& ctx, ui_time_type duration)
{
    ui_system& ui = *ctx.system;
    ui_time_type update_time = ui.millisecond_tick_count + duration;
    if (!ui.next_update || int(get(ui.next_update) - update_time) > 0)
        ui.next_update = update_time;
    record_content_change(ctx);
}

void request_animation_refresh(dataless_ui_context& ctx)
{
    request_refresh(ctx, 1);
}

ui_time_type get_animation_tick_count(dataless_ui_context& ctx)
{
    if (is_refresh_pass(ctx))
        request_animation_refresh(ctx);
    return ctx.system->millisecond_tick_count;
}

ui_time_type
get_animation_ticks_left(dataless_ui_context& ctx, ui_time_type end_time)
{
    int ticks_remaining = int(end_time - ctx.system->millisecond_tick_count);
    if (ticks_remaining > 0)
    {
        if (is_refresh_pass(ctx))
            request_animation_refresh(ctx);
        return ui_time_type(ticks_remaining);
    }
    return 0;
}

struct square_wave_data
{
    widget_identity id;
    bool value;
};

bool square_wave(ui_context& ctx, ui_time_type true_duration,
    ui_time_type false_duration)
{
    square_wave_data* data;
    if (get_cached_data(ctx, &data))
    {
        data->value = true;
        start_timer(ctx, &data->id, true_duration);
    }
    if (detect_timer_event(ctx, &data->id))
    {
        data->value = !data->value;
        restart_timer(ctx, &data->id,
            (data->value || !false_duration) ? true_duration : false_duration);
    }
    return data->value;
}

void request_timer_event(
    dataless_ui_context& ctx, widget_id id, ui_time_type time)
{
    ui_system& ui = *ctx.system;
    // If an event already exists for that ID, then reschedule it.
    for (ui_timer_request_list::iterator i = ui.timer_requests.begin();
        i != ui.timer_requests.end(); ++i)
    {
        if (i->id.id == id)
        {
            i->id = make_routable_widget_id(ctx, id);
            i->trigger_time = time;
            i->frame_issued = ui.timer_event_counter;
            return;
        }
    }
    // Otherwise, add a new event.
    ui_timer_request rq;
    rq.id = make_routable_widget_id(ctx, id);
    rq.trigger_time = time;
    rq.frame_issued = ui.timer_event_counter;
    ui.timer_requests.push_back(rq);
}

void start_timer(dataless_ui_context& ctx, widget_id id, unsigned duration)
{
    input_event* ie = dynamic_cast<input_event*>(ctx.event);
    ui_time_type now = ie ? ie->time : ctx.system->millisecond_tick_count;
    request_timer_event(ctx, id, now + duration);
}

bool detect_timer_event(dataless_ui_context& ctx, widget_id id)
{
    return ctx.event->type == TIMER_EVENT &&
        get_event<timer_event>(ctx).id == id;
}

void restart_timer(dataless_ui_context& ctx, widget_id id, unsigned duration)
{
    timer_event& e = get_event<timer_event>(ctx);
    request_timer_event(ctx, id, e.trigger_time + duration);
}

timer::timer(ui_context& ctx, timer_data* data)
{
    if (data)
        data_ = data;
    else
        get_cached_data(ctx, &data_);
    ctx_ = &ctx;
    triggered_ = data_->active && detect_timer_event(ctx, &data_->id);
    if (triggered_)
        data_->active = false;
}
void timer::start(unsigned duration)
{
    if (triggered_)
        restart_timer(*ctx_, &data_->id, duration);
    else
        start_timer(*ctx_, &data_->id, duration);
    data_->active = true;
}
void timer::stop()
{
    data_->active = false;
}

}
