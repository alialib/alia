#include <alia/linear_layout.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <algorithm>

namespace alia {

struct linear_layout::data : layout_object_data
{
    unsigned axis;
    // sum of all the proportion values reported by the child nodes (along the
    // major axis)
    float total_proportion;
};

// linear_layout::logic

void linear_layout::logic::begin(context& ctx, data& data, unsigned axis)
{
    ctx_ = &ctx;
    data_ = &data;
    axis_ = axis;
    item_n_ = 0;
    switch (ctx_->event->type)
    {
     case LAYOUT_PASS_0:
      {
        data_->minimum_size[0] = 0;
        if (axis_ == 0)
            data_->total_proportion = 0;
        break;
      }
     case LAYOUT_PASS_1:
      {
        data_->minimum_size[1] = 0;
        if (axis_ == 1)
            data_->total_proportion = 0;
        data_->minimum_ascent = 0;
        data_->minimum_descent = 0;
        // intentional fall through...
      }
     case LAYOUT_PASS_2:
      {
        int major = axis_, minor = 1 - axis_;
        box2i const& region = data_->layout_data.assigned_region;
        baseline_y_ = axis_ == 0 ? region.size[1] - data_->minimum_descent : 0;
        remaining_proportion_ = data_->total_proportion;
        major_position_ = region.corner[major];
        extra_space_ = region.size[major] - data_->minimum_size[major];
        break;
      }
    }
}
void linear_layout::logic::end()
{
    if (ctx_->event->type == LAYOUT_PASS_1 && axis_ == 0)
    {
        data_->minimum_size[1] = (std::max)(data_->minimum_size[1],
            data_->minimum_ascent + data_->minimum_descent);
    }
}
void linear_layout::logic::request_horizontal_space_for_node(
    int minimum_width, float proportion)
{
    if (axis_ == 0)
    {
        data_->minimum_size[0] += minimum_width;
        data_->total_proportion += proportion;
    }
    else
    {
        data_->minimum_size[0] = (std::max)(data_->minimum_size[0],
            minimum_width);
    }
}
int linear_layout::logic::get_width_for_node(int minimum_width,
    float proportion)
{
    if (axis_ == 0)
    {
        int this_size = minimum_width;
        if (proportion > 0)
        {
            int this_extra = int(extra_space_ * proportion /
                remaining_proportion_ + 0.5);
            this_size += this_extra;
            remaining_proportion_ -= proportion;
            extra_space_ -= this_extra;
        }
        return this_size;
    }
    else
        return data_->layout_data.assigned_region.size[0];
}
void linear_layout::logic::request_vertical_space_for_node(
    int minimum_height, int minimum_ascent, int minimum_descent,
    float proportion)
{
    if (axis_ == 1)
    {
        data_->minimum_size[1] += minimum_height;
        data_->total_proportion += proportion;
        if (item_n_ == 0)
        {
            data_->minimum_ascent = minimum_ascent;
            data_->minimum_descent = minimum_descent;
        }
        else
            data_->minimum_descent += minimum_height;
        ++item_n_;
    }
    else
    {
        data_->minimum_size[1] = (std::max)(data_->minimum_size[1],
            minimum_height);
        data_->minimum_ascent = (std::max)(data_->minimum_ascent,
            minimum_ascent);
        data_->minimum_descent = (std::max)(data_->minimum_descent,
            minimum_descent);
    }
}
void linear_layout::logic::get_region_for_node(box2i* region, int* baseline_y,
    vector2i const& minimum_size, float proportion)
{
    int major = axis_, minor = 1 - axis_;

    region->corner[minor] = data_->layout_data.assigned_region.corner[minor];
    region->size[minor] = data_->layout_data.assigned_region.size[minor];

    int this_size = minimum_size[major];
    if (proportion > 0)
    {
        int this_extra = int(extra_space_ * proportion /
            remaining_proportion_ + 0.5);
        this_size += this_extra;
        remaining_proportion_ -= proportion;
        extra_space_ -= this_extra;
    }
    region->corner[major] = major_position_;
    major_position_ += this_size;
    region->size[major] = this_size;
    *baseline_y = baseline_y_;
}

// linear_layout

void linear_layout::begin(context& ctx, layout const& layout_spec,
    flag_set flags)
{
    data& data = *get_data<linear_layout::data>(ctx);
    unsigned axis = (flags & HORIZONTAL) ? 0 : 1;
    switch (ctx.event->type)
    {
     case REFRESH_EVENT:
        if (axis != data.axis)
            record_layout_change(ctx, data.layout_data);
        break;
     case LAYOUT_PASS_2:
        data.axis = axis;
        break;
    }
    layout_object::begin(ctx, data, layout_spec, &logic_);
    if (layout_object::logic_needed())
        logic_.begin(ctx, data, axis);
}
void linear_layout::end()
{
    if (layout_object::is_active())
    {
        if (layout_object::logic_needed())
            logic_.end();
        layout_object::end();
    }
}

}
