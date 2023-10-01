#ifndef ALIA_INDIE_LAYOUT_LOGIC_HPP
#define ALIA_INDIE_LAYOUT_LOGIC_HPP

#include <alia/indie/layout/api.hpp>

namespace alia { namespace indie {

template<class Children>
void
compute_total_width_and_growth(
    layout_scalar* total_width, float* total_growth, Children&& children)
{
    *total_width = 0;
    *total_growth = 0;
    walk_layout_nodes(
        std::forward<Children>(children), [&](layout_node& node) {
            layout_requirements r = node.get_horizontal_requirements();
            *total_width += r.size;
            *total_growth += r.growth_factor;
        });
}

inline layout_scalar
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
    {
        return this_required_size;
    }
}

struct row_layout_logic
{
    template<class Children>
    calculated_layout_requirements
    get_horizontal_requirements(Children&& children)
    {
        layout_scalar total_size;
        float total_growth;
        compute_total_width_and_growth(
            &total_size, &total_growth, std::forward<Children>(children));
        return calculated_layout_requirements{total_size, 0, 0};
    }

    template<class Children>
    calculated_layout_requirements
    get_vertical_requirements(
        Children&& children, layout_scalar assigned_width)
    {
        layout_scalar total_size;
        float total_growth;
        compute_total_width_and_growth(
            &total_size, &total_growth, std::forward<Children>(children));
        layout_scalar remaining_extra_size = assigned_width - total_size;
        float remaining_growth = total_growth;
        calculated_layout_requirements requirements{0, 0, 0};
        walk_layout_nodes(children, [&](layout_node& node) {
            layout_requirements x = node.get_horizontal_requirements();
            layout_scalar this_width = calculate_child_size(
                remaining_extra_size,
                remaining_growth,
                x.size,
                x.growth_factor);
            fold_in_requirements(
                requirements, node.get_vertical_requirements(this_width));
        });
        return requirements;
    }

    template<class Children>
    void
    set_relative_assignment(
        Children&& children,
        layout_vector const& assigned_size,
        layout_scalar assigned_baseline_y)
    {
        layout_scalar total_size;
        float total_growth;
        compute_total_width_and_growth(
            &total_size, &total_growth, std::forward<Children>(children));
        layout_vector p = make_layout_vector(0, 0);
        layout_scalar remaining_extra_size = assigned_size[0] - total_size;
        float remaining_growth = total_growth;
        walk_layout_nodes(
            std::forward<Children>(children), [&](layout_node& node) {
                layout_requirements x = node.get_horizontal_requirements();
                layout_scalar this_width = calculate_child_size(
                    remaining_extra_size,
                    remaining_growth,
                    x.size,
                    x.growth_factor);
                node.set_relative_assignment(relative_layout_assignment{
                    layout_box(
                        p, make_layout_vector(this_width, assigned_size[1])),
                    assigned_baseline_y});
                p[0] += this_width;
            });
    }
};

// COLUMN LAYOUT

struct column_layout_logic
{
    template<class Children>
    calculated_layout_requirements
    get_horizontal_requirements(Children&& children)
    {
        return fold_horizontal_child_requirements(
            std::forward<Children>(children));
    }

    template<class Children>
    calculated_layout_requirements
    get_vertical_requirements(
        Children&& children, layout_scalar assigned_width)
    {
        layout_scalar total_height = 0;
        layout_scalar ascent = 0, descent = 0;
        bool is_first = true;
        walk_layout_nodes(
            std::forward<Children>(children), [&](layout_node& node) {
                layout_requirements r
                    = node.get_vertical_requirements(assigned_width);
                total_height += r.size;
                // The ascent of a column is the ascent of its first child.
                // Its descent is the descent of its first child plus the total
                // height of all other children. However, if the first child
                // doesn't care about the baseline, then the column ignores it
                // as well.
                if (is_first)
                {
                    ascent = r.ascent;
                    descent = r.descent;
                    is_first = false;
                }
                else if (ascent != 0 || descent != 0)
                {
                    descent += r.size;
                }
            });
        return calculated_layout_requirements{total_height, ascent, descent};
    }

    template<class Children>
    void
    set_relative_assignment(
        Children&& children,
        layout_vector const& assigned_size,
        layout_scalar assigned_baseline_y)
    {
        layout_scalar total_size = 0;
        float total_growth = 0;
        walk_layout_nodes(
            std::forward<Children>(children),
            [&](layout_node_interface& node) {
                layout_requirements x
                    = node.get_vertical_requirements(assigned_size[0]);
                total_size += x.size;
                total_growth += x.growth_factor;
            });
        layout_vector p = make_layout_vector(0, 0);
        layout_scalar remaining_extra_size = assigned_size[1] - total_size;
        float remaining_growth = total_growth;
        bool is_first = true;
        walk_layout_nodes(
            std::forward<Children>(children),
            [&](layout_node_interface& node) {
                layout_requirements y
                    = node.get_vertical_requirements(assigned_size[0]);
                layout_scalar this_height = calculate_child_size(
                    remaining_extra_size,
                    remaining_growth,
                    y.size,
                    y.growth_factor);
                // If this is the first child, assign it the baseline of the
                // column. The exception to this rule is when the column was
                // allocated more space than it requested. In this case, the
                // baseline calculation may have been inconsistent with how the
                // column is planning to allocate the extra space.
                layout_scalar this_baseline
                    = is_first && remaining_extra_size == 0
                          ? assigned_baseline_y
                          : y.ascent;
                node.set_relative_assignment(relative_layout_assignment{
                    layout_box(
                        p, make_layout_vector(assigned_size[0], this_height)),
                    this_baseline});
                p[1] += this_height;
                is_first = false;
            });
    }
};

struct layered_layout_logic
{
    template<class Children>
    calculated_layout_requirements
    get_horizontal_requirements(Children&& children)
    {
        return fold_horizontal_child_requirements(
            std::forward<Children>(children));
    }

    template<class Children>
    calculated_layout_requirements
    get_vertical_requirements(
        Children&& children, layout_scalar assigned_width)
    {
        return fold_vertical_child_requirements(
            std::forward<Children>(children), assigned_width);
    }

    template<class Children>
    void
    set_relative_assignment(
        Children&& children,
        layout_vector const& assigned_size,
        layout_scalar assigned_baseline_y)
    {
        assign_identical_child_regions(
            std::forward<Children>(children),
            assigned_size,
            assigned_baseline_y);
    }
};

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
            std::forward<Children>(children), [&](layout_node& node) {
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
            [&](layout_node& node) { calculate_node_wrapping(state, node); });
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
            [&](layout_node& node) { assign_wrapped_regions(state, node); });
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
            std::forward<Children>(children), [&](layout_node& node) {
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
            std::forward<Children>(children), [&](layout_node& node) {
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

struct clamped_layout_logic
{
    layout_vector max_size;

    template<class Children>
    calculated_layout_requirements
    get_horizontal_requirements(Children&& children)
    {
        calculated_layout_requirements requirements
            = fold_horizontal_child_requirements(
                std::forward<Children>(children));
        return requirements;
    }

    template<class Children>
    calculated_layout_requirements
    get_vertical_requirements(
        Children&& children, layout_scalar assigned_width)
    {
        calculated_layout_requirements requirements
            = fold_horizontal_child_requirements(
                std::forward<Children>(children));
        layout_scalar clamped_width = (std::max)(
            requirements.size,
            this->max_size[0] <= 0
                ? assigned_width
                : (std::min)(assigned_width, this->max_size[0]));
        return fold_vertical_child_requirements(
            std::forward<Children>(children), clamped_width);
    }

    template<class Children>
    void
    set_relative_assignment(
        Children&& children,
        layout_vector const& assigned_size,
        layout_scalar /*assigned_baseline_y*/)
    {
        layout_vector clamped_size;
        for (int i = 0; i != 2; ++i)
        {
            clamped_size[i]
                = this->max_size[i] <= 0
                      ? assigned_size[i]
                      : (std::min)(assigned_size[i], this->max_size[i]);
        }
        walk_layout_nodes(
            std::forward<Children>(children), [&](layout_node& node) {
                layout_requirements y
                    = node.get_vertical_requirements(clamped_size[0]);
                node.set_relative_assignment(relative_layout_assignment{
                    layout_box(
                        (assigned_size - clamped_size) / 2, clamped_size),
                    y.ascent});
            });
    }
};

}} // namespace alia::indie

#endif
