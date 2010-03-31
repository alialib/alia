#include <alia/mouse_gestures.hpp>
#include <alia/context.hpp>
#include <alia/input_utils.hpp>

namespace alia {

struct mouse_gesture_data
{
    alia::gesture gesture;
    std::vector<point2d> trail;
};

void calculate_gesture(mouse_gesture_data& data)
{
    data.gesture.clear();
    std::vector<point2d>::iterator start, i;
    start = i = data.trail.begin();
    while (i != data.trail.end())
    {
        vector2d delta = *i - *start;
        double l = length(delta);
        if (l > 10)
        {
            gesture_direction d;
            bool valid = false;
            if (fabs(delta[0] / l) > 0.9)
            {
                d = delta[0] > 0 ? GESTURE_RIGHT : GESTURE_LEFT;
                valid = true;
            }
            else if (fabs(delta[1] / l) > 0.9)
            {
                d = delta[1] > 0 ? GESTURE_DOWN : GESTURE_UP;
                valid = true;
            }
            if (valid && (data.gesture.empty() || data.gesture.back() != d))
                data.gesture.push_back(d);
            start = i;
        }
        ++i;
    }
}

bool detect_mouse_gesture(context& ctx, region_id id,
    mouse_button button, rgba8 const& color, line_style const& style)
{
    mouse_gesture_data& data = *get_data<mouse_gesture_data>(ctx);
    if (detect_drag(ctx, id, button))
    {
        point2d const& p = ctx.pass_state.mouse_position;
        if (data.trail.empty() || data.trail.back() != p)
        {
            data.trail.push_back(p);
            calculate_gesture(data);
        }
    }
    if (detect_mouse_up(ctx, button))
        data.trail.clear();
    if (ctx.event->type == RENDER_EVENT && !data.trail.empty())
    {
        ctx.surface->draw_line_strip(color, style, &data.trail[0],
            data.trail.size());
    }
    return false;
}

}
