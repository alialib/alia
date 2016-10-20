#include <alia/layout/api.hpp>
#include <alia/layout/utilities.hpp>
#include <alia/layout/system.hpp>
#include <vector>
#include <algorithm>

// This file implements the standard library of layout containers (and the
// spacer, which is the only standard leaf).

namespace alia {

void do_spacer(layout_traversal& traversal, data_traversal& data,
    layout const& layout_spec)
{
    layout_leaf* node;
    get_cached_data(data, &node);

    if (traversal.is_refresh_pass)
    {
        node->refresh_layout(traversal, layout_spec,
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
        add_layout_node(traversal, node);
    }
}

void do_spacer(layout_traversal& traversal, data_traversal& data,
    layout_box* region, layout const& layout_spec)
{
    layout_leaf* node;
    get_cached_data(data, &node);

    if (traversal.is_refresh_pass)
    {
        node->refresh_layout(traversal, layout_spec,
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
        add_layout_node(traversal, node);
    }
    else
        *region = node->assignment().region;
}

// ROW LAYOUT

ALIA_DECLARE_LAYOUT_LOGIC(row_layout_logic)

static void
compute_total_width_and_growth(
    layout_calculation_context& ctx,
    layout_scalar* total_width,
    float* total_growth,
    layout_node* children)
{
    *total_width = 0;
    *total_growth = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements r = get_horizontal_requirements(ctx, *i);
        *total_width += r.size;
        *total_growth += r.growth_factor;
    }
}

calculated_layout_requirements
row_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx,
    layout_node* children)
{
    layout_scalar total_size;
    float total_growth;
    compute_total_width_and_growth(ctx, &total_size, &total_growth, children);
    return calculated_layout_requirements(total_size, 0, 0);
}

static layout_scalar
calculate_child_size(
    layout_scalar& remaining_extra_size,
    float& remaining_growth,
    layout_scalar this_required_size,
    float this_growth)
{
    if (remaining_growth != 0)
    {
        layout_scalar extra_size = round_to_layout_scalar(
            float(remaining_extra_size) * (this_growth / remaining_growth));
        remaining_extra_size -= extra_size;
        remaining_growth -= this_growth;
        return this_required_size + extra_size;
    }
    else
        return this_required_size;
}

calculated_layout_requirements
row_layout_logic::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    layout_scalar total_size;
    float total_growth;
    compute_total_width_and_growth(ctx, &total_size, &total_growth, children);
    layout_scalar remaining_extra_size = assigned_width - total_size;
    float remaining_growth = total_growth;
    calculated_layout_requirements requirements(0, 0, 0);
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements x =
            alia::get_horizontal_requirements(ctx, *i);
        layout_scalar this_width = calculate_child_size(
            remaining_extra_size, remaining_growth,
            x.size, x.growth_factor);
        fold_in_requirements(requirements,
            alia::get_vertical_requirements(ctx, *i, this_width));
    }
    return requirements;
}

void row_layout_logic::set_relative_assignment(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    layout_scalar total_size;
    float total_growth;
    compute_total_width_and_growth(ctx, &total_size, &total_growth, children);
    layout_vector p = make_layout_vector(0, 0);
    layout_scalar remaining_extra_size = assigned_size[0] - total_size;
    float remaining_growth = total_growth;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements x =
            alia::get_horizontal_requirements(ctx, *i);
        layout_scalar this_width = calculate_child_size(
            remaining_extra_size, remaining_growth,
            x.size, x.growth_factor);
        alia::set_relative_assignment(
            ctx, *i,
            relative_layout_assignment(
                layout_box(p,
                    make_layout_vector(this_width, assigned_size[1])),
                assigned_baseline_y));
        p[0] += this_width;
    }
}

void row_layout::concrete_begin(
    layout_traversal& traversal, data_traversal& data,
    layout const& layout_spec)
{
    ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(row_layout_logic)
}

// COLUMN LAYOUT

ALIA_DECLARE_LAYOUT_LOGIC(column_layout_logic)

calculated_layout_requirements
column_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx,
    layout_node* children)
{
    return fold_horizontal_child_requirements(ctx, children);
}

calculated_layout_requirements
column_layout_logic::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    layout_scalar total_height = 0;
    layout_scalar ascent = 0, descent = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements r = alia::get_vertical_requirements(
            ctx, *i, assigned_width);
        total_height += r.size;
        // The ascent of a column is the ascent of its first child.
        // Its descent is the descent of its first child plus the total height
        // of all other children.
        // However, if the first child doesn't care about the baseline, then
        // the column ignores it as well.
        if (i == children)
        {
            ascent = r.ascent;
            descent = r.descent;
        }
        else if (ascent != 0 || descent != 0)
            descent += r.size;
    }
    return calculated_layout_requirements(
        total_height, ascent, descent);
}

void column_layout_logic::set_relative_assignment(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    layout_scalar total_size = 0;
    float total_growth = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements x =
            alia::get_vertical_requirements(ctx, *i, assigned_size[0]);
        total_size += x.size;
        total_growth += x.growth_factor;
    }
    layout_vector p = make_layout_vector(0, 0);
    layout_scalar remaining_extra_size = assigned_size[1] - total_size;
    float remaining_growth = total_growth;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements y =
            alia::get_vertical_requirements(ctx, *i, assigned_size[0]);
        layout_scalar this_height =
            calculate_child_size(
                remaining_extra_size, remaining_growth,
                y.size, y.growth_factor);
        // If this is the first child, assign it the baseline of the column.
        // The exception to this rule is when the column was allocated more
        // space than it requested. In this case, the baseline calculation
        // may have been inconsistent with how the column is planning to
        // allocate the extra space.
        layout_scalar this_baseline =
            i == children && remaining_extra_size == 0 ?
                assigned_baseline_y :
                y.ascent;
        alia::set_relative_assignment(
            ctx, *i,
            relative_layout_assignment(
                layout_box(p,
                    make_layout_vector(assigned_size[0], this_height)),
                this_baseline));
        p[1] += this_height;
    }
}

void column_layout::concrete_begin(
    layout_traversal& traversal, data_traversal& data,
    layout const& layout_spec)
{
    ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(column_layout_logic)
}

// LINEAR LAYOUT - This just chooses between row and column logic.

void linear_layout::concrete_begin(
    layout_traversal& traversal, data_traversal& data,
    linear_layout_flag_set flags, layout const& layout_spec)
{
    alia_if_(data, flags & VERTICAL_LAYOUT)
    {
        column_layout_logic* logic;
        get_simple_layout_container(
            traversal, data, &container_, &logic, layout_spec);
    }
    alia_else_(data)
    {
        row_layout_logic* logic;
        get_simple_layout_container(
            traversal, data, &container_, &logic, layout_spec);
    }
    alia_end
    slc_.begin(traversal, container_);
    begin_layout_transform(transform_, traversal, container_->cacher);
}

// LAYERED LAYOUT

ALIA_DECLARE_LAYOUT_LOGIC(layered_layout_logic)

calculated_layout_requirements
layered_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx,
    layout_node* children)
{
    return fold_horizontal_child_requirements(ctx, children);
}

calculated_layout_requirements
layered_layout_logic::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    return fold_vertical_child_requirements(ctx, children, assigned_width);
}

void layered_layout_logic::set_relative_assignment(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    assign_identical_child_regions(ctx, children, assigned_size,
        assigned_baseline_y);
}

void layered_layout::concrete_begin(
    layout_traversal& traversal, data_traversal& data,
    layout const& layout_spec)
{
    ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(layered_layout_logic)
}

// ROTATED LAYOUT

ALIA_DECLARE_LAYOUT_LOGIC(rotated_layout_logic)

calculated_layout_requirements
rotated_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx,
    layout_node* children)
{
    // The horizontal requirements of the rotated layout are the vertical
    // requirements of the child, but in order to obtain the vertical
    // requirements, we need to specify a width, so we specify an essentially
    // infinite width, which should give the minimum required height of the
    // child.
    layout_scalar width = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        // The layout protocol requires that we ask for horizontal
        // requirements first.
        layout_requirements x =
            alia::get_horizontal_requirements(ctx, *i);
        layout_requirements y =
            alia::get_vertical_requirements(ctx, *i, 100000);
        width = (std::max)(y.size, width);
    }
    return calculated_layout_requirements(width, 0, 0);
}

calculated_layout_requirements
rotated_layout_logic::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    // This requires performing the inverse of the normal vertical
    // requirements request (i.e., determine the minimum width required for
    // a given height), so we do a binary search to determine the minimum
    // child width for which the child height is less than or equal to
    // our assigned width.
    layout_scalar height = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements x =
            alia::get_horizontal_requirements(ctx, *i);
        layout_scalar lower_bound = x.size, upper_bound = 100000;
        while (!layout_scalars_almost_equal(upper_bound, lower_bound))
        {
            layout_scalar test_value = (upper_bound + lower_bound) / 2;
            layout_requirements y = alia::get_vertical_requirements(
                ctx, *i, test_value);
            if (y.size <= assigned_width)
                upper_bound = test_value;
            else
                lower_bound = test_value + layout_scalar_epsilon;
        }
        height = (std::max)(upper_bound, height);
    }
    return calculated_layout_requirements(height, 0, 0);
}

void rotated_layout_logic::set_relative_assignment(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements y =
            alia::get_vertical_requirements(ctx, *i, assigned_size[1]);
        alia::set_relative_assignment(
            ctx, *i,
            relative_layout_assignment(
                layout_box(make_layout_vector(0, 0),
                    make_layout_vector(assigned_size[1], assigned_size[0])),
                y.ascent));
    }
}

void rotated_layout::concrete_begin(
    layout_traversal& traversal, data_traversal& data,
    layout const& layout_spec)
{
    rotated_layout_logic* logic;
    get_simple_layout_container(traversal, data, &container_, &logic,
        layout_spec);
    slc_.begin(traversal, container_);

    if (!traversal.is_refresh_pass)
    {
        transform_.begin(*traversal.geometry);
        transform_.set(
            translation_matrix(vector<2,double>(
                get_assignment(container_->cacher).region.corner +
                make_layout_vector(
                    0, get_assignment(container_->cacher).region.size[1]))) *
            make_matrix<double>(
                0,  1, 0,
               -1,  0, 0,
                0,  0, 1));
    }
}

// FLOW LAYOUT

ALIA_DECLARE_LAYOUT_LOGIC_WITH_DATA(flow_layout_logic,
    layout_flag_set x_alignment_;)

calculated_layout_requirements
flow_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx, layout_node* children)
{
    // In the worst case scenario, we can put one child on each row, so the
    // required width is simply the minimal width required by the widest child.
    calculated_layout_requirements requirements(0, 0, 0);
    for (layout_node* i = children; i; i = i->next)
    {
        fold_in_requirements(requirements,
            i->get_minimal_horizontal_requirements(ctx));
    }
    return requirements;
}

static layout_scalar
calculate_wrapping(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width,
    std::vector<wrapped_row>* rows)
{
    wrapping_state state;
    state.assigned_width = assigned_width;
    state.accumulated_width = 0;
    state.active_row.requirements = layout_requirements(0, 0, 0, 0);
    state.active_row.y = 0;
    state.rows = rows;

    for (layout_node* i = children; i; i = i->next)
        i->calculate_wrapping(ctx, state);
    // Include the last/current row in the height requirements.
    wrap_row(state);

    return state.active_row.y;
}

calculated_layout_requirements
flow_layout_logic::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    std::vector<wrapped_row> rows;
    layout_scalar total_height =
        calculate_wrapping(ctx, children, assigned_width, &rows);

    // Similar to the column, the flow layout uses the baseline of the first
    // row as the baseline of the whole layout (if there is one).
    layout_scalar ascent = 0, descent = 0;
    if (!rows.empty())
    {
        layout_requirements const& row0 = rows.front().requirements;
        if (row0.ascent != 0 || row0.descent != 0)
        {
            ascent = row0.ascent;
            descent = row0.descent + (total_height - row0.size);
        }
    }

    return calculated_layout_requirements(total_height, ascent, descent);
}

void flow_layout_logic::set_relative_assignment(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    // First, compute the wrapping for the assigned width so that we know the
    // vertical requirements on each row.
    std::vector<wrapped_row> rows;
    calculate_wrapping(ctx, children, assigned_size[0], &rows);

    // Now actually do the assignments.
    wrapping_assignment_state state;
    state.x = !rows.empty()
      ? calculate_initial_x(assigned_size[0], x_alignment_, rows.front())
      : 0;
    state.assigned_width = assigned_size[0];
    state.active_row = rows.begin();
    state.end_row = rows.end();
    state.x_alignment = x_alignment_;
    for (layout_node* i = children; i; i = i->next)
        i->assign_wrapped_regions(ctx, state);
}

void flow_layout::concrete_begin(
    layout_traversal& traversal, data_traversal& data,
    layout const& requested_layout_spec)
{
    // With a flow layout, we want to have the layout itself always fill the
    // horizontal space and use the requested X alignment to position the
    // individual rows in the flow.
    auto layout_spec = add_default_padding(requested_layout_spec, PADDED);
    layout_flag_set x_alignment = FILL_X;
    if ((layout_spec.flags.code & 0x3) != 0)
    {
        x_alignment.code = layout_spec.flags.code & X_ALIGNMENT_MASK_CODE;
        layout_spec.flags.code &= ~X_ALIGNMENT_MASK_CODE;
        layout_spec.flags.code |= FILL_X_CODE;
    }
    ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(flow_layout_logic)
    logic->x_alignment_ = x_alignment;
}

// VERTICAL FLOW LAYOUT

// The vertical flow algorithm is a bit simplistic. All columns have the
// same width.

ALIA_DECLARE_LAYOUT_LOGIC(vertical_flow_layout_logic)

calculated_layout_requirements
vertical_flow_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx,
    layout_node* children)
{
    // In the worst case scenario, we can put all children in one column, so
    // the required width is simply the width required by the widest child.
    return fold_horizontal_child_requirements(ctx, children);
}

calculated_layout_requirements
vertical_flow_layout_logic::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    layout_scalar column_width = get_max_child_width(ctx, children);

    layout_scalar total_height =
        compute_total_height(ctx, children, column_width);

    int n_columns = (std::max)(int(assigned_width / column_width), 1);

    layout_scalar average_column_height = total_height / n_columns;

    // Break the children into columns and determine the longest column size.
    // Each column accumulates children until it's at least as big as the
    // average height. There are other strategies that would yield smaller and
    // more balanced placements, but this one is simple and gives reasonable
    // results.
    layout_scalar max_column_height = 0;
    layout_node* child_i = children;
    for (int i = 0; i != n_columns; ++i)
    {
        layout_scalar column_height = 0;
        while (child_i && column_height < average_column_height)
        {
            layout_requirements y =
                alia::get_vertical_requirements(ctx, *child_i, column_width);
            column_height += y.size;
            child_i = child_i->next;
        }
        if (column_height > max_column_height)
            max_column_height = column_height;
    }

    return calculated_layout_requirements(max_column_height, 0, 0);
}

void vertical_flow_layout_logic::set_relative_assignment(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    layout_scalar column_width = get_max_child_width(ctx, children);

    layout_scalar total_height =
        compute_total_height(ctx, children, column_width);

    int n_columns = (std::max)(int(assigned_size[0] / column_width), 1);

    layout_scalar average_column_height = total_height / n_columns;

    // Break the children into columns and assign their regions.
    // Each column accumulates children until it's at least as big as the
    // average height. There are other strategies that would yield smaller and
    // more balanced placements, but this one is simple and gives reasonable
    // results.
    layout_node* child_i = children;
    layout_scalar bottom = average_column_height;
    for (int i = 0; i != n_columns; ++i)
    {
        layout_vector p = make_layout_vector(0, 0);
        p[0] += i * column_width;
        while (child_i && p[1] < bottom)
        {
            layout_requirements y =
                alia::get_vertical_requirements(ctx, *child_i, column_width);
            alia::set_relative_assignment(
                ctx, *child_i,
                relative_layout_assignment(
                    layout_box(p,
                        make_layout_vector(column_width, y.size)),
                    y.ascent));
            p[1] += y.size;
            child_i = child_i->next;
        }
    }
}

void vertical_flow_layout::concrete_begin(
    layout_traversal& traversal, data_traversal& data,
    layout const& layout_spec)
{
    ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(vertical_flow_layout_logic)
}

// CLIP EVASION LAYOUT

ALIA_DECLARE_LAYOUT_LOGIC(clip_evasion_layout_logic)

calculated_layout_requirements
clip_evasion_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx,
    layout_node* children)
{
    return fold_horizontal_child_requirements(ctx, children);
}

calculated_layout_requirements
clip_evasion_layout_logic::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    return fold_vertical_child_requirements(ctx, children, assigned_width);
}

void clip_evasion_layout_logic::set_relative_assignment(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    assign_identical_child_regions(ctx, children, assigned_size,
        assigned_baseline_y);
}

void clip_evasion_layout::concrete_begin(
    layout_traversal& traversal, data_traversal& data,
    layout const& layout_spec)
{
    clip_evasion_layout_logic* logic;
    get_simple_layout_container(traversal, data, &container_, &logic,
        layout_spec);
    slc_.begin(traversal, container_);

    if (!traversal.is_refresh_pass)
    {
        vector<2,double> corner_on_surface =
            transform(
                traversal.geometry->transformation_matrix,
                vector<2,double>(
                    get_assignment(container_->cacher).region.corner));
        vector<2,double> high_corner_on_surface =
            transform(
                traversal.geometry->transformation_matrix,
                vector<2,double>(get_high_corner(
                    get_assignment(container_->cacher).region)));
        vector<2,double> offset;
        for (unsigned i = 0; i != 2; ++i)
        {
            // If the content is smaller than the clip region, just prevent it
            // from scrolling off the top of the clip region.
            if (high_corner_on_surface[i] - corner_on_surface[i] <
                traversal.geometry->clip_region.size[i])
            {
                offset[i] = (std::max)(0.,
                    traversal.geometry->clip_region.corner[i] -
                        corner_on_surface[i]);
            }
            // Otherwise, just make sure that it's not scrolled to the point
            // that there's empty space.
            else if (corner_on_surface[i] >
                traversal.geometry->clip_region.corner[i])
            {
                offset[i] = traversal.geometry->clip_region.corner[i] -
                    corner_on_surface[i];
            }
            else if (high_corner_on_surface[i] <
                get_high_corner(traversal.geometry->clip_region)[i])
            {
                offset[i] =
                    get_high_corner(traversal.geometry->clip_region)[i] -
                    high_corner_on_surface[i];
            }
            else
                offset[i] = 0;
        }
        transform_.begin(*traversal.geometry);
        transform_.set(translation_matrix(offset));
    }
}

// CLAMPED LAYOUT

struct clamped_layout_logic : layout_logic
{
    calculated_layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx,
        layout_node* children);
    calculated_layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_node* children,
        layout_scalar assigned_width);
    void set_relative_assignment(
        layout_calculation_context& ctx,
        layout_node* children,
        layout_vector const& assigned_size,
        layout_scalar assigned_baseline_y);

    layout_vector max_size;
};

calculated_layout_requirements
clamped_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx,
    layout_node* children)
{
    calculated_layout_requirements requirements =
        fold_horizontal_child_requirements(ctx, children);
    return requirements;
}

calculated_layout_requirements
clamped_layout_logic::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    calculated_layout_requirements requirements =
        fold_horizontal_child_requirements(ctx, children);
    layout_scalar clamped_width =
        (std::max)(requirements.size,
            this->max_size[0] <= 0 ? assigned_width :
                (std::min)(assigned_width, this->max_size[0]));
    return fold_vertical_child_requirements(ctx, children, clamped_width);
}

void clamped_layout_logic::set_relative_assignment(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    layout_vector clamped_size;
    for (int i = 0; i != 2; ++i)
    {
        clamped_size[i] = this->max_size[i] <= 0 ? assigned_size[i] :
            (std::min)(assigned_size[i], this->max_size[i]);
    }
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements y =
            alia::get_vertical_requirements(ctx, *i, clamped_size[0]);
        alia::set_relative_assignment(ctx, *i,
            relative_layout_assignment(
                layout_box((assigned_size - clamped_size) / 2, clamped_size),
                y.ascent));
    }
}

void clamped_layout::concrete_begin(
    layout_traversal& traversal, data_traversal& data,
    absolute_size max_size, layout const& layout_spec)
{
    clamped_layout_logic* logic;
    get_simple_layout_container(traversal, data, &container_, &logic,
        layout_spec);
    slc_.begin(traversal, container_);
    begin_layout_transform(transform_, traversal, container_->cacher);
    if (traversal.is_refresh_pass)
    {
        detect_layout_change(traversal, &logic->max_size,
            as_layout_size(resolve_absolute_size(traversal, max_size)));
    }
}

// BORDERED LAYOUT

struct bordered_layout_logic : layout_logic
{
    calculated_layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx,
        layout_node* children);
    calculated_layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_node* children,
        layout_scalar assigned_width);
    void set_relative_assignment(
        layout_calculation_context& ctx,
        layout_node* children,
        layout_vector const& assigned_size,
        layout_scalar assigned_baseline_y);

    box_border_width<layout_scalar> border;
};

calculated_layout_requirements
bordered_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx,
    layout_node* children)
{
    return calculated_layout_requirements(
        get_max_child_width(ctx, children) + (border.left + border.right), 0, 0);
}

calculated_layout_requirements
bordered_layout_logic::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    calculated_layout_requirements requirements =
        fold_vertical_child_requirements(ctx, children,
            assigned_width - (border.left + border.right));
    return calculated_layout_requirements(
        requirements.size + border.top + border.bottom,
        requirements.ascent + border.top,
        requirements.descent + border.bottom);
}

void bordered_layout_logic::set_relative_assignment(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    for (layout_node* i = children; i; i = i->next)
    {
        alia::set_relative_assignment(ctx, *i,
            relative_layout_assignment(
                layout_box(
                    make_layout_vector(border.left, border.top),
                    assigned_size -
                        make_layout_vector(
                            border.left + border.right,
                            border.top + border.bottom)),
                assigned_baseline_y - border.top));
    }
}

void bordered_layout::concrete_begin(
    layout_traversal& traversal, data_traversal& data,
    box_border_width<absolute_length> border, layout const& layout_spec)
{
    bordered_layout_logic* logic;
    get_simple_layout_container(traversal, data, &container_, &logic,
        layout_spec);
    slc_.begin(traversal, container_);
    begin_layout_transform(transform_, traversal, container_->cacher);
    if (traversal.is_refresh_pass)
    {
        detect_layout_change(traversal, &logic->border,
            as_layout_size(resolve_box_border_width(traversal, border)));
    }
}

// GRIDS

// Grids are composed of multiple rows, each with its own children.
// The children in corresponding positions in different rows are assigned the
// same width, so that the children line up in columns.
// (And, as with normal rows, children within the same row all receive the
// same height and ascent, so that the rows line up as well.)
//
// Grids store a list of the rows associated with them. When created, a grid
// row associates itself with its parent grid by adding itself to the grid's
// list of rows. Otherwise, the grid acts like a normal column layout.
// You can add other children to it as well, and they will be interspersed
// vertically amongst the grid rows.
//
// When a grid row needs to determine its horizontal requirements, it must
// invoke all other rows in the grid to do the same, since the width of each
// column could be determined by any row in the grid. Otherwise, grid rows act
// like normal rows.
//
// Grids come in two forms: uniform and nonuniform. Since the two are so
// similar, they're built on the same generic implementation.

// The following serve as tags for selecting the proper grid behavior.
struct nonuniform_grid_tag {};
struct uniform_grid_tag {};

// This structure stores the layout requirements for the columns in a grid.
template<class Uniformity>
struct grid_column_requirements
{
};
// In nonuniform grids, each column has its own requirements.
template<>
struct grid_column_requirements<nonuniform_grid_tag>
{
    std::vector<layout_requirements> columns;
};
// In uniform grids, the columns all share the same requirements.
template<>
struct grid_column_requirements<uniform_grid_tag>
{
    size_t n_columns;
    layout_requirements requirements;
};

// Get the number of columns.
static size_t
get_column_count(grid_column_requirements<nonuniform_grid_tag> const& x)
{
    return x.columns.size();
}
static size_t
get_column_count(grid_column_requirements<uniform_grid_tag> const& x)
{
    return x.n_columns;
}

// Reset the column requirements.
static void
clear_requirements(grid_column_requirements<nonuniform_grid_tag>& x)
{
    x.columns.clear();
}
static void
clear_requirements(grid_column_requirements<uniform_grid_tag>& x)
{
    x.n_columns = 0;
    x.requirements = layout_requirements(0, 0, 0, 1);
}

// Add the requirements for a column.
static void
add_requirements(grid_column_requirements<nonuniform_grid_tag>& x,
    layout_requirements const& addition)
{
    x.columns.push_back(addition);
}
static void
add_requirements(grid_column_requirements<uniform_grid_tag>& x,
    layout_requirements const& addition)
{
    ++x.n_columns;
    fold_in_requirements(x.requirements, addition);
}

// Fold the second set of requirements into the first.
static void
fold_in_requirements(grid_column_requirements<nonuniform_grid_tag>& x,
    grid_column_requirements<nonuniform_grid_tag> const& y)
{
    size_t n_columns = get_column_count(y);
    if (get_column_count(x) < n_columns)
        x.columns.resize(n_columns, layout_requirements(0, 0, 0, 0));
    for (size_t i = 0; i != n_columns; ++i)
    {
        layout_requirements& xi = x.columns[i];
        layout_requirements const& yi = y.columns[i];
        fold_in_requirements(xi, yi);
        if (xi.growth_factor < yi.growth_factor)
            xi.growth_factor = yi.growth_factor;
    }
}
static void
fold_in_requirements(grid_column_requirements<uniform_grid_tag>& x,
    grid_column_requirements<uniform_grid_tag> const& y)
{
    if (x.n_columns < y.n_columns)
        x.n_columns = y.n_columns;
    fold_in_requirements(x.requirements, y.requirements);
}

// Get the requirements for the nth column.
static layout_requirements const&
get_column_requirements(
    grid_column_requirements<nonuniform_grid_tag> const& x, size_t n)
{
    return x.columns[n];
}
static layout_requirements const&
get_column_requirements(
    grid_column_requirements<uniform_grid_tag> const& x, size_t n)
{
    return x.requirements;
}

template<class Uniformity>
struct grid_row_container;

template<class Uniformity>
struct grid_data
{
    // unique ID of this grid
    counter_type id;

    // the container that contains the whole grid
    layout_container* container;

    // list of rows in the grid
    grid_row_container<Uniformity>* rows;

    // spacing between columns
    layout_scalar column_spacing;

    // requirements for the columns
    grid_column_requirements<Uniformity> requirements;
    counter_type last_content_query;

    grid_data() : id(0), last_content_query(0) {}
};

template<class Uniformity>
struct grid_row_container : layout_container
{
    // implementation of layout interface
    layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx);
    layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_scalar assigned_width);
    void set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment);

    void record_change(layout_traversal& traversal);
    void record_self_change(layout_traversal& traversal);

    layout_cacher cacher;

    // cached requirements for cells within this row
    grid_column_requirements<Uniformity> requirements;
    counter_type last_content_query;

    // reference to the data for the grid that this row belongs to
    grid_data<Uniformity>* grid;

    // next row in this grid
    grid_row_container* next;

    grid_row_container() : last_content_query(0) {}
};

// Update the requirements for a grid's columns by querying its contents.
template<class Uniformity>
void update_grid_column_requirements(
    layout_calculation_context& ctx, grid_data<Uniformity>& grid)
{
    named_block nb;
    ALIA_BEGIN_LOCATION_SPECIFIC_NAMED_BLOCK(ctx, nb, make_id(grid.id));
    // Only update if something in the grid has changed since the last update.
    alia_if (grid.last_content_query != grid.container->last_content_change)
    {
        naming_context nc(ctx);
        // Clear the requirements for the grid and recompute them by iterating
        // through the rows and folding each row's requirements into the main
        // grid requirements.
        clear_requirements(grid.requirements);
        for (grid_row_container<Uniformity>* row = grid.rows; row;
            row = row->next)
        {
            named_block nb(nc, make_id(row));
            // Again, only update if something in the row has changed.
            alia_if (row->last_content_query != row->last_content_change)
            {
                clear_requirements(row->requirements);
                for (layout_node* child = row->children; child;
                    child = child->next)
                {
                    layout_requirements x =
                        alia::get_horizontal_requirements(ctx, *child);
                    add_requirements(row->requirements, x);
                }
                row->last_content_query = row->last_content_change;
            }
            alia_end
            fold_in_requirements(grid.requirements, row->requirements);
        }
        grid.last_content_query = grid.container->last_content_change;
    }
    alia_end
}

template<class Uniformity>
layout_scalar
get_required_width(grid_data<Uniformity> const& grid)
{
    size_t n_columns = get_column_count(grid.requirements);
    layout_scalar width = 0;
    for (size_t i = 0; i != n_columns; ++i)
        width += get_column_requirements(grid.requirements, i).size;
    if (n_columns > 0)
        width += grid.column_spacing * layout_scalar(n_columns - 1);
    return width;
}

template<class Uniformity>
float
get_total_growth(grid_data<Uniformity> const& grid)
{
    size_t n_columns = get_column_count(grid.requirements);
    float growth = 0;
    for (size_t i = 0; i != n_columns; ++i)
        growth += get_column_requirements(grid.requirements, i).growth_factor;
    return growth;
}

template<class Uniformity>
layout_requirements
grid_row_container<Uniformity>::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    horizontal_layout_query query(ctx, cacher,
        grid->container->last_content_change);
    alia_if (query.update_required())
    {
        update_grid_column_requirements(ctx, *grid);
        query.update(calculated_layout_requirements(
            get_required_width(*grid), 0, 0));
    }
    alia_end
    return query.result();
}

struct cached_grid_column_assignments
{
    counter_type last_update;
    std::vector<layout_scalar> assignments;
    cached_grid_column_assignments() : last_update(0) {}
};

template<class Uniformity>
std::vector<layout_scalar> const&
calculate_column_assignments(
    layout_calculation_context& ctx,
    grid_data<Uniformity>& grid,
    layout_scalar assigned_width)
{
    named_block nb;
    ALIA_BEGIN_LOCATION_SPECIFIC_NAMED_BLOCK(ctx, nb,
        combine_ids(make_id(grid.id), make_id(assigned_width)));
    cached_grid_column_assignments* cache;
    get_cached_data(ctx, &cache);
    if (cache->last_update != grid.container->last_content_change)
    {
        update_grid_column_requirements(ctx, grid);
        size_t n_columns = get_column_count(grid.requirements);
        cache->assignments.resize(n_columns);
        layout_scalar required_width = get_required_width(grid);
        float total_growth = get_total_growth(grid);;
        layout_scalar extra_width = assigned_width - required_width;
        for (size_t i = 0; i != n_columns; ++i)
        {
            layout_scalar width =
                get_column_requirements(grid.requirements, i).size;
            if (total_growth != 0)
            {
                float growth_factor = get_column_requirements(
                    grid.requirements, i).growth_factor;
                layout_scalar extra = round_to_layout_scalar(
                    (growth_factor / total_growth) * extra_width);
                extra_width -= extra;
                total_growth -= growth_factor;
                width += extra;
            }
            cache->assignments[i] = width;
        }
        cache->last_update = grid.container->last_content_change;
    }
    return cache->assignments;
}

calculated_layout_requirements
calculate_grid_row_vertical_requirements(
    layout_calculation_context& ctx,
    grid_data<nonuniform_grid_tag>& grid,
    grid_row_container<nonuniform_grid_tag>& row,
    layout_scalar assigned_width)
{
    std::vector<layout_scalar> const& column_widths =
        calculate_column_assignments(ctx, grid, assigned_width);
    calculated_layout_requirements requirements(0, 0, 0);
    size_t column_index = 0;
    for (layout_node* i = row.children; i; i = i->next, ++column_index)
    {
        fold_in_requirements(requirements,
            get_vertical_requirements(ctx, *i, column_widths[column_index]));
    }
    return requirements;
}

struct cached_uniform_grid_vertical_requirements
{
    calculated_layout_requirements requirements;
    counter_type last_update;
    cached_uniform_grid_vertical_requirements() : last_update(0) {}
};
calculated_layout_requirements
calculate_grid_row_vertical_requirements(
    layout_calculation_context& ctx,
    grid_data<uniform_grid_tag>& grid,
    grid_row_container<uniform_grid_tag>& _, // row is irrelevant
    layout_scalar assigned_width)
{
    named_block nb;
    ALIA_BEGIN_LOCATION_SPECIFIC_NAMED_BLOCK(ctx, nb,
        combine_ids(make_id(grid.id), make_id(assigned_width)));
    cached_uniform_grid_vertical_requirements* cache;
    get_cached_data(ctx, &cache);
    if (cache->last_update != grid.container->last_content_change)
    {
        update_grid_column_requirements(ctx, grid);

        std::vector<layout_scalar> const& widths =
            calculate_column_assignments(ctx, grid, assigned_width);

        calculated_layout_requirements& grid_requirements =
            cache->requirements;
        grid_requirements = calculated_layout_requirements(0, 0, 0);
        for (grid_row_container<uniform_grid_tag>* row = grid.rows;
            row; row = row->next)
        {
            size_t column_index = 0;
            for (layout_node* child = row->children; child;
                child = child->next, ++column_index)
            {
                fold_in_requirements(grid_requirements,
                    alia::get_vertical_requirements(
                        ctx, *child, widths[column_index]));
            }
        }

        cache->last_update = grid.container->last_content_change;
    }
    return cache->requirements;}

template<class Uniformity>
layout_requirements
grid_row_container<Uniformity>::get_vertical_requirements(
    layout_calculation_context& ctx, layout_scalar assigned_width)
{
    vertical_layout_query query(ctx, cacher,
        grid->container->last_content_change, assigned_width);
    alia_if (query.update_required())
    {
        query.update(
            calculate_grid_row_vertical_requirements(
                ctx, *grid, *this, assigned_width));
    }
    alia_end
    return query.result();
}

template<class Uniformity>
void
set_grid_row_relative_assignment(
    layout_calculation_context& ctx,
    grid_data<Uniformity>& grid,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    std::vector<layout_scalar> const& column_widths =
        calculate_column_assignments(ctx, grid, assigned_size[0]);
    size_t n = 0;
    layout_vector p = make_layout_vector(0, 0);
    for (layout_node* i = children; i; i = i->next, ++n)
    {
        layout_scalar this_width = column_widths[n];
        alia::set_relative_assignment(
            ctx, *i,
            relative_layout_assignment(
                layout_box(p,
                    make_layout_vector(this_width, assigned_size[1])),
                assigned_baseline_y));
        p[0] += this_width + grid.column_spacing;
    }
}

template<class Uniformity>
void
grid_row_container<Uniformity>::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    relative_region_assignment rra(ctx, *this, cacher,
        grid->container->last_content_change, assignment);
    alia_if (rra.update_required())
    {
        set_grid_row_relative_assignment(
            ctx, *grid, children,
            rra.resolved_assignment().region.size,
            rra.resolved_assignment().baseline_y);
        rra.update();
    }
    alia_end
}

template<class Uniformity>
void
grid_row_container<Uniformity>::record_change(layout_traversal& traversal)
{
    if (this->last_content_change != traversal.refresh_counter)
    {
        this->last_content_change = traversal.refresh_counter;
        if (this->parent)
            this->parent->record_change(traversal);
        grid_data<Uniformity>& grid = *this->grid;
        for (grid_row_container<Uniformity>* row = grid.rows;
            row; row = row->next)
        {
            row->record_self_change(traversal);
        }
    }
}

template<class Uniformity>
void
grid_row_container<Uniformity>::record_self_change(layout_traversal& traversal)
{
    if (this->last_content_change != traversal.refresh_counter)
    {
        this->last_content_change = traversal.refresh_counter;
        if (this->parent)
            this->parent->record_change(traversal);
    }
}

template<class Uniformity>
void refresh_grid(layout_traversal& traversal, grid_data<Uniformity>& data)
{
    if (traversal.is_refresh_pass)
    {
        if (!data.id)
            data.id = get_cacher_id(*traversal.system);
        // Reset the row list.
        data.rows = 0;
    }
}

template<class Uniformity>
void refresh_grid_row(layout_traversal& traversal, grid_data<Uniformity>& grid,
    grid_row_container<Uniformity>& row, layout const& layout_spec)
{
    // Add this row to the grid's list of rows.
    // It doesn't matter what order the list is in, and adding the row to the
    // front of the list is easier.
    if (traversal.is_refresh_pass)
    {
        row.next = grid.rows;
        grid.rows = &row;
        row.grid = &grid;
    }

    update_layout_cacher(traversal, row.cacher, layout_spec, FILL | UNPADDED);
}

// NONUNIFORM GRID

struct grid_layout_data : grid_data<nonuniform_grid_tag>
{};

void grid_layout::concrete_begin(
    layout_traversal& traversal, data_traversal& data,
    layout const& layout_spec, absolute_length const& column_spacing)
{
    traversal_ = &traversal;
    data_traversal_ = &data;

    get_cached_data(data, &data_);
    refresh_grid(traversal, *data_);

    simple_layout_container* container;
    column_layout_logic* logic;
    get_simple_layout_container(traversal, data, &container, &logic,
        layout_spec);
    container_.begin(traversal, container);

    begin_layout_transform(transform_, traversal, container->cacher);

    data_->container = container;

    layout_scalar resolved_spacing =
        as_layout_size(resolve_absolute_length(traversal, 0, column_spacing));
    detect_layout_change(traversal, &data_->column_spacing, resolved_spacing);
}

void grid_row::begin(grid_layout const& grid, layout const& layout_spec)
{
    layout_traversal& traversal = *grid.traversal_;

    grid_row_container<nonuniform_grid_tag>* row;
    if (get_cached_data(*grid.data_traversal_, &row))
        initialize(traversal, *row);

    refresh_grid_row(traversal, *grid.data_, *row, layout_spec);

    container_.begin(traversal, row);

    begin_layout_transform(transform_, traversal, row->cacher);
}

void grid_row::end()
{
    transform_.end();
    container_.end();
}

// UNIFORM GRID

struct uniform_grid_layout_data : grid_data<uniform_grid_tag>
{};

void uniform_grid_layout::concrete_begin(
    layout_traversal& traversal, data_traversal& data,
    layout const& layout_spec, absolute_length const& column_spacing)
{
    traversal_ = &traversal;
    data_traversal_ = &data;

    get_cached_data(*data_traversal_, &data_);
    refresh_grid(traversal, *data_);

    simple_layout_container* container;
    column_layout_logic* logic;
    get_simple_layout_container(traversal, data, &container, &logic,
        layout_spec);
    container_.begin(traversal, container);

    begin_layout_transform(transform_, traversal, container->cacher);

    data_->container = container;

    layout_scalar resolved_spacing =
        as_layout_size(resolve_absolute_length(traversal, 0, column_spacing));
    detect_layout_change(traversal, &data_->column_spacing, resolved_spacing);
}

void uniform_grid_row::begin(
    uniform_grid_layout const& grid, layout const& layout_spec)
{
    layout_traversal& traversal = *grid.traversal_;

    grid_row_container<uniform_grid_tag>* row;
    if (get_cached_data(*grid.data_traversal_, &row))
        initialize(traversal, *row);

    // All rows must be set to GROW to ensure they receive equal space from
    // the column that contains them.
    refresh_grid_row(traversal, *grid.data_, *row, GROW | UNPADDED);

    container_.begin(traversal, row);

    begin_layout_transform(transform_, traversal, row->cacher);
}

void uniform_grid_row::end()
{
    transform_.end();
    container_.end();
}

// FLOATING LAYOUT

struct floating_layout_data
{
    data_graph measurement_cache, placement_cache;
    layout_vector size;
    floating_layout_data() {}
};

void floating_layout::concrete_begin(
    layout_traversal& traversal, data_traversal& data,
    layout_vector const& min_size, layout_vector const& max_size)
{
    get_cached_data(data, &data_);

    traversal_ = &traversal;

    if (traversal.is_refresh_pass)
    {
        old_container_ = traversal.active_container;
        old_next_ptr_ = traversal.next_ptr;

        traversal.active_container = 0;
        traversal.next_ptr = &floating_root_;

        min_size_ = min_size;
        max_size_ = max_size;
    }

    if (traversal.geometry)
        clipping_reset_.begin(*traversal.geometry);
}

void floating_layout::end()
{
    if (traversal_)
    {
        layout_traversal& traversal = *traversal_;

        if (traversal.is_refresh_pass)
        {
            traversal_->active_container = old_container_;
            traversal_->next_ptr = old_next_ptr_;

            layout_vector measured_size =
                get_minimum_size(floating_root_, data_->measurement_cache);
            for (unsigned i = 0; i != 2; ++i)
            {
                data_->size[i] = measured_size[i];
                if (min_size_[i] >= 0 && data_->size[i] < min_size_[i])
                    data_->size[i] = min_size_[i];
                if (max_size_[i] >= 0 && data_->size[i] > max_size_[i])
                    data_->size[i] = max_size_[i];
            }
            resolve_layout(floating_root_, data_->placement_cache,
                data_->size);
        }

        clipping_reset_.end();

        traversal_ = 0;
    }
}

layout_vector floating_layout::size() const
{
    return data_->size;
}

}
