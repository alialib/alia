#include <alia/fps.hpp>
#include <alia/context.hpp>
#include <alia/input_events.hpp>
#include <alia/input_utils.hpp>
#include <alia/timer.hpp>
#include <boost/optional/optional.hpp>

namespace alia {

struct fps_data
{
    fps_data() : frame_count(0) {}
    int frame_count;
    boost::optional<unsigned> last_update;
    boost::optional<int> fps;
};

bool compute_fps(context& ctx, int* fps)
{
    region_id id = get_region_id(ctx);
    fps_data& data = *get_data<fps_data>(ctx);

    if (ctx.event->type == REFRESH_EVENT)
        ++data.frame_count;

    if (is_timer_done(ctx, id))
    {
        if (!data.last_update)
        {
            data.last_update = get_event<timer_event>(ctx).now;
        }
        else if (get_event<timer_event>(ctx).now >=
            data.last_update.get() + 1000)
        {
            data.last_update.get() += 1000;
            data.fps = data.frame_count;
            data.frame_count = 0;
        }
    }

    start_timer(ctx, id, 0);

    if (data.fps)
    {
        *fps = data.fps.get();
        return true;
    }
    else
        return false;
}

}
