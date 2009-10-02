#include <alia/grid_layout.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <algorithm>
#include <vector>

namespace alia {

struct grid_layout::data
{
    struct column_info
    {
        column_info() : proportion(0), minimum_size(0), minimum_ascent(0),
            minimum_descent(0) {}
        float proportion;
        int minimum_size, minimum_ascent, minimum_descent, assigned_size;
    };
    std::vector<column_info> columns;
};

struct grid_row::data : layout_object_data
{
    unsigned axis;
};

// grid_layout

void grid_layout::begin(context& ctx, unsigned axis,
    layout const& layout_spec)
{
    ctx_ = &ctx;
    data_ = get_data<data>(ctx);
    layout_.begin(ctx, 1 - axis, layout_spec);
    axis_ = axis;
    saw_first_ = false;
    if (ctx_->event->category == LAYOUT_CATEGORY)
    {
        layout_event& e = get_event<layout_event>(*ctx_);
        ++e.active_grids;
        assert(e.active_grids > 0);
    }
    active_ = true;
}
void grid_layout::end()
{
    if (active_)
    {
        if (ctx_->event->category == LAYOUT_CATEGORY)
        {
            layout_event& e = get_event<layout_event>(*ctx_);
            assert(e.active_grids > 0);
            --e.active_grids;
        }
        layout_.end();
        active_ = false;
    }
}

void grid_layout::clear_columns()
{
    if (saw_first_)
        return;
    data_->columns.clear();
    saw_first_ = true;
}

void grid_layout::calculate_column_sizes(int assigned_size)
{
    if (saw_first_)
        return;
    float total_proportion = 0;
    int total_size = 0;
    for (std::vector<data::column_info>::iterator
        i = data_->columns.begin(); i != data_->columns.end(); ++i)
    {
        total_proportion += i->proportion;
        total_size += i->minimum_size;
    }
    int extra_space = assigned_size - total_size;
    for (std::vector<data::column_info>::iterator
        i = data_->columns.begin(); i != data_->columns.end(); ++i)
    {
        int this_size = i->minimum_size;
        if (i->proportion > 0)
        {
            int this_extra = int(extra_space * i->proportion /
                total_proportion + 0.5);
            this_size += this_extra;
            total_proportion -= i->proportion;
            extra_space -= this_extra;
        }
        i->assigned_size = this_size;
    }
    saw_first_ = true;
}

// grid_row::logic

void grid_row::logic::begin(grid_layout& grid, data& data)
{
    grid_ = &grid;
    data_ = &data;
    switch (grid.ctx_->event->type)
    {
     case LAYOUT_PASS_0:
        minimum_length_ = 0;
        data_->minimum_size[0] = 0;
        break;
     case LAYOUT_PASS_1:
        if (grid_->axis_ == 0)
        {
            grid_->calculate_column_sizes(
                data_->layout_data.assigned_region.size[0]);
        }
        minimum_length_ = 0;
        data_->minimum_size[1] = 0;
        data_->minimum_ascent = 0;
        data_->minimum_descent = 0;
        break;
     case LAYOUT_PASS_2:
        grid_->calculate_column_sizes(
            data_->layout_data.assigned_region.size[grid_->axis_]);
        position_ = data.layout_data.assigned_region.corner[grid.axis_];
        break;
    }
    index_ = 0;
}
void grid_row::logic::end()
{
    switch (grid_->ctx_->event->type)
    {
     case LAYOUT_PASS_0:
        if (grid_->axis_ == 0)
            data_->minimum_size[0] = minimum_length_;
        break;
     case LAYOUT_PASS_1:
        if (grid_->axis_ == 0)
        {
            data_->minimum_size[1] = (std::max)(data_->minimum_size[1],
                data_->minimum_ascent + data_->minimum_descent);
        }
        else
            data_->minimum_size[1] = minimum_length_;
        break;
    }
}
void grid_row::logic::request_horizontal_space_for_node(int minimum_width,
    float proportion)
{
    if (grid_->axis_ == 0)
    {
        grid_->clear_columns();
        assert(index_ <= grid_->data_->columns.size());
        if (index_ >= grid_->data_->columns.size())
            grid_->data_->columns.resize(index_ + 1);
        grid_layout::data::column_info& ci = grid_->data_->columns[index_];
        ++index_;
        ci.minimum_size = (std::max)(ci.minimum_size, minimum_width);
        ci.proportion = (std::max)(ci.proportion, proportion);
        minimum_length_ += ci.minimum_size;
    }
    else
    {
        data_->minimum_size[0] = (std::max)(data_->minimum_size[0],
            minimum_width);
    }
}
int grid_row::logic::get_width_for_node(int minimum_width,
    float proportion)
{
    if (grid_->axis_ == 0)
    {
        if (index_ >= grid_->data_->columns.size())
            assert(index_ < grid_->data_->columns.size());
        grid_layout::data::column_info& ci = grid_->data_->columns[index_];
        ++index_;
        return ci.assigned_size;
    }
    else
        return data_->layout_data.assigned_region.size[0];
}
void grid_row::logic::request_vertical_space_for_node(int minimum_height,
    int minimum_ascent, int minimum_descent, float proportion)
{
    if (grid_->axis_ == 1)
    {
        grid_->clear_columns();
        assert(index_ < grid_->data_->columns.size());
        grid_layout::data::column_info& ci = grid_->data_->columns[index_];
        ++index_;
        ci.minimum_size = (std::max)(ci.minimum_size, minimum_height);
        ci.minimum_ascent = (std::max)(ci.minimum_ascent, minimum_ascent);
        ci.minimum_descent = (std::max)(ci.minimum_descent, minimum_descent);
        ci.proportion = (std::max)(ci.proportion, proportion);
        ci.minimum_size = (std::max)(ci.minimum_size, ci.minimum_ascent +
            ci.minimum_descent);
        minimum_length_ += ci.minimum_size;
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
void grid_row::logic::get_region_for_node(box2i* region, int* baseline_y,
    vector2i const& minimum_size, float proportion)
{
    assert(index_ < grid_->data_->columns.size());
    grid_layout::data::column_info& ci = grid_->data_->columns[index_];
    ++index_;
    if (grid_->axis_ == 0)
    {
        region->corner[0] = position_;
        position_ += ci.assigned_size;
        region->corner[1] = data_->layout_data.assigned_region.corner[1];
        region->size[0] = ci.assigned_size;
        region->size[1] = data_->layout_data.assigned_region.size[1];
        *baseline_y = data_->layout_data.assigned_region.size[1] -
            data_->minimum_descent;
    }
    else
    {
        region->corner[0] = data_->layout_data.assigned_region.corner[0];
        region->corner[1] = position_;
        position_ += ci.assigned_size;
        region->size[0] = data_->layout_data.assigned_region.size[0];
        region->size[1] = ci.assigned_size;
        *baseline_y = ci.assigned_size - ci.minimum_descent;
    }
}

// grid_row

void grid_row::begin(grid_layout& grid, layout const& layout_spec)
{
    grid_ = &grid;
    data& data = *get_data<grid_row::data>(*grid.ctx_);
    if (grid.ctx_->event->category == LAYOUT_CATEGORY)
    {
        switch (grid.ctx_->event->type)
        {
         case REFRESH_EVENT:
            if (grid.axis_ != data.axis)
                record_layout_change(*grid.ctx_, data.layout_data);
            break;
         case LAYOUT_PASS_2:
            data.axis = grid.axis_;
            break;
        }
    }
    layout_object::begin(*grid.ctx_, data, layout_spec, &logic_);
    if (layout_object::logic_needed())
        logic_.begin(grid, data);
    if (grid.ctx_->event->category == LAYOUT_CATEGORY)
    {
        layout_event& e = get_event<layout_event>(*grid.ctx_);
        assert(e.active_grids > 0);
        --e.active_grids;
    }
    active_ = true;
}
void grid_row::end()
{
    if (active_)
    {
        if (grid_->ctx_->event->category == LAYOUT_CATEGORY)
        {
            layout_event& e = get_event<layout_event>(*grid_->ctx_);
            ++e.active_grids;
            assert(e.active_grids > 0);
        }
        if (layout_object::is_active())
        {
            if (layout_object::logic_needed())
                logic_.end();
            layout_object::end();
        }
        active_ = false;
    }
}

}
