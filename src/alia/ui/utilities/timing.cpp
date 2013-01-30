#include <alia/ui/utilities/timing.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

void request_refresh(ui_context& ctx)
{
    ctx.surface->request_refresh(false);
    record_content_change(ctx);
}

ui_time_type get_animation_tick_count(ui_context& ctx)
{
    request_refresh(ctx);
    return ctx.system->millisecond_tick_count;
}

ui_time_type get_animation_ticks_left(ui_context& ctx, ui_time_type end_time)
{
    int ticks_remaining = int(end_time - ctx.system->millisecond_tick_count);
    if (ticks_remaining > 0)
    {
        request_refresh(ctx);
        return ui_time_type(ticks_remaining);
    }
    return 0;
}

float
smooth_value(ui_context& ctx, float x,
    animated_transition const& transition)
{
    return float(smooth_value(ctx, double(x), transition));
}

void reset_smoothing(value_smoothing_data& data, double value)
{
    data.smoothing = false;
    data.new_value = value;
}

double
smooth_value(ui_context& ctx, value_smoothing_data& data, double x,
    animated_transition const& transition)
{
    double current_value = data.new_value;
    if (data.smoothing)
    {
        ui_time_type ticks_left =
            get_animation_ticks_left(ctx, data.transition_end);
        if (ticks_left > 0)
        {
            current_value = data.old_value +
                eval_curve_at_x(transition.curve,
                    1. - double(ticks_left) / data.duration,
                    1. / data.duration) *
                (data.new_value - data.old_value);
        }
        else
            data.smoothing = false;
    }

    if (is_refresh_pass(ctx) && x != data.new_value)
    {
        data.duration =
            // If we're just going back to the old value, go back in the same
            // amount of time it took to get here.
            data.smoothing && x == data.old_value ?
                (transition.duration -
                    get_animation_ticks_left(ctx, data.transition_end)) :
                transition.duration;
        data.transition_end = get_animation_tick_count(ctx) + data.duration;
        data.old_value = current_value;
        data.new_value = x;
        data.smoothing = true;
    }

    return current_value;
}

double
smooth_value(ui_context& ctx, double x,
    animated_transition const& transition)
{
    value_smoothing_data* data;
    if (get_cached_data(ctx, &data))
        reset_smoothing(*data, x);

    return smooth_value(ctx, *data, x, transition);
}

optional_input_accessor<float>
smooth_value(ui_context& ctx, getter<float> const& x,
    animated_transition const& transition)
{
    optional<float> output;
    alia_if (is_gettable(x))
    {
        output = smooth_value(ctx, get(x), transition);
    }
    alia_end
    return optional_in(output);
}

optional_input_accessor<double>
smooth_value(ui_context& ctx, getter<double> const& x,
    animated_transition const& transition)
{
    optional<double> output;
    alia_if (is_gettable(x))
    {
        output = smooth_value(ctx, get(x), transition);
    }
    alia_end
    return optional_in(output);
}

void start_timer(ui_context& ctx, widget_id id, unsigned duration)
{
    input_event* ie = dynamic_cast<input_event*>(ctx.event);
    ui_time_type now = ie ? ie->time : ctx.system->millisecond_tick_count;
    ctx.surface->request_timer_event(make_routable_widget_id(ctx, id),
        now + duration);
}

bool is_timer_done(ui_context& ctx, widget_id id)
{
    return ctx.event->type == TIMER_EVENT &&
        get_event<timer_event>(ctx).id == id;
}

void restart_timer(ui_context& ctx, widget_id id, unsigned duration)
{
    timer_event& e = get_event<timer_event>(ctx);
    ctx.surface->request_timer_event(make_routable_widget_id(ctx, id),
        e.trigger_time + duration);
}

struct fps_data
{
    int frame_count;
    optional<int> fps;
};

bool compute_fps(ui_context& ctx, int* fps)
{
    widget_id id = get_widget_id(ctx);
    fps_data* data;
    if (get_cached_data(ctx, &data))
    {
        start_timer(ctx, id, 1000);
        data->frame_count = 0;
    }

    if (ctx.event->type == REFRESH_EVENT)
    {
        ++data->frame_count;
        ctx.surface->request_refresh(true);
        record_content_change(ctx);
    }

    if (is_timer_done(ctx, id))
    {
        data->fps = data->frame_count;
        data->frame_count = 0;
        restart_timer(ctx, id, 1000);
        end_pass(ctx);
    }

    if (data->fps)
    {
        *fps = get(data->fps);
        return true;
    }
    else
        return false;
}

}
