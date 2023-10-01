#include <alia/indie/layout/library.hpp>

#include <algorithm>
#include <vector>

#include <alia/core/flow/macros.hpp>

#include <alia/indie/layout/logic.hpp>
#include <alia/indie/layout/system.hpp>
#include <alia/indie/layout/traversal.hpp>
#include <alia/indie/layout/utilities.hpp>

// This file implements the standard library of layout containers (and the
// spacer, which is the only standard leaf).

namespace alia { namespace indie {

void
scoped_layout_container::begin(
    layout_traversal<layout_container, layout_node>& traversal,
    layout_container* container)
{
    if (traversal.is_refresh_pass)
    {
        traversal_ = &traversal;

        set_next_node(traversal, container);
        container->parent = traversal.active_container;

        traversal.next_ptr = &container->children;
        traversal.active_container = container;
    }
}
void
scoped_layout_container::end()
{
    if (traversal_)
    {
        set_next_node(*traversal_, 0);

        layout_container* container = traversal_->active_container;
        traversal_->next_ptr = &container->next;
        traversal_->active_container = container->parent;

        traversal_ = 0;
    }
}

// void
// do_spacer(
//     layout_traversal& traversal,
//     data_traversal& data,
//     layout const& layout_spec)
// {
//     layout_leaf* node;
//     get_cached_data(data, &node);

//     if (traversal.is_refresh_pass)
//     {
//         node->refresh_layout(
//             traversal,
//             layout_spec,
//             leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
//         add_layout_node(traversal, node);
//     }
// }

// void
// do_spacer(
//     layout_traversal& traversal,
//     data_traversal& data,
//     layout_box* region,
//     layout const& layout_spec)
// {
//     layout_leaf* node;
//     get_cached_data(data, &node);

//     if (traversal.is_refresh_pass)
//     {
//         node->refresh_layout(
//             traversal,
//             layout_spec,
//             leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
//         add_layout_node(traversal, node);
//     }
//     else
//         *region = node->assignment().region;
// }

// ROW LAYOUT

// void
// row_layout::concrete_begin(
//     layout_traversal& traversal,
//     data_traversal& data,
//     layout const& layout_spec){
//     ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(row_layout_logic)}

// COLUMN LAYOUT

// void
// column_layout::concrete_begin(
//     layout_traversal& traversal,
//     data_traversal& data,
//     layout const& layout_spec)
// {
//     ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(old_column_layout_logic)
// }

// LINEAR LAYOUT - This just chooses between row and column logic.

// void
// linear_layout::concrete_begin(
//     layout_traversal& traversal,
//     data_traversal& data,
//     linear_layout_flag_set flags,
//     layout const& layout_spec)
// {
//     ALIA_IF_(data, flags & VERTICAL_LAYOUT)
//     {
//         old_column_layout_logic* logic;
//         get_simple_layout_container(
//             traversal, data, &container_, &logic, layout_spec);
//     }
//     ALIA_ELSE_(data)
//     {
//         row_layout_logic* logic;
//         get_simple_layout_container(
//             traversal, data, &container_, &logic, layout_spec);
//     }
//     ALIA_END
//     slc_.begin(traversal, container_);
// }

// LAYERED LAYOUT

// void
// layered_layout::concrete_begin(
//     layout_traversal& traversal,
//     data_traversal& data,
//     layout const& layout_spec){
//     ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(layered_layout_logic)}

// FLOW LAYOUT

// void
// flow_layout::concrete_begin(
//     layout_traversal& traversal,
//     data_traversal& data,
//     layout const& requested_layout_spec)
// {
//     // With a flow layout, we want to have the layout itself always fill
//     // the horizontal space and use the requested X alignment to position
//     // the individual rows in the flow.
//     auto layout_spec = add_default_padding(requested_layout_spec, PADDED);
//     layout_flag_set x_alignment = FILL_X;
//     if ((layout_spec.flags.code & 0x3) != 0)
//     {
//         x_alignment.code = layout_spec.flags.code & X_ALIGNMENT_MASK_CODE;
//         layout_spec.flags.code &= ~X_ALIGNMENT_MASK_CODE;
//         layout_spec.flags.code |= FILL_X_CODE;
//     }
//     ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(flow_layout_logic)
//     logic->x_alignment_ = x_alignment;
// }

// VERTICAL FLOW LAYOUT

// void
// vertical_flow_layout::concrete_begin(
//     layout_traversal& traversal,
//     data_traversal& data,
//     layout const& layout_spec)
// {
//     ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(vertical_flow_layout_logic)
// }

// CLAMPED LAYOUT

// void
// clamped_layout::concrete_begin(
//     layout_traversal& traversal,
//     data_traversal& data,
//     absolute_size max_size,
//     layout const& layout_spec)
// {
//     clamped_layout_logic* logic;
//     get_simple_layout_container(
//         traversal, data, &container_, &logic, layout_spec);
//     slc_.begin(traversal, container_);
//     if (traversal.is_refresh_pass)
//     {
//         detect_layout_change(
//             traversal,
//             &logic->max_size,
//             as_layout_size(resolve_absolute_size(traversal, max_size)));
//     }
// }

// BORDERED LAYOUT

struct bordered_layout_logic
{
    calculated_layout_requirements
    get_horizontal_requirements(layout_node* children);
    calculated_layout_requirements
    get_vertical_requirements(
        layout_node* children, layout_scalar assigned_width);
    void
    set_relative_assignment(
        layout_node* children,
        layout_vector const& assigned_size,
        layout_scalar assigned_baseline_y);

    box_border_width<layout_scalar> border;
};

calculated_layout_requirements
bordered_layout_logic::get_horizontal_requirements(layout_node* children)
{
    return calculated_layout_requirements{
        get_max_child_width(children) + (border.left + border.right), 0, 0};
}

calculated_layout_requirements
bordered_layout_logic::get_vertical_requirements(
    layout_node* children, layout_scalar assigned_width)
{
    calculated_layout_requirements requirements
        = fold_vertical_child_requirements(
            children, assigned_width - (border.left + border.right));
    return calculated_layout_requirements{
        requirements.size + border.top + border.bottom,
        requirements.ascent + border.top,
        requirements.descent + border.bottom};
}

void
bordered_layout_logic::set_relative_assignment(
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    walk_layout_nodes(children, [&](layout_node& node) {
        node.set_relative_assignment(relative_layout_assignment{
            layout_box(
                make_layout_vector(border.left, border.top),
                assigned_size
                    - make_layout_vector(
                        border.left + border.right,
                        border.top + border.bottom)),
            assigned_baseline_y - border.top});
    });
}

// void
// bordered_layout::concrete_begin(
//     layout_traversal& traversal,
//     data_traversal& data,
//     box_border_width<absolute_length> border,
//     layout const& layout_spec)
// {
//     bordered_layout_logic* logic;
//     get_simple_layout_container(
//         traversal, data, &container_, &logic, layout_spec);
//     slc_.begin(traversal, container_);
//     if (traversal.is_refresh_pass)
//     {
//         detect_layout_change(
//             traversal,
//             &logic->border,
//             as_layout_size(resolve_box_border_width(traversal, border)));
//     }
// }

#if 0

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
struct nonuniform_grid_tag
{
};
struct uniform_grid_tag
{
};

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
    x.requirements = layout_requirements{0, 0, 0, 1};
}

// Add the requirements for a column.
static void
add_requirements(
    grid_column_requirements<nonuniform_grid_tag>& x,
    layout_requirements const& addition)
{
    x.columns.push_back(addition);
}
static void
add_requirements(
    grid_column_requirements<uniform_grid_tag>& x,
    layout_requirements const& addition)
{
    ++x.n_columns;
    fold_in_requirements(x.requirements, addition);
}

// Fold the second set of requirements into the first.
static void
fold_in_requirements(
    grid_column_requirements<nonuniform_grid_tag>& x,
    grid_column_requirements<nonuniform_grid_tag> const& y)
{
    size_t n_columns = get_column_count(y);
    if (get_column_count(x) < n_columns)
        x.columns.resize(n_columns, layout_requirements{0, 0, 0, 0});
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
fold_in_requirements(
    grid_column_requirements<uniform_grid_tag>& x,
    grid_column_requirements<uniform_grid_tag> const& y)
{
    if (x.n_columns < y.n_columns)
        x.n_columns = y.n_columns;
    fold_in_requirements(x.requirements, y.requirements);
}

// Get the requirements for the nth column.
inline layout_requirements const&
get_column_requirements(
    grid_column_requirements<nonuniform_grid_tag> const& x, size_t n)
{
    return x.columns[n];
}
inline layout_requirements const&
get_column_requirements(
    grid_column_requirements<uniform_grid_tag> const& x, size_t /*n*/)
{
    return x.requirements;
}

template<class Uniformity>
struct grid_row_container;

template<class Uniformity>
struct cached_grid_vertical_requirements
{
};

template<>
struct cached_grid_vertical_requirements<uniform_grid_tag>
{
    calculated_layout_requirements requirements;
    counter_type last_update = 1;
};

template<class Uniformity>
struct grid_data
{
    // the container that contains the whole grid
    layout_container* container;

    // list of rows in the grid
    grid_row_container<Uniformity>* rows;

    // spacing between columns
    layout_scalar column_spacing;

    // requirements for the columns
    grid_column_requirements<Uniformity> requirements;
    counter_type last_content_query = 0;

    // cached vertical requirements
    cached_grid_vertical_requirements<Uniformity> vertical_requirements_cache;

    // cached assignments
    std::vector<layout_scalar> assignments;
    counter_type last_assignments_update;
};

template<class Uniformity>
struct grid_row_container : layout_container
{
    // implementation of layout interface
    layout_requirements
    get_horizontal_requirements();
    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width);
    void
    set_relative_assignment(relative_layout_assignment const& assignment);

    void
    record_content_change(layout_traversal& traversal);
    void
    record_self_change(layout_traversal& traversal);

    layout_cacher cacher;

    // cached requirements for cells within this row
    grid_column_requirements<Uniformity> requirements;
    counter_type last_content_query;

    // reference to the data for the grid that this row belongs to
    grid_data<Uniformity>* grid;

    // next row in this grid
    grid_row_container* next;

    grid_row_container() : last_content_query(0)
    {
    }
};

// Update the requirements for a grid's columns by querying its contents.
template<class Uniformity>
void
update_grid_column_requirements(grid_data<Uniformity>& grid)
{
    // Only update if something in the grid has changed since the last
    // update.
    if (grid.last_content_query != grid.container->last_content_change)
    {
        // Clear the requirements for the grid and recompute them by
        // iterating through the rows and folding each row's requirements
        // into the main grid requirements.
        clear_requirements(grid.requirements);
        for (grid_row_container<Uniformity>* row = grid.rows; row;
             row = row->next)
        {
            // Again, only update if something in the row has changed.
            if (row->last_content_query != row->last_content_change)
            {
                clear_requirements(row->requirements);
                walk_layout_nodes(row->children, [&](layout_node& node) {
                    layout_requirements x = node.get_horizontal_requirements();
                    add_requirements(row->requirements, x);
                });
                row->last_content_query = row->last_content_change;
            }
            fold_in_requirements(grid.requirements, row->requirements);
        }
        grid.last_content_query = grid.container->last_content_change;
    }
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
grid_row_container<Uniformity>::get_horizontal_requirements()
{
    horizontal_layout_query query(
        cacher, grid->container->last_content_change);
    if (query.update_required())
    {
        update_grid_column_requirements(*grid);
        query.update(
            calculated_layout_requirements(get_required_width(*grid), 0, 0));
    }
    return query.result();
}

template<class Uniformity>
std::vector<layout_scalar> const&
calculate_column_assignments(
    grid_data<Uniformity>& grid, layout_scalar assigned_width)
{
    if (grid.last_assignments_update != grid.container->last_content_change)
    {
        update_grid_column_requirements(grid);
        size_t n_columns = get_column_count(grid.requirements);
        grid.assignments.resize(n_columns);
        layout_scalar required_width = get_required_width(grid);
        float total_growth = get_total_growth(grid);
        layout_scalar extra_width = assigned_width - required_width;
        for (size_t i = 0; i != n_columns; ++i)
        {
            layout_scalar width
                = get_column_requirements(grid.requirements, i).size;
            if (total_growth != 0)
            {
                float growth_factor
                    = get_column_requirements(grid.requirements, i)
                          .growth_factor;
                layout_scalar extra = round_to_layout_scalar(
                    (growth_factor / total_growth) * extra_width);
                extra_width -= extra;
                total_growth -= growth_factor;
                width += extra;
            }
            grid.assignments[i] = width;
        }
        grid.last_assignments_update = grid.container->last_content_change;
    }
    return grid.assignments;
}

calculated_layout_requirements
calculate_grid_row_vertical_requirements(
    grid_data<nonuniform_grid_tag>& grid,
    grid_row_container<nonuniform_grid_tag>& row,
    layout_scalar assigned_width)
{
    std::vector<layout_scalar> const& column_widths
        = calculate_column_assignments(grid, assigned_width);
    calculated_layout_requirements requirements(0, 0, 0);
    size_t column_index = 0;
    walk_layout_nodes(row.children, [&](layout_node& node) {
        fold_in_requirements(
            requirements,
            node.get_vertical_requirements(column_widths[column_index]));
        ++column_index;
    });
    return requirements;
}

calculated_layout_requirements
calculate_grid_row_vertical_requirements(
    grid_data<uniform_grid_tag>& grid,
    grid_row_container<uniform_grid_tag>& /*row*/,
    layout_scalar assigned_width)
{
    named_block nb;
    auto& cache = grid.vertical_requirements_cache;
    if (cache.last_update != grid.container->last_content_change)
    {
        update_grid_column_requirements(grid);

        std::vector<layout_scalar> const& widths
            = calculate_column_assignments(grid, assigned_width);

        calculated_layout_requirements& grid_requirements = cache.requirements;
        grid_requirements = calculated_layout_requirements(0, 0, 0);
        for (grid_row_container<uniform_grid_tag>* row = grid.rows; row;
             row = row->next)
        {
            size_t column_index = 0;
            walk_layout_nodes(row->children, [&](layout_node& node) {
                fold_in_requirements(
                    grid_requirements,
                    node.get_vertical_requirements(widths[column_index]));
                ++column_index;
            });
        }

        cache.last_update = grid.container->last_content_change;
    }
    return cache.requirements;
}

template<class Uniformity>
layout_requirements
grid_row_container<Uniformity>::get_vertical_requirements(
    layout_scalar assigned_width)
{
    vertical_layout_query query(
        cacher, grid->container->last_content_change, assigned_width);
    if (query.update_required())
    {
        query.update(calculate_grid_row_vertical_requirements(
            *grid, *this, assigned_width));
    }
    return query.result();
}

template<class Uniformity>
void
set_grid_row_relative_assignment(
    grid_data<Uniformity>& grid,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    std::vector<layout_scalar> const& column_widths
        = calculate_column_assignments(grid, assigned_size[0]);
    size_t n = 0;
    layout_vector p = make_layout_vector(0, 0);
    walk_layout_nodes(children, [&](layout_node& node) {
        layout_scalar this_width = column_widths[n];
        node.set_relative_assignment(relative_layout_assignment{
            layout_box(p, make_layout_vector(this_width, assigned_size[1])),
            assigned_baseline_y});
        p[0] += this_width + grid.column_spacing;
    });
}

template<class Uniformity>
void
grid_row_container<Uniformity>::set_relative_assignment(
    relative_layout_assignment const& assignment)
{
    relative_region_assignment rra(
        *this, cacher, grid->container->last_content_change, assignment);
    if (rra.update_required())
    {
        set_grid_row_relative_assignment(
            *grid,
            children,
            rra.resolved_assignment().region.size,
            rra.resolved_assignment().baseline_y);
        rra.update();
    }
}

template<class Uniformity>
void
grid_row_container<Uniformity>::record_content_change(
    layout_traversal& traversal)
{
    if (this->last_content_change != traversal.refresh_counter)
    {
        this->last_content_change = traversal.refresh_counter;
        if (this->parent)
            this->parent->record_content_change(traversal);
        for (grid_row_container<Uniformity>* row = this->grid->rows; row;
             row = row->next)
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
            this->parent->record_content_change(traversal);
    }
}

template<class Uniformity>
void
refresh_grid(layout_traversal& traversal, grid_data<Uniformity>& data)
{
    if (traversal.is_refresh_pass)
    {
        // Reset the row list.
        data.rows = 0;
    }
}

template<class Uniformity>
void
refresh_grid_row(
    layout_traversal& traversal,
    grid_data<Uniformity>& grid,
    grid_row_container<Uniformity>& row,
    layout const& layout_spec)
{
    // Add this row to the grid's list of rows.
    // It doesn't matter what order the list is in, and adding the row to
    // the front of the list is easier.
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
{
};

void
grid_layout::concrete_begin(
    layout_traversal& traversal,
    data_traversal& data,
    layout const& layout_spec,
    absolute_length const& column_spacing)
{
    traversal_ = &traversal;
    data_traversal_ = &data;

    get_cached_data(data, &data_);
    refresh_grid(traversal, *data_);

    simple_layout_container<column_layout_logic>* container;
    column_layout_logic* logic;
    get_simple_layout_container(
        traversal, data, &container, &logic, layout_spec);
    container_.begin(traversal, container);

    data_->container = container;

    layout_scalar resolved_spacing = as_layout_size(
        resolve_absolute_length(traversal, 0, column_spacing));
    detect_layout_change(traversal, &data_->column_spacing, resolved_spacing);
}

void
grid_row::begin(grid_layout const& grid, layout const& layout_spec)
{
    layout_traversal& traversal = *grid.traversal_;

    grid_row_container<nonuniform_grid_tag>* row;
    if (get_cached_data(*grid.data_traversal_, &row))
        initialize(traversal, *row);

    refresh_grid_row(traversal, *grid.data_, *row, layout_spec);

    container_.begin(traversal, row);
}

void
grid_row::end()
{
    container_.end();
}

// UNIFORM GRID

struct uniform_grid_layout_data : grid_data<uniform_grid_tag>
{
};

void
uniform_grid_layout::concrete_begin(
    layout_traversal& traversal,
    data_traversal& data,
    layout const& layout_spec,
    absolute_length const& column_spacing)
{
    traversal_ = &traversal;
    data_traversal_ = &data;

    get_cached_data(*data_traversal_, &data_);
    refresh_grid(traversal, *data_);

    simple_layout_container<column_layout_logic>* container;
    column_layout_logic* logic;
    get_simple_layout_container(
        traversal, data, &container, &logic, layout_spec);
    container_.begin(traversal, container);

    data_->container = container;

    layout_scalar resolved_spacing = as_layout_size(
        resolve_absolute_length(traversal, 0, column_spacing));
    detect_layout_change(traversal, &data_->column_spacing, resolved_spacing);
}

void
uniform_grid_row::begin(
    uniform_grid_layout const& grid, layout const& /*layout_spec*/)
{
    layout_traversal& traversal = *grid.traversal_;

    grid_row_container<uniform_grid_tag>* row;
    if (get_cached_data(*grid.data_traversal_, &row))
        initialize(traversal, *row);

    // All rows must be set to GROW to ensure they receive equal space from
    // the column that contains them.
    refresh_grid_row(traversal, *grid.data_, *row, GROW | UNPADDED);

    container_.begin(traversal, row);
}

void
uniform_grid_row::end()
{
    container_.end();
}

// FLOATING LAYOUT

struct floating_layout_data
{
    data_graph measurement_cache, placement_cache;
    layout_vector size;
    floating_layout_data()
    {
    }
};

void
floating_layout::concrete_begin(
    layout_traversal& traversal,
    data_traversal& data,
    layout_vector const& min_size,
    layout_vector const& max_size)
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

    // if (traversal.geometry)
    //     clipping_reset_.begin(*traversal.geometry);
}

void
floating_layout::end()
{
    if (traversal_)
    {
        layout_traversal& traversal = *traversal_;

        if (traversal.is_refresh_pass)
        {
            traversal_->active_container = old_container_;
            traversal_->next_ptr = old_next_ptr_;

            layout_vector measured_size = get_minimum_size(floating_root_);
            for (unsigned i = 0; i != 2; ++i)
            {
                data_->size[i] = measured_size[i];
                if (min_size_[i] >= 0 && data_->size[i] < min_size_[i])
                    data_->size[i] = min_size_[i];
                if (max_size_[i] >= 0 && data_->size[i] > max_size_[i])
                    data_->size[i] = max_size_[i];
            }
            resolve_layout(floating_root_, data_->size);
        }

        // clipping_reset_.end();

        traversal_ = 0;
    }
}

layout_vector
floating_layout::size() const
{
    return data_->size;
}

#endif

}} // namespace alia::indie
