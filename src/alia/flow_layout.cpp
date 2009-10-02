#include <alia/flow_layout.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <algorithm>
#include <vector>

namespace alia {

struct flow_layout_data : layout_object_data
{
    struct row_info
    {
        vector2i minimum_size;
        int minimum_ascent, minimum_descent;
        float total_proportion;
    };
    std::vector<row_info> rows;
    int assigned_width;
};

// flow_layout_logic

void flow_layout_logic::begin(context& ctx, flow_layout_data& data)
{
    ctx_ = &ctx;
    data_ = &data;
    switch (ctx_->event->type)
    {
     case LAYOUT_PASS_0:
        data_->minimum_size[0] = 0;
        break;
     case LAYOUT_PASS_1:
        data_->assigned_width = data_->layout_data.assigned_region.size[0];
        data_->rows.clear();
        remaining_width_ = -1;
        break;
     case LAYOUT_PASS_2:
        if (!data_->rows.empty())
        {
            position_ = data_->layout_data.assigned_region.corner;
            row_n_ = 0;
            start_row_assignment();
        }
        break;
    }
}
void flow_layout_logic::end()
{
    data_->minimum_size[1] = data_->minimum_ascent =
        data_->minimum_descent = 0;
    for (std::vector<flow_layout_data::row_info>::iterator
        i = data_->rows.begin(); i != data_->rows.end(); ++i)
    {
        i->minimum_size[1] = (std::max)(i->minimum_size[1],
            i->minimum_ascent + i->minimum_descent);
        data_->minimum_size[1] += i->minimum_size[1];
    }
}
void flow_layout_logic::start_new_row()
{
    flow_layout_data::row_info ri;
    ri.minimum_size = vector2i(0, 0);
    ri.minimum_ascent = 0;
    ri.minimum_descent = 0;
    ri.total_proportion = 0;
    data_->rows.push_back(ri);
    remaining_width_ = data_->layout_data.assigned_region.size[0];
}
void flow_layout_logic::start_row_assignment()
{
    remaining_width_ = data_->assigned_width;
    flow_layout_data::row_info& ri = data_->rows[row_n_];
    remaining_proportion_ = ri.total_proportion;
    extra_space_ = data_->assigned_width - ri.minimum_size[0];
}
void flow_layout_logic::request_horizontal_space_for_node(int minimum_width,
    float proportion)
{
    data_->minimum_size[0] = (std::max)(data_->minimum_size[0],
        minimum_width);
}
int flow_layout_logic::get_width_for_node(int minimum_width, float proportion)
{
    if (remaining_width_ < minimum_width)
        start_new_row();
    assert(remaining_width_ >= minimum_width);
    remaining_width_ -= minimum_width;
    flow_layout_data::row_info& ri = data_->rows.back();
    ri.total_proportion += proportion;
    ri.minimum_size[0] += minimum_width;
    return minimum_width;
}
void flow_layout_logic::request_vertical_space_for_node(
    int minimum_height, int minimum_ascent, int minimum_descent,
    float proportion)
{
    flow_layout_data::row_info& ri = data_->rows.back();
    ri.minimum_size[1] = (std::max)(ri.minimum_size[1], minimum_height);
    ri.minimum_ascent = (std::max)(ri.minimum_ascent, minimum_ascent);
    ri.minimum_descent = (std::max)(ri.minimum_descent, minimum_descent);
}
void flow_layout_logic::get_region_for_node(box2i* region, int* baseline_y,
    vector2i const& minimum_size, float proportion)
{
    if (remaining_width_ < minimum_size[0])
    {
        position_[0] = data_->layout_data.assigned_region.corner[0];
        position_[1] += data_->rows[row_n_].minimum_size[1];
        ++row_n_;
        start_row_assignment();
    }
    flow_layout_data::row_info& ri = data_->rows[row_n_];
    int this_size = minimum_size[0];
    if (proportion > 0)
    {
        int this_extra = int(extra_space_ * proportion /
            remaining_proportion_ + 0.5);
        this_size += this_extra;
        remaining_proportion_ -= proportion;
        extra_space_ -= this_extra;
    }
    *baseline_y = ri.minimum_size[1] - ri.minimum_descent;
    region->corner = position_;
    region->size[0] = this_size;
    region->size[1] = ri.minimum_size[1];
    position_[0] += this_size;
    remaining_width_ -= this_size;
}

// flow_layout

void flow_layout::begin(context& ctx, layout const& layout_spec)
{
    flow_layout_data& data = *get_data<flow_layout_data>(ctx);
    layout_object::begin(ctx, data, layout_spec, &logic_);
    if (layout_object::is_active() && layout_object::logic_needed())
        logic_.begin(ctx, data);
}
void flow_layout::end()
{
    if (layout_object::is_active())
    {
        if (layout_object::logic_needed())
            logic_.end();
        layout_object::end();
    }
}

}
