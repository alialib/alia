#include <alia/uniform_grid_layout.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <algorithm>
#include <vector>

namespace alia {

struct uniform_grid_layout::data : layout_object_data
{
    data() : n_columns(0), n_rows(0) {}
    unsigned n_columns, n_rows;
};

// uniform_grid_layout::logic

void uniform_grid_layout::logic::initialize_assignment(unsigned axis)
{
    box2i const& region = data_->layout_data.assigned_region;
    position_[axis] = region.corner[axis];
    int assigned_size = region.size[axis];
    unsigned n_items = (axis == 0) ? n_columns_ : data_->n_rows;
    item_size_[axis] = n_items > 0 ? assigned_size / n_items : 0;
    n_larger_items_[axis] = assigned_size - (n_items * item_size_[axis]);
}
void uniform_grid_layout::logic::inc(vector2i& index)
{
    ++index[0];
    if (index[0] == n_columns_)
    {
        index[0] = 0;
        ++index[1];
    }
}
int uniform_grid_layout::logic::get_assigned_size(unsigned axis)
{
    int size = item_size_[axis];
    if (assignment_index_[axis] < n_larger_items_[axis])
        ++size;
    return size;
}

void uniform_grid_layout::logic::begin(context& ctx, data& data,
    unsigned n_columns)
{
    ctx_ = &ctx;
    data_ = &data;
    n_columns_ = n_columns;
    request_index_ = assignment_index_ = vector2i(0, 0);
    switch (ctx_->event->type)
    {
     case LAYOUT_PASS_0:
        data_->minimum_size[0] = 0;
        break;
     case LAYOUT_PASS_1:
        data_->minimum_size[1] = 0;
        data_->minimum_ascent = 0;
        data_->minimum_descent = 0;
        initialize_assignment(0);
        break;
     case LAYOUT_PASS_2:
        baseline_y_ = data_->layout_data.assigned_region.size[1] -
            data_->minimum_descent;
        initialize_assignment(0);
        initialize_assignment(1);
        break;
    }
}
void uniform_grid_layout::logic::end()
{
    if (ctx_->event->type == LAYOUT_PASS_1)
    {
        data_->minimum_size[1] = (std::max)(data_->minimum_size[1],
            data_->minimum_ascent + data_->minimum_descent);
        data_->n_rows = request_index_[1] + (request_index_[0] == 0 ? 0 : 1);
    }
}
void uniform_grid_layout::logic::request_horizontal_space_for_node(
    int minimum_width, float proportion)
{
    data_->minimum_size[0] = (std::max)(data_->minimum_size[0], minimum_width);
    inc(request_index_);
}
int uniform_grid_layout::logic::get_width_for_node(int minimum_width,
    float proportion)
{
    int size = get_assigned_size(0);
    inc(assignment_index_);
    return size;
}
void uniform_grid_layout::logic::request_vertical_space_for_node(
    int minimum_height, int minimum_ascent, int minimum_descent,
    float proportion)
{
    data_->minimum_size[1] = (std::max)(data_->minimum_size[1],
        minimum_height);
    data_->minimum_ascent = (std::max)(data_->minimum_ascent,
        minimum_ascent);
    data_->minimum_descent = (std::max)(data_->minimum_descent,
        minimum_descent);
    inc(request_index_);
}
void uniform_grid_layout::logic::get_region_for_node(box2i* region,
    int* baseline_y, vector2i const& minimum_size, float proportion)
{
    region->corner = position_;
    *baseline_y = baseline_y_;
    for (unsigned i = 0; i != 2; ++i)
        region->size[i] = get_assigned_size(i);
    position_[0] += region->size[0];
    ++assignment_index_[0];
    if (assignment_index_[0] == n_columns_)
    {
        assignment_index_[0] = 0;
        position_[0] = data_->layout_data.assigned_region.corner[0];
        ++assignment_index_[1];
        position_[1] += region->size[1];
    }
}

// uniform_grid_layout

void uniform_grid_layout::begin(context& ctx, unsigned n_columns,
    layout const& layout_spec)
{
    data& data = *get_data<uniform_grid_layout::data>(ctx);
    switch (ctx.event->type)
    {
     case REFRESH_EVENT:
        if (n_columns != data.n_columns)
            record_layout_change(ctx, data.layout_data);
        break;
     case LAYOUT_PASS_2:
        data.n_columns = n_columns;
        break;
    }
    layout_object::begin(ctx, data, layout_spec, &logic_);
    if (layout_object::logic_needed())
        logic_.begin(ctx, data, n_columns);
}
void uniform_grid_layout::end()
{
    if (layout_object::is_active())
    {
        if (layout_object::logic_needed())
            logic_.end();
        layout_object::end();
    }
}

}
