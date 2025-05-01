#include <alia/ui/layout/grids.hpp>

#include <alia/ui/layout/simple.hpp>
#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

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
    // TODO: Store this in-object somehow. (Usually, the column will have a
    // fixed number of columns anyway.)
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

inline void
invalidate(cached_grid_vertical_requirements<nonuniform_grid_tag>&)
{
}

template<>
struct cached_grid_vertical_requirements<uniform_grid_tag>
{
    calculated_layout_requirements requirements;
    layout_scalar assigned_width = 0;
};

inline void
invalidate(cached_grid_vertical_requirements<uniform_grid_tag>& grid)
{
    grid.assigned_width = -1;
}

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

    // cached vertical requirements
    cached_grid_vertical_requirements<Uniformity> vertical_requirements_cache;

    // cached assignments
    // TODO: Give the option to avoid using a vector here.
    std::vector<layout_scalar> assignments;
    // TODO: Combine this with other flags.
    bool assignments_valid = false;
    // TODO: Combine this with other flags.
    bool requirements_valid = false;
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
    record_content_change();
    void
    record_self_change();

    // cached requirements for cells within this row
    grid_column_requirements<Uniformity> requirements;
    // TODO: Merge this in somewhere else.
    bool requirements_valid = false;

    // reference to the data for the grid that this row belongs to
    grid_data<Uniformity>* grid;

    // next row in this grid
    grid_row_container* next;
};

// Update the requirements for a grid's columns by querying its contents.
template<class Uniformity>
void
update_grid_column_requirements(grid_data<Uniformity>& grid)
{
    if (!grid.requirements_valid)
    {
        // Clear the requirements for the grid and recompute them by
        // iterating through the rows and folding each row's requirements
        // into the main grid requirements.
        clear_requirements(grid.requirements);
        for (grid_row_container<Uniformity>* row = grid.rows; row;
             row = row->next)
        {
            if (!row->requirements_valid)
            {
                clear_requirements(row->requirements);
                walk_layout_children(row->children, [&](layout_node& node) {
                    layout_requirements x = node.get_horizontal_requirements();
                    add_requirements(row->requirements, x);
                });
                row->requirements_valid = true;
            }
            fold_in_requirements(grid.requirements, row->requirements);
        }
        grid.requirements_valid = true;
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
    return cache_horizontal_layout_requirements(cacher, [&] {
        update_grid_column_requirements(*grid);
        return calculated_layout_requirements(get_required_width(*grid), 0, 0);
    });
}

template<class Uniformity>
std::vector<layout_scalar> const&
calculate_column_assignments(
    grid_data<Uniformity>& grid, layout_scalar assigned_width)
{
    if (!grid.assignments_valid)
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
        grid.assignments_valid = true;
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
    walk_layout_children(row.children, [&](layout_node& node) {
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
    auto& cache = grid.vertical_requirements_cache;
    if (cache.assigned_width != assigned_width)
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
            walk_layout_children(row->children, [&](layout_node& node) {
                fold_in_requirements(
                    grid_requirements,
                    node.get_vertical_requirements(widths[column_index]));
                ++column_index;
            });
        }

        cache.assigned_width = assigned_width;
    }
    return cache.requirements;
}

template<class Uniformity>
layout_requirements
grid_row_container<Uniformity>::get_vertical_requirements(
    layout_scalar assigned_width)
{
    return cache_vertical_layout_requirements(
        this->cacher, assigned_width, [&] {
            return calculate_grid_row_vertical_requirements(
                *grid, *this, assigned_width);
        });
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
    walk_layout_children(children, [&](layout_node& node) {
        layout_scalar this_width = column_widths[n];
        node.set_relative_assignment(relative_layout_assignment{
            layout_box(p, make_layout_vector(this_width, assigned_size[1])),
            assigned_baseline_y});
        p[0] += this_width + grid.column_spacing;
        ++n;
    });
}

template<class Uniformity>
void
grid_row_container<Uniformity>::set_relative_assignment(
    relative_layout_assignment const& assignment)
{
    update_relative_assignment(
        *this, cacher, assignment, [&](auto const& resolved_assignment) {
            set_grid_row_relative_assignment(
                *grid,
                children,
                resolved_assignment.region.size,
                resolved_assignment.baseline_y);
        });
}

template<class Uniformity>
void
grid_row_container<Uniformity>::record_content_change()
{
    if (!cache_is_fully_invalid(this->cacher))
    {
        invalidate(this->grid->vertical_requirements_cache);
        this->grid->assignments_valid = false;
        this->grid->requirements_valid = false;
        invalidate_cached_layout(this->cacher);
        // TODO: Is it true that this flag is never valid when the cacher is
        // fully invalid?
        this->requirements_valid = false;
        if (this->parent)
            this->parent->record_content_change();
        for (grid_row_container<Uniformity>* row = this->grid->rows; row;
             row = row->next)
        {
            row->record_self_change();
        }
    }
}

template<class Uniformity>
void
grid_row_container<Uniformity>::record_self_change()
{
    if (!cache_is_fully_invalid(this->cacher))
    {
        invalidate_cached_layout(this->cacher);
        this->requirements_valid = false;
        if (this->parent)
            this->parent->record_content_change();
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

    simple_layout_container<column_layout_logic>* container = nullptr;
    column_layout_logic* logic;
    get_simple_layout_container(
        traversal, data, &container, &logic, layout_spec);
    container_.begin(traversal, container);

    begin_layout_transform(transform_, traversal, container->cacher);

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
    get_cached_data(*grid.data_traversal_, &row);

    refresh_grid_row(traversal, *grid.data_, *row, layout_spec);

    container_.begin(traversal, row);

    begin_layout_transform(transform_, traversal, row->cacher);
}

void
grid_row::end()
{
    transform_.end();
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

    simple_layout_container<column_layout_logic>* container = nullptr;
    column_layout_logic* logic;
    get_simple_layout_container(
        traversal, data, &container, &logic, layout_spec);
    container_.begin(traversal, container);

    begin_layout_transform(transform_, traversal, container->cacher);

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
    get_cached_data(*grid.data_traversal_, &row);

    // All rows must be set to GROW to ensure they receive equal space from
    // the column that contains them.
    refresh_grid_row(traversal, *grid.data_, *row, GROW | UNPADDED);

    container_.begin(traversal, row);

    begin_layout_transform(transform_, traversal, row->cacher);
}

void
uniform_grid_row::end()
{
    transform_.end();
    container_.end();
}

} // namespace alia
