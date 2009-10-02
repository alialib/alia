#include <alia/layered_layout.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <algorithm>

namespace alia {

struct layered_layout::data : layout_object_data
{
};

// layered_layout::logic

void layered_layout::logic::begin(context& ctx, data& data)
{
    data_ = &data;
    switch (ctx.event->type)
    {
     case LAYOUT_PASS_0:
        data_->minimum_size[0] = 0;
        break;
     case LAYOUT_PASS_1:
        data_->minimum_size[1] = 0;
        data_->minimum_ascent = 0;
        data_->minimum_descent = 0;
        break;
    }
}
void layered_layout::logic::end()
{
}
void layered_layout::logic::request_horizontal_space_for_node(
    int minimum_width, float proportion)
{
    data_->minimum_size[0] = (std::max)(data_->minimum_size[0], minimum_width);
}
int layered_layout::logic::get_width_for_node(
    int minimum_width, float proportion)
{
    return data_->layout_data.assigned_region.size[0];
}
void layered_layout::logic::request_vertical_space_for_node(
    int minimum_height, int minimum_ascent, int minimum_descent,
    float proportion)
{
    data_->minimum_size[1] = (std::max)(data_->minimum_size[1],
        minimum_height);
    data_->minimum_ascent = (std::max)(data_->minimum_ascent,
        minimum_ascent);
    data_->minimum_descent = (std::max)(data_->minimum_descent,
        minimum_descent);
}
void layered_layout::logic::get_region_for_node(box2i* region, int* baseline_y,
    vector2i const& minimum_size, float proportion)
{
    *region = data_->layout_data.assigned_region;
    *baseline_y = 0;
}

// layered_layout

void layered_layout::begin(context& ctx, layout const& layout_spec)
{
    data& data = *get_data<layered_layout::data>(ctx);
    layout_object::begin(ctx, data, layout_spec, &logic_);
    if (layout_object::logic_needed())
        logic_.begin(ctx, data);
}
void layered_layout::end()
{
    if (layout_object::is_active())
    {
        if (layout_object::logic_needed())
            logic_.end();
        layout_object::end();
    }
}

}
