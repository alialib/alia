#include <alia/rotated_layout.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <alia/transformations.hpp>

namespace alia {

struct rotated_layout::data
{
    data() : minimum_size(0, 0) {}
    alia::layout_data layout_data;
    vector2i minimum_size;
};

void rotated_layout::begin(context& ctx, layout const& layout_spec)
{
    ctx_ = &ctx;
    data_ = get_data<data>(ctx);
    layout_spec_ = layout_spec;

    if (ctx.event->category == LAYOUT_CATEGORY &&
        get_event<layout_event>(ctx).active_logic)
    {
        resolved_layout_spec resolved;
        resolve_layout_spec(ctx, &resolved, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(data_->minimum_size, 0, 0, data_->minimum_size,
                FILL, true));
        switch (ctx.event->type)
        {
         case REFRESH_EVENT:
            diff_widget_location(ctx, data_->layout_data);
            break;
         case LAYOUT_PASS_1:
          {
            data_->layout_data.assigned_region.size[0] =
                get_assigned_width(ctx, resolved);
            break;
          }
         case LAYOUT_PASS_2:
          {
            get_assigned_region(ctx, &data_->layout_data.assigned_region,
                resolved);
            record_layout(ctx, data_->layout_data, resolved);
            break;
          }
        }
    }

    st_.begin(ctx);
    st_.set(
        translation(vector2d(
            data_->layout_data.assigned_region.corner[0],
            get_high_corner(data_->layout_data.assigned_region)[1])) *
        rotation(-pi / 2));

    overlay_.begin(ctx, box2i(point2i(0, 0), vector2i(
        data_->layout_data.assigned_region.size[1],
        data_->layout_data.assigned_region.size[0])));

    active_ = true;
}
void rotated_layout::end()
{
    if (!active_)
        return;
    active_ = false;

    overlay_.end();
    st_.end();

    if (ctx_->event->category == LAYOUT_CATEGORY &&
        get_event<layout_event>(*ctx_).active_logic)
    {
        switch (ctx_->event->type)
        {
         case LAYOUT_PASS_0:
          {
            data_->minimum_size[0] = overlay_.get_minimum_size()[1];
            break;
          }
         case LAYOUT_PASS_1:
          {
            data_->minimum_size[1] = overlay_.get_minimum_size()[0];
            break;
          }
        }
        resolved_layout_spec resolved;
        resolve_layout_spec(*ctx_, &resolved, layout_spec_,
            resolve_size(*ctx_, layout_spec_.size),
            widget_layout_info(data_->minimum_size, 0, 0,
                data_->minimum_size, FILL, true));
        switch (ctx_->event->type)
        {
         case REFRESH_EVENT:
            diff_layout_spec(*ctx_, data_->layout_data, resolved);
            break;
         case LAYOUT_PASS_0:
            request_horizontal_space(*ctx_, resolved);
            break;
         case LAYOUT_PASS_1:
            request_vertical_space(*ctx_, resolved);
            break;
        }
    }
}

}
