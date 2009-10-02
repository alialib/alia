#include <alia/context.hpp>
#include <alia/controller.hpp>
#include <alia/surface.hpp>
#include <alia/overlay.hpp>
#include <alia/layout.hpp>
#include <alia/scrollable_region.hpp>
#include <alia/input_utils.hpp>
#include <alia/region.hpp>
#include <alia/transformations.hpp>

namespace alia {

struct pass_state_saver
{
    pass_state_saver(context& ctx)
      : ctx(ctx)
    {
        old_state = ctx.pass_state;
        old_event = ctx.event;
    }
    ~pass_state_saver()
    {
        ctx.pass_state = old_state;
        ctx.event = old_event;
    }
    context& ctx;
    pass_state old_state;
    event* old_event;
};

void issue_event(context& ctx, event& event)
{
    ++ctx.pass_counter;
    pass_state_saver pss(ctx);
    ctx.event = &event;
    if (event.culling_type == TARGETED_CULLING)
    {
        targeted_event& e = get_event<targeted_event>(ctx);
        //if (e.target_id && e.target_id->last_refresh == ctx.refresh_counter)
        //{
        //    layout_data* d = e.target_id->container;
        //    while (d)
        //    {
        //        e.path_to_target.push_front(d);
        //        d = d->parent;
        //    }
        //}
        //else
            e.culling_type = NO_CULLING;
    }
    scoped_data_block db(ctx, ctx.root_block);
    naming_context nc(ctx);
    box2i full_region(point2i(0, 0), ctx.surface->get_size());
    set_transformation(ctx, identity_matrix<3,double>());
    set_clip_region(ctx, full_region);
    ctx.pass_state.style_code = 0;
    ctx.artist->activate_style(0);
    overlay root_overlay(ctx, full_region);
    column_layout c(ctx, GROW);
    //scrollable_region sr(ctx, -1, GROW);
    ctx.controller->do_ui(ctx);
    if (event.type == LAYOUT_PASS_2)
        ctx.content_size = root_overlay.get_minimum_size();
        //ctx.content_size = sr.get_content_size();
    // TODO: move this to behavior
    if (detect_key_press(ctx, KEY_TAB))
        set_focus(ctx, get_id_after_focus(ctx));
    if (detect_key_press(ctx, KEY_TAB, MOD_SHIFT))
        set_focus(ctx, get_id_before_focus(ctx));
}

void update_untransformed_clip_region(context& ctx)
{
    ctx.pass_state.untransformed_clip_region.corner =
        transform_point(ctx.pass_state.inverse_transformation,
            point2d(ctx.pass_state.clip_region.corner));
    ctx.pass_state.untransformed_clip_region.size =
        transform_vector(ctx.pass_state.inverse_transformation,
            vector2d(ctx.pass_state.clip_region.size));
    for (int i = 0; i < 2; ++i)
    {
        // TODO: This rounding might not be correct.
        ctx.pass_state.integer_untransformed_clip_region.corner[i] =
            int(ctx.pass_state.untransformed_clip_region.corner[i] + 0.5);
        ctx.pass_state.integer_untransformed_clip_region.size[i] =
            int(ctx.pass_state.untransformed_clip_region.size[i] + 0.5);
    }
}

void set_clip_region(context& ctx, box2i const& region)
{
    ctx.pass_state.clip_region = region;
    update_untransformed_clip_region(ctx);
    if (ctx.event->type == RENDER_EVENT)
        ctx.surface->set_clip_region(region);
}

void set_transformation(context& ctx,
    matrix<3,3,double> const& transformation)
{
    ctx.pass_state.transformation = transformation;
    ctx.pass_state.inverse_transformation = inverse(transformation);
    ctx.pass_state.mouse_position =
        transform_point(ctx.pass_state.inverse_transformation,
            point2d(ctx.mouse_position));
    update_untransformed_clip_region(ctx);
    for (int i = 0; i < 2; ++i)
    {
        ctx.pass_state.integer_mouse_position[i] =
            int(std::floor(ctx.pass_state.mouse_position[i]));
    }
    if (ctx.event->culling_type == POINT_CULLING)
    {
        point_event& e = get_event<point_event>(ctx);
        e.untransformed_p = transform_point(
            ctx.pass_state.inverse_transformation, point2d(e.p));
        for (int i = 0; i < 2; ++i)
        {
            e.integer_untransformed_p[i] =
                int(std::floor(e.untransformed_p[i]));
        }
    }
    if (ctx.event->type == RENDER_EVENT)
        ctx.surface->set_transformation_matrix(transformation);
}

}
