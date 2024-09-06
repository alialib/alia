#include <alia/ui/layout/simple.hpp>

#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

calculated_layout_requirements
flow_layout_logic::get_horizontal_requirements(layout_node* children)
{
    // In the worst case scenario, we can put one child on each row, so the
    // required width is simply the minimal width required by the widest
    // child.
    calculated_layout_requirements requirements(0, 0, 0);
    walk_layout_children(children, [&](layout_node& node) {
        fold_in_requirements(requirements, node.get_horizontal_requirements());
    });
    return requirements;
}

void
wrap_row(wrapping_state& state)
{
    state.active_row.width = state.visible_width;
    state.active_row.requirements.size = (std::max)(
        state.active_row.requirements.size,
        state.active_row.requirements.ascent
            + state.active_row.requirements.descent);
    if (state.rows)
        state.rows->push_back(state.active_row);
    state.active_row.y += state.active_row.requirements.size;
    state.active_row.requirements = layout_requirements{0, 0, 0, 0};
    state.accumulated_width = state.visible_width = 0;
}
layout_scalar
calculate_initial_x(
    layout_scalar assigned_width,
    layout_flag_set x_alignment,
    wrapped_row const& row)
{
    switch (x_alignment.code)
    {
        case RIGHT_CODE:
            return assigned_width - row.width;
        case CENTER_X_CODE:
            return (assigned_width - row.width) / 2;
        default:
            return 0;
    }
}
void
wrap_row(wrapping_assignment_state& state)
{
    ++state.active_row;
    state.x
        = state.active_row != state.end_row
              ? calculate_initial_x(
                    state.assigned_width, state.x_alignment, *state.active_row)
              : 0;
}

void
calculate_node_wrapping(wrapping_state& state, layout_node& node)
{
    layout_requirements x = node.get_horizontal_requirements();
    if (state.accumulated_width + x.size > state.assigned_width)
        wrap_row(state);
    layout_requirements y = node.get_vertical_requirements(x.size);
    state.visible_width += x.size;
    state.accumulated_width += x.size;
    fold_in_requirements(state.active_row.requirements, y);
}

static layout_scalar
calculate_wrapping(
    layout_node* children,
    layout_scalar assigned_width,
    std::vector<wrapped_row>* rows)
{
    wrapping_state state;
    state.assigned_width = assigned_width;
    state.accumulated_width = 0;
    state.active_row.requirements = layout_requirements{0, 0, 0, 0};
    state.active_row.y = 0;
    state.rows = rows;

    walk_layout_children(children, [&](layout_node& node) {
        calculate_node_wrapping(state, node);
    });
    // Include the last/current row in the height requirements.
    wrap_row(state);

    return state.active_row.y;
}

calculated_layout_requirements
flow_layout_logic::get_vertical_requirements(
    layout_node* children, layout_scalar assigned_width)
{
    std::vector<wrapped_row> rows;
    layout_scalar total_height
        = calculate_wrapping(children, assigned_width, &rows);

    // Similar to the column, the flow layout uses the baseline of the
    // first row as the baseline of the whole layout (if there is one).
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

void
assign_wrapped_regions(wrapping_assignment_state& state, layout_node& node)
{
    layout_requirements x = node.get_horizontal_requirements();
    if (state.x + x.size > state.assigned_width)
        wrap_row(state);
    layout_scalar row_height = state.active_row->requirements.size;
    node.set_relative_assignment(relative_layout_assignment{
        layout_box(
            make_layout_vector(state.x, state.active_row->y),
            make_layout_vector(x.size, row_height)),
        state.active_row->requirements.ascent});
    state.x += x.size;
}

void
flow_layout_logic::set_relative_assignment(
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar /*assigned_baseline_y*/)
{
    // First, compute the wrapping for the assigned width so that we know
    // the vertical requirements on each row.
    std::vector<wrapped_row> rows;
    calculate_wrapping(children, assigned_size[0], &rows);

    // Now actually do the assignments.
    wrapping_assignment_state state;
    state.x = !rows.empty() ? calculate_initial_x(
                                  assigned_size[0], x_alignment_, rows.front())
                            : 0;
    state.assigned_width = assigned_size[0];
    state.active_row = rows.begin();
    state.end_row = rows.end();
    state.x_alignment = x_alignment_;
    walk_layout_children(children, [&](layout_node& node) {
        assign_wrapped_regions(state, node);
    });
}

void
flow_layout::concrete_begin(
    layout_traversal& traversal,
    data_traversal& data,
    layout const& requested_layout_spec)
{
    // With a flow layout, we want to have the layout itself always fill
    // the horizontal space and use the requested X alignment to position
    // the individual rows in the flow.
    auto layout_spec = add_default_padding(requested_layout_spec, PADDED);
    layout_flag_set x_alignment = FILL_X;
    if ((layout_spec.flags.code & 0x3) != 0)
    {
        x_alignment.code = layout_spec.flags.code & X_ALIGNMENT_MASK_CODE;
        layout_spec.flags.code &= ~X_ALIGNMENT_MASK_CODE;
        layout_spec.flags.code |= FILL_X_CODE;
    }
    ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(flow_layout_logic);
    logic->x_alignment_ = x_alignment;
}

} // namespace alia
