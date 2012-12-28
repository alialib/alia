#include <alia/ui/utilities/timing.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

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
        ++data->frame_count;
    ctx.surface->request_refresh();

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
