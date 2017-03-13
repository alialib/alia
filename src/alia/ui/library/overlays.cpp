#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

void overlay_event_transformer::begin(dataless_ui_context& ctx, widget_id id)
{
    ctx_ = &ctx;

    real_event_category_ = ctx.event->category;
    real_event_type_ = ctx.event->type;

    // If this is an overlay event, and the overlay is active, activate the
    // event by translating it to the underlying event type.
    // Or if this is one of corresponding normal events (like rendering),
    // disable it inside the overlay.
    switch(ctx.event->type)
    {
     case OVERLAY_RENDER_EVENT:
        if (is_overlay_active(ctx, id))
        {
            ctx.event->category = RENDER_CATEGORY;
            ctx.event->type = RENDER_EVENT;
        }
        break;

     case OVERLAY_MOUSE_HIT_TEST_EVENT:
        if (is_overlay_active(ctx, id))
        {
            ctx.event->category = REGION_CATEGORY;
            ctx.event->type = MOUSE_HIT_TEST_EVENT;
        }
        break;

     case OVERLAY_WHEEL_HIT_TEST_EVENT:
        if (is_overlay_active(ctx, id))
        {
            ctx.event->category = REGION_CATEGORY;
            ctx.event->type = WHEEL_HIT_TEST_EVENT;
        }
        break;

     case OVERLAY_MAKE_WIDGET_VISIBLE_EVENT:
        if (is_overlay_active(ctx, id))
        {
            ctx.event->category = REGION_CATEGORY;
            ctx.event->type = MAKE_WIDGET_VISIBLE_EVENT;
        }
        break;

     case RENDER_EVENT:
     case MOUSE_HIT_TEST_EVENT:
     case WHEEL_HIT_TEST_EVENT:
     case MAKE_WIDGET_VISIBLE_EVENT:
        ctx.event->category = NO_CATEGORY;
        ctx.event->type = NO_EVENT;
        break;
    }
}

void overlay_event_transformer::end()
{
    if (ctx_)
    {
        dataless_ui_context& ctx = *ctx_;

        ctx.event->category = real_event_category_;
        ctx.event->type = real_event_type_;

        ctx_ = 0;
    }
}

void popup::begin(ui_context& ctx, widget_id id,
    popup_positioning const& positioning)
{
    ctx_ = &ctx;
    id_ = id;

    bool active = is_overlay_active(ctx, id);

    layout_vector surface_size = layout_vector(ctx.system->surface_size);
    layout_vector maximum_size;
    if (active)
    {
        for (unsigned i = 0; i != 2; ++i)
        {
            maximum_size[i] = (std::max)(
                positioning.absolute_upper[i],
                surface_size[i] - positioning.absolute_lower[i]);
        }
    }
    else
        maximum_size = surface_size;

    layout_.begin(ctx, positioning.minimum_size, maximum_size);

    if (active && !is_refresh_pass(ctx))
    {
        vector<2,int> position;
        for (unsigned i = 0; i != 2; ++i)
        {
            if (positioning.absolute_lower[i] + layout_.size()[i] <=
                    surface_size[i] ||
                surface_size[i] - positioning.absolute_lower[i] >
                    positioning.absolute_upper[i])
            {
                position[i] = positioning.lower_bound[i];
            }
            else
            {
                position[i] = positioning.upper_bound[i] - layout_.size()[i];
            }
        }
        transform_.begin(*get_layout_traversal(ctx).geometry);
        transform_.set(translation_matrix(vector<2,double>(position)));
    }

    overlay_.begin(ctx, id);

    background_id_ = get_widget_id(ctx);
    if (active)
    {
        // Intercept mouse clicks and wheel movement to other parts of the surface.
        handle_mouse_hit(ctx, background_id_,
            // This box doesn't matter since we're not really doing any input processing.
            box<2,double>(make_vector(0., 0.), make_vector(0., 0.)),
            HIT_TEST_MOUSE | HIT_TEST_WHEEL);
        // If any are detected, or if the popup loses focus, close it.
        if (detect_mouse_press(ctx, background_id_, LEFT_BUTTON) ||
            detect_mouse_press(ctx, background_id_, MIDDLE_BUTTON) ||
            detect_mouse_press(ctx, background_id_, RIGHT_BUTTON) ||
            detect_focus_loss(ctx, id_))
        {
            ctx.system->overlay_id = null_widget_id;
        }
    }
}
void popup::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;

        overlay_.end();
        transform_.end();
        layout_.end();

        ctx_ = 0;
    }
}

}
