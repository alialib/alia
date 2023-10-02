#ifndef ALIA_INDIE_LAYOUT_LOGIC_FLOW_HPP
#define ALIA_INDIE_LAYOUT_LOGIC_FLOW_HPP

#include <alia/indie/layout/logic/utilities.hpp>
#include <alia/indie/layout/specification.hpp>

namespace alia { namespace indie {

struct flow_layout_logic
{
    layout_flag_set x_alignment;

    struct wrapped_row
    {
        layout_scalar width;
        layout_requirements requirements;
        layout_scalar y;
    };

    struct wrapping_state
    {
        std::vector<wrapped_row>* rows;
        layout_scalar assigned_width;
        wrapped_row active_row;
        layout_scalar accumulated_width;
        layout_scalar visible_width;
    };

    struct wrapping_assignment_state
    {
        layout_scalar assigned_width;
        layout_flag_set x_alignment;
        std::vector<wrapped_row>::iterator active_row, end_row;
        layout_scalar x;
    };

    template<class Children>
    calculated_layout_requirements
    get_horizontal_requirements(Children&& children)
    {
        // In the worst case scenario, we can put one child on each row, so the
        // required width is simply the minimal width required by the widest
        // child.
        calculated_layout_requirements requirements{
            layout_scalar(0), layout_scalar(0), layout_scalar(0)};
        walk_layout_nodes(
            std::forward<Children>(children),
            [&](layout_node_interface& node) {
                fold_in_requirements(
                    requirements, node.get_horizontal_requirements());
            });
        return requirements;
    }

    static void
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

    static layout_scalar
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

    static void
    wrap_row(wrapping_assignment_state& state)
    {
        ++state.active_row;
        state.x
            = state.active_row != state.end_row ? calculate_initial_x(
                  state.assigned_width, state.x_alignment, *state.active_row)
                                                : 0;
    }

    static void
    calculate_node_wrapping(wrapping_state& state, layout_node_interface& node)
    {
        layout_requirements x = node.get_horizontal_requirements();
        if (state.accumulated_width + x.size > state.assigned_width)
            wrap_row(state);
        layout_requirements y = node.get_vertical_requirements(x.size);
        state.visible_width += x.size;
        state.accumulated_width += x.size;
        fold_in_requirements(state.active_row.requirements, y);
    }

    template<class Children>
    static layout_scalar
    calculate_wrapping(
        Children&& children,
        layout_scalar assigned_width,
        std::vector<wrapped_row>* rows)
    {
        wrapping_state state;
        state.assigned_width = assigned_width;
        state.accumulated_width = 0;
        state.active_row.requirements = layout_requirements{0, 0, 0, 0};
        state.active_row.y = 0;
        state.rows = rows;

        walk_layout_nodes(
            std::forward<Children>(children),
            [&](layout_node_interface& node) {
                calculate_node_wrapping(state, node);
            });
        // Include the last/current row in the height requirements.
        wrap_row(state);

        return state.active_row.y;
    }

    template<class Children>
    calculated_layout_requirements
    get_vertical_requirements(
        Children&& children, layout_scalar assigned_width)
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

        return calculated_layout_requirements{total_height, ascent, descent};
    }

    static void
    assign_wrapped_regions(
        wrapping_assignment_state& state, layout_node_interface& node)
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

    template<class Children>
    void
    set_relative_assignment(
        Children&& children,
        layout_vector const& assigned_size,
        layout_scalar /*assigned_baseline_y*/)
    {
        // First, compute the wrapping for the assigned width so that we know
        // the vertical requirements on each row.
        std::vector<wrapped_row> rows;
        calculate_wrapping(
            std::forward<Children>(children), assigned_size[0], &rows);

        // Now actually do the assignments.
        wrapping_assignment_state state;
        state.x = !rows.empty() ? calculate_initial_x(
                      assigned_size[0], x_alignment, rows.front())
                                : 0;
        state.assigned_width = assigned_size[0];
        state.active_row = rows.begin();
        state.end_row = rows.end();
        state.x_alignment = x_alignment;
        walk_layout_nodes(
            std::forward<Children>(children),
            [&](layout_node_interface& node) {
                assign_wrapped_regions(state, node);
            });
    }
};

// The vertical flow algorithm is a bit simplistic. All columns have the
// same width.
struct vertical_flow_layout_logic
{
    template<class Children>
    calculated_layout_requirements
    get_horizontal_requirements(Children&& children)
    {
        // In the worst case scenario, we can put all children in one column,
        // so the required width is simply the width required by the widest
        // child.
        return fold_horizontal_child_requirements(children);
    }

    template<class Children>
    calculated_layout_requirements
    get_vertical_requirements(
        Children&& children, layout_scalar assigned_width)
    {
        layout_scalar column_width
            = get_max_child_width(std::forward<Children>(children));

        layout_scalar total_height = compute_total_height(
            std::forward<Children>(children), column_width);

        int n_columns = (std::max)(int(assigned_width / column_width), 1);

        layout_scalar average_column_height = total_height / n_columns;

        // Break the children into columns and determine the longest column
        // size. Each column accumulates children until it's at least as big as
        // the average height. There are other strategies that would yield
        // smaller and more balanced placements, but this one is simple and
        // gives reasonable results.
        layout_scalar max_column_height = 0;
        layout_scalar current_column_height = 0;
        walk_layout_nodes(
            std::forward<Children>(children),
            [&](layout_node_interface& node) {
                if (current_column_height >= average_column_height)
                {
                    if (current_column_height > max_column_height)
                        max_column_height = current_column_height;
                    current_column_height = 0;
                }
                layout_requirements y
                    = node.get_vertical_requirements(column_width);
                current_column_height += y.size;
            });

        return calculated_layout_requirements(max_column_height, 0, 0);
    }

    template<class Children>
    void
    set_relative_assignment(
        Children&& children,
        layout_vector const& assigned_size,
        layout_scalar /*assigned_baseline_y*/)
    {
        layout_scalar column_width
            = get_max_child_width(std::forward<Children>(children));

        layout_scalar total_height = compute_total_height(
            std::forward<Children>(children), column_width);

        int n_columns = (std::max)(int(assigned_size[0] / column_width), 1);

        layout_scalar average_column_height = total_height / n_columns;

        // Break the children into columns and assign their regions.
        // Each column accumulates children until it's at least as big as the
        // average height. There are other strategies that would yield smaller
        // and more balanced placements, but this one is simple and gives
        // reasonable results.
        layout_vector p = make_layout_vector(0, 0);
        walk_layout_nodes(
            std::forward<Children>(children),
            [&](layout_node_interface& node) {
                if (p[1] >= average_column_height)
                {
                    p[0] += column_width;
                    p[1] = 0;
                }
                layout_requirements y
                    = node.get_vertical_requirements(column_width);
                node.set_relative_assignment(relative_layout_assignment{
                    layout_box(p, make_layout_vector(column_width, y.size)),
                    y.ascent});
                p[1] += y.size;
            });
    }
};

}} // namespace alia::indie

#endif
