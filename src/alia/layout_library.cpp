#include <alia/layout_library.hpp>
#include <alia/layout_system.hpp>
#include <vector>

namespace alia {

void do_spacer(layout_traversal& traversal, layout const& layout_spec)
{
    layout_leaf* node;
    get_data(*traversal.data, &node);

    if (traversal.is_refresh_pass)
    {
        node->refresh_layout(traversal, layout_spec,
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
        add_layout_node(traversal, node);
    }
}

void do_spacer(layout_traversal& traversal, layout_box* region,
    layout const& layout_spec)
{
    layout_leaf* node;
    get_data(*traversal.data, &node);

    if (traversal.is_refresh_pass)
    {
        node->refresh_layout(traversal, layout_spec,
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
        add_layout_node(traversal, node);
    }
    else
        *region = node->assignment().region;
}

layout_box get_container_region(simple_layout_container const& container)
{
    return layout_box(make_layout_vector(0, 0), container.assigned_size);
}

#define DECLARE_LAYOUT_LOGIC(logic_name) \
    struct logic_name : layout_logic \
    { \
        calculated_layout_requirements get_horizontal_requirements( \
            layout_calculation_context& ctx, \
            layout_node* children); \
        calculated_layout_requirements get_vertical_requirements( \
            layout_calculation_context& ctx, \
            layout_node* children, \
            layout_scalar assigned_width); \
        void set_relative_assignment( \
            layout_calculation_context& ctx, \
            layout_node* children, \
            layout_vector const& assigned_size, \
            layout_scalar assigned_baseline_y); \
    };

#define BEGIN_SIMPLE_LAYOUT_CONTAINER(logic_name) \
    logic_name* logic; \
    get_simple_layout_container(traversal, &container_, &logic, layout_spec); \
    slc_.begin(traversal, container_); \
    begin_transform(transform_, traversal, container_->cacher);

// ROW LAYOUT

DECLARE_LAYOUT_LOGIC(row_layout_logic)

static void compute_total_width(
    layout_calculation_context& ctx,
    layout_scalar& total_width,
    float& total_proportion,
    layout_node* children)
{
    total_width = 0;
    total_proportion = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements r = get_horizontal_requirements(ctx, *i);
        total_width += r.minimum_size;
        total_proportion += r.proportion;
    }
}

calculated_layout_requirements
row_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx,
    layout_node* children)
{
    layout_scalar total_size;
    float total_proportion;
    compute_total_width(ctx, total_size, total_proportion, children);
    return calculated_layout_requirements(total_size, 0, 0);
}

static layout_scalar calculate_child_size(
    layout_scalar& remaining_extra_size,
    float& remaining_proportion,
    layout_scalar this_minimum_size,
    float this_proportion)
{
    if (remaining_proportion != 0)
    {
        layout_scalar extra_size = round_to_layout_scalar(
            float(remaining_extra_size) *
            (this_proportion / remaining_proportion));
        remaining_extra_size -= extra_size;
        remaining_proportion -= this_proportion;
        return this_minimum_size + extra_size;
    }
    else
        return this_minimum_size;
}

calculated_layout_requirements
row_layout_logic::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    layout_scalar total_size;
    float total_proportion;
    compute_total_width(ctx, total_size, total_proportion, children);
    layout_scalar remaining_extra_size = assigned_width - total_size;
    float remaining_proportion = total_proportion;
    layout_scalar height = 0, ascent = 0, descent = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements x =
            alia::get_horizontal_requirements(ctx, *i);
        layout_scalar this_width = calculate_child_size(
            remaining_extra_size, remaining_proportion,
            x.minimum_size, x.proportion);
        layout_requirements y = alia::get_vertical_requirements(
            ctx, *i, this_width);
        height = (std::max)(y.minimum_size, height);
        ascent = (std::max)(y.minimum_ascent, ascent);
        descent = (std::max)(y.minimum_descent, descent);
    }
    return calculated_layout_requirements(height, ascent, descent);
}

void row_layout_logic::set_relative_assignment(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    layout_scalar total_size;
    float total_proportion;
    compute_total_width(ctx, total_size, total_proportion, children);
    layout_vector p = make_layout_vector(0, 0);
    layout_scalar remaining_extra_size = assigned_size[0] - total_size;
    float remaining_proportion = total_proportion;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements x =
            alia::get_horizontal_requirements(ctx, *i);
        layout_scalar this_width = calculate_child_size(
            remaining_extra_size, remaining_proportion,
            x.minimum_size, x.proportion);
        alia::set_relative_assignment(
            ctx, *i,
            relative_layout_assignment(
                layout_box(p,
                    make_layout_vector(this_width, assigned_size[1])),
                assigned_baseline_y));
        p[0] += this_width;
    }
}

static void begin_transform(
    scoped_transformation& transform,
    layout_traversal const& traversal,
    layout_cacher const& cacher)
{
    if (!traversal.is_refresh_pass)
    {
        transform.begin(*traversal.geometry);
        transform.set(translation_matrix(vector<2,double>(
            cacher.resolved_relative_assignment.region.corner)));
    }
}

void row_layout::concrete_begin(
    layout_traversal& traversal, layout const& layout_spec)
{
    BEGIN_SIMPLE_LAYOUT_CONTAINER(row_layout_logic)
}

// COLUMN LAYOUT

DECLARE_LAYOUT_LOGIC(column_layout_logic)

static void compute_total_height(
    layout_scalar& total_height,
    float& total_proportion,
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar width)
{
    total_height = 0;
    total_proportion = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements r = alia::get_vertical_requirements(
            ctx, *i, width);
        total_height += r.minimum_size;
        total_proportion += r.proportion;
    }
}

calculated_layout_requirements
column_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx,
    layout_node* children)
{
    layout_scalar width = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements r = alia::get_horizontal_requirements(ctx, *i);
        width = (std::max)(r.minimum_size, width);
    }
    return calculated_layout_requirements(width, 0, 0);
}

calculated_layout_requirements
column_layout_logic::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    layout_scalar total_size;
    float total_proportion;
    compute_total_height(total_size, total_proportion, ctx, children,
        assigned_width);
    return calculated_layout_requirements(total_size, 0, 0);
}

void column_layout_logic::set_relative_assignment(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    layout_scalar total_size;
    float total_proportion;
    compute_total_height(total_size, total_proportion, ctx, children,
        assigned_size[0]);
    layout_vector p = make_layout_vector(0, 0);
    layout_scalar remaining_extra_size = assigned_size[1] - total_size;
    float remaining_proportion = total_proportion;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements y = alia::get_vertical_requirements(
            ctx, *i, assigned_size[0]);
        layout_scalar this_height = calculate_child_size(
            remaining_extra_size, remaining_proportion,
            y.minimum_size, y.proportion);
        alia::set_relative_assignment(
            ctx, *i,
            relative_layout_assignment(
                layout_box(p,
                    make_layout_vector(assigned_size[0], this_height)),
                this_height - y.minimum_descent));
        p[1] += this_height;
    }
}

void column_layout::concrete_begin(
    layout_traversal& traversal, layout const& layout_spec)
{
    BEGIN_SIMPLE_LAYOUT_CONTAINER(column_layout_logic)
}

// LINEAR LAYOUT - This just chooses between row and column logic.

void linear_layout::concrete_begin(
    layout_traversal& traversal, unsigned axis, layout const& layout_spec)
{
    alia_if_(*traversal.data, axis != 0)
    {
        column_layout_logic* logic;
        get_simple_layout_container(
            traversal, &container_, &logic, layout_spec);
    }
    alia_else_(*traversal.data)
    {
        row_layout_logic* logic;
        get_simple_layout_container(
            traversal, &container_, &logic, layout_spec);
    }
    alia_end
    slc_.begin(traversal, container_);
    begin_transform(transform_, traversal, container_->cacher);
}

// LAYERED LAYOUT

DECLARE_LAYOUT_LOGIC(layered_layout_logic)

calculated_layout_requirements
layered_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx,
    layout_node* children)
{
    layout_scalar width = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements r = alia::get_horizontal_requirements(ctx, *i);
        width = (std::max)(r.minimum_size, width);
    }
    return calculated_layout_requirements(width, 0, 0);
}

calculated_layout_requirements
layered_layout_logic::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    layout_scalar height = 0, ascent = 0, descent = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements y = alia::get_vertical_requirements(
            ctx, *i, assigned_width);
        height = (std::max)(y.minimum_size, height);
        ascent = (std::max)(y.minimum_ascent, ascent);
        descent = (std::max)(y.minimum_descent, descent);
    }
    return calculated_layout_requirements(height, ascent, descent);
}

void layered_layout_logic::set_relative_assignment(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    for (layout_node* i = children; i; i = i->next)
    {
        alia::set_relative_assignment(ctx, *i,
            relative_layout_assignment(
                layout_box(make_layout_vector(0, 0), assigned_size),
                assigned_baseline_y));
    }
}

void layered_layout::concrete_begin(
    layout_traversal& traversal, layout const& layout_spec)
{
    BEGIN_SIMPLE_LAYOUT_CONTAINER(layered_layout_logic)
}

// ROTATED LAYOUT

DECLARE_LAYOUT_LOGIC(rotated_layout_logic)

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
        layout_requirements x = alia::get_horizontal_requirements(
            ctx, *i);
        layout_requirements y = alia::get_vertical_requirements(
            ctx, *i, 100000);
        width = (std::max)(y.minimum_size, width);
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
        layout_requirements x = alia::get_horizontal_requirements(
            ctx, *i);
        layout_scalar lower_bound = x.minimum_size, upper_bound = 100000;
        while (!layout_scalars_almost_equal(upper_bound, lower_bound))
        {
            layout_scalar test_value = (upper_bound + lower_bound) / 2;
            layout_requirements y = alia::get_vertical_requirements(
                ctx, *i, test_value);
            if (y.minimum_size <= assigned_width)
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
        layout_requirements y = alia::get_vertical_requirements(
            ctx, *i, assigned_size[1]);
        alia::set_relative_assignment(
            ctx, *i,
            relative_layout_assignment(
                layout_box(make_layout_vector(0, 0),
                    make_layout_vector(assigned_size[1], assigned_size[0])),
                assigned_size[0] - y.minimum_descent));
    }
}

void rotated_layout::concrete_begin(
    layout_traversal& traversal, layout const& layout_spec)
{
    rotated_layout_logic* logic;
    get_simple_layout_container(traversal, &container_, &logic, layout_spec);
    slc_.begin(traversal, container_);

    if (!traversal.is_refresh_pass)
    {
        transform_.begin(*traversal.geometry);
        transform_.set(
            translation_matrix(vector<2,double>(
                container_->cacher.resolved_relative_assignment.region.corner +
                    make_layout_vector(0, container_->cacher.
                        resolved_relative_assignment.region.size[1]))) *
            make_matrix<double>(
                0,  1, 0,
               -1,  0, 0,
                0,  0, 1));
    }
}

// FLOW LAYOUT

DECLARE_LAYOUT_LOGIC(flow_layout_logic)

calculated_layout_requirements
flow_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx, layout_node* children)
{
    // In the worst case scenario, we can put one child on each row, so the
    // minimum required width is simply the minimum required width of the
    // widest child.
    layout_scalar width = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements x = i->get_minimal_horizontal_requirements(ctx);
        width = (std::max)(x.minimum_size, width);
    }
    return calculated_layout_requirements(width, 0, 0);
}

static layout_scalar calculate_wrapping(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width,
    std::vector<wrapped_row>* rows)
{
    wrapping_state state;
    state.accumulated_width = 0;
    state.active_row.requirements = layout_requirements(0, 0, 0, 0);
    state.active_row.y = 0;
    state.rows = rows;

    for (layout_node* i = children; i; i = i->next)
        i->calculate_wrapping(ctx, assigned_width, state);
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
    // Use the baseline of the first row as the baseline of the whole flow.
    return calculated_layout_requirements(
        total_height, rows[0].requirements.minimum_ascent,
        total_height - rows[0].requirements.minimum_ascent);
}

void assign_flow_regions(
    layout_calculation_context& ctx,
    layout_node*& i, layout_node* end, layout_vector p,
    layout_scalar row_height, layout_scalar row_descent)
{
    for (; i != end; i = i->next)
    {
        layout_requirements x = alia::get_horizontal_requirements(
            ctx, *i);
        alia::set_relative_assignment(
            ctx, *i,
            relative_layout_assignment(
                layout_box(p, make_layout_vector(x.minimum_size, row_height)),
                row_height - row_descent));
        p[0] += x.minimum_size;
    }
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
    state.x = 0;
    state.active_row = rows.begin();
    for (layout_node* i = children; i; i = i->next)
        i->assign_wrapped_regions(ctx, assigned_size[0], state);
}

void flow_layout::concrete_begin(
    layout_traversal& traversal, layout const& layout_spec)
{
    BEGIN_SIMPLE_LAYOUT_CONTAINER(flow_layout_logic)
}

// VERTICAL FLOW LAYOUT

// The vertical flow algorithm is a bit overly simplistic. All columns are
// created with the same width.

DECLARE_LAYOUT_LOGIC(vertical_flow_layout_logic)

static layout_scalar get_max_child_width(
    layout_calculation_context& ctx,
    layout_node* children)
{
    layout_scalar width = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements x = alia::get_horizontal_requirements(
            ctx, *i);
        width = (std::max)(x.minimum_size, width);
    }
    return width;
}

calculated_layout_requirements
vertical_flow_layout_logic::get_horizontal_requirements(
    layout_calculation_context& ctx,
    layout_node* children)
{
    // In the worst case scenario, we can put all children in one column, so
    // the minimum required width is simply the maximum required width of any
    // child.
    return calculated_layout_requirements(
        get_max_child_width(ctx, children), 0, 0);
}

calculated_layout_requirements
vertical_flow_layout_logic::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    layout_scalar assigned_width)
{
    layout_scalar column_width = get_max_child_width(ctx, children);

    // Count the total vertical space required by all children.
    layout_scalar total_height = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements y = alia::get_vertical_requirements(
            ctx, *i, column_width);
        total_height += y.minimum_size;
    }

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
            layout_requirements y = alia::get_vertical_requirements(
                ctx, *child_i, column_width);
            column_height += y.minimum_size;
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

    // Count the total vertical space required by all children.
    layout_scalar total_height = 0;
    for (layout_node* i = children; i; i = i->next)
    {
        layout_requirements y = alia::get_vertical_requirements(
            ctx, *i, column_width);
        total_height += y.minimum_size;
    }

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
            layout_requirements y = alia::get_vertical_requirements(
                ctx, *child_i, column_width);
            alia::set_relative_assignment(
                ctx, *child_i,
                relative_layout_assignment(
                    layout_box(p,
                        make_layout_vector(column_width, y.minimum_size)),
                    y.minimum_size - y.minimum_descent));
            p[1] += y.minimum_size;
            child_i = child_i->next;
        }
    }
}

void vertical_flow_layout::concrete_begin(
    layout_traversal& traversal, layout const& layout_spec)
{
    BEGIN_SIMPLE_LAYOUT_CONTAINER(vertical_flow_layout_logic)
}

// GRID LAYOUT

// Grids are composed of multiple rows, each with their own children.
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

struct grid_column_requirements
{
    layout_scalar minimum_size;
    float proportion;

    grid_column_requirements() {}
    grid_column_requirements(layout_scalar minimum_size, float proportion)
      : minimum_size(minimum_size), proportion(proportion)
    {}
};

struct grid_row_container;

struct grid_data
{
    // the container that contains the whole grid
    alia::layout_container* container;

    // list of rows in the grid
    grid_row_container* rows;

    // spacing between columns
    layout_scalar column_spacing;

    // requirements for the columns
    std::vector<grid_column_requirements> columns;
    counter_type last_content_query;

    grid_data() : last_content_query(0) {}
};

struct grid_row_container : layout_container
{
    grid_row_container()
      : last_content_query(0)
    {}

    // implementation of layout interface
    layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx);
    layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_scalar assigned_width);
    void set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment);

    layout_cacher cacher;

    // cached requirements for cells within this row
    counter_type last_content_query;
    std::vector<grid_column_requirements> cells;

    // data for the grid that this row belongs to
    grid_data* grid;

    // next row in this grid
    grid_row_container* next;
};

struct scoped_recursion_detector : noncopyable
{
    scoped_recursion_detector(bool* inside)
    {
        already_inside_ = *inside;
        inside_ = inside;
        *inside = true;
    }
    ~scoped_recursion_detector()
    {
        *inside_ = already_inside_;
    }
    bool already_inside() const { return already_inside_; }
 private:
    bool already_inside_;
    bool* inside_;
};

static void update_grid_column_requirements(
    layout_calculation_context& ctx, grid_data& grid)
{
    named_block nb(ctx, make_id(&grid.last_content_query));
    alia_if (grid.last_content_query != grid.container->last_content_change)
    {
        naming_context nc(ctx);
        grid.columns.clear();
        for (grid_row_container* row = grid.rows; row; row = row->next)
        {
            named_block nb(nc, make_id(row));
            alia_if (row->last_content_query != row->last_content_change)
            {
                row->cells.clear();
                for (layout_node* child = row->children; child;
                    child = child->next)
                {
                    layout_requirements x = alia::get_horizontal_requirements(
                        ctx, *child);
                    grid_column_requirements requirements;
                    requirements.minimum_size = x.minimum_size;
                    requirements.proportion = x.proportion;
                    row->cells.push_back(requirements);
                }
                row->last_content_query = row->last_content_change;
            }
            alia_end
            size_t n_columns = row->cells.size();
            if (grid.columns.size() < n_columns)
                grid.columns.resize(n_columns, grid_column_requirements(0, 0));
            for (size_t i = 0; i != n_columns; ++i)
            {
                if (row->cells[i].minimum_size > grid.columns[i].minimum_size)
                    grid.columns[i].minimum_size = row->cells[i].minimum_size;
                if (row->cells[i].proportion > grid.columns[i].proportion)
                    grid.columns[i].proportion = row->cells[i].proportion;
            }
        }
        grid.last_content_query = grid.container->last_content_change;
    }
    alia_end
}

static calculated_layout_requirements
calculate_grid_horizontal_requirements(grid_data const& grid)
{
    size_t n_columns = grid.columns.size();
    layout_scalar minimum_width = 0;
    for (size_t i = 0; i != n_columns; ++i)
        minimum_width += grid.columns[i].minimum_size;
    if (n_columns > 0)
        minimum_width += grid.column_spacing * layout_scalar(n_columns - 1);
    return calculated_layout_requirements(minimum_width, 0, 0);
}

layout_requirements
grid_row_container::get_horizontal_requirements(layout_calculation_context& ctx)
{
    horizontal_layout_query query(ctx, cacher,
        grid->container->last_content_change);
    alia_if (query.update_required())
    {
        update_grid_column_requirements(ctx, *grid);
        query.update(calculate_grid_horizontal_requirements(*grid));
    }
    alia_end
    return query.result();
}

struct cached_grid_column_assignments
{
    counter_type last_update;
    std::vector<layout_scalar> assignments;
};

static std::vector<layout_scalar> const&
calculate_column_assignments(
    layout_calculation_context& ctx,
    grid_data& grid,
    layout_scalar assigned_width)
{
    named_block nb(ctx, combine_ids(make_id(&grid), make_id(assigned_width)));
    cached_grid_column_assignments* cache;
    if (get_data(ctx, &cache) ||
        cache->last_update != grid.container->last_content_change)
    {
        update_grid_column_requirements(ctx, grid);

        size_t n_columns = grid.columns.size();
        cache->assignments.resize(n_columns);
        layout_scalar required_width = 0;
        float total_proportion = 0;
        for (size_t i = 0; i != n_columns; ++i)
        {
            required_width += grid.columns[i].minimum_size;
            total_proportion += grid.columns[i].proportion;
        }
        if (n_columns > 0)
            required_width += grid.column_spacing;
        layout_scalar extra_width = assigned_width - required_width;
        for (size_t i = 0; i != n_columns; ++i)
        {
            layout_scalar width = grid.columns[i].minimum_size;
            if (total_proportion != 0)
            {
                layout_scalar extra =
                    layout_scalar(
                        (grid.columns[i].proportion / total_proportion)
                        * extra_width);
                extra_width -= extra;
                total_proportion -= grid.columns[i].proportion;
                width += extra;
            }
            cache->assignments[i] = width;
        }
        cache->last_update = grid.container->last_content_change;
    }
    return cache->assignments;
}

static calculated_layout_requirements
calculate_grid_row_vertical_requirements(
    layout_calculation_context& ctx,
    layout_node* children,
    std::vector<layout_scalar> const& column_widths)
{
    layout_scalar height = 0, ascent = 0, descent = 0;
    size_t n = 0;
    for (layout_node* i = children; i; i = i->next, ++n)
    {
        layout_requirements y =
            get_vertical_requirements(ctx, *i, column_widths[n]);
        height = (std::max)(y.minimum_size, height);
        ascent = (std::max)(y.minimum_ascent, ascent);
        descent = (std::max)(y.minimum_descent, descent);
    }
    return calculated_layout_requirements(height, ascent, descent);
}

layout_requirements grid_row_container::get_vertical_requirements(
    layout_calculation_context& ctx, layout_scalar assigned_width)
{
    vertical_layout_query query(ctx, cacher,
        grid->container->last_content_change, assigned_width);
    alia_if (query.update_required())
    {
        std::vector<layout_scalar> const& column_widths =
            calculate_column_assignments(ctx, *grid, assigned_width);
        query.update(
            calculate_grid_row_vertical_requirements(
                ctx, children, column_widths));
    }
    alia_end
    return query.result();
}

static void set_grid_row_relative_assignment(
    layout_calculation_context& ctx,
    grid_data& grid,
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

void grid_row_container::set_relative_assignment(
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

void grid_layout::concrete_begin(
    layout_traversal& traversal, layout const& layout_spec,
    float column_spacing, layout_units spacing_units)
{
    traversal_ = &traversal;

    get_data(*traversal.data, &data_);
    if (traversal.is_refresh_pass)
        data_->rows = 0;

    simple_layout_container* container;
    column_layout_logic* logic;
    get_simple_layout_container(traversal, &container, &logic, layout_spec);
    container_.begin(traversal, container);

    begin_transform(transform_, traversal, container->cacher);

    data_->container = container;

    layout_scalar resolved_spacing =
        resolve_layout_width(traversal, column_spacing, spacing_units);
    detect_layout_change(traversal, &data_->column_spacing, resolved_spacing);
}

void grid_row::begin(grid_layout const& grid, layout const& layout_spec)
{
    layout_traversal& traversal = *grid.traversal_;

    grid_row_container* row;
    if (get_data(*traversal.data, &row))
        initialize(traversal, *row);

    // Add this row to the grid's list of children.
    // It doesn't matter what order the list is in, and adding the row to the
    // front of the list is easier.
    if (traversal.is_refresh_pass)
    {
        row->next = grid.data_->rows;
        grid.data_->rows = row;
        row->grid = grid.data_;
    }

    container_.begin(traversal, row);

    update(traversal, row->cacher, layout_spec, FILL | UNPADDED);

    begin_transform(transform_, traversal, row->cacher);
}

void grid_row::end()
{
    transform_.end();
    container_.end();
}

// UNIFORM GRID LAYOUT

struct uniform_grid_row_container;

struct uniform_grid_data
{
    // the container that contains the whole grid
    alia::layout_container* container;

    // list of rows in the grid
    uniform_grid_row_container* rows;

    // spacing between columns
    layout_scalar column_spacing;

    // # of columns
    size_t n_columns;

    // horizontal requirements of the cells
    calculated_layout_requirements horizontal_cell_requirements;
    counter_type last_horizontal_content_query;

    uniform_grid_data() : last_horizontal_content_query(0) {}
};

struct uniform_grid_row_container : layout_container
{
    uniform_grid_row_container()
      : last_horizontal_content_query(0)
    {}

    // implementation of layout interface
    layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx);
    layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_scalar assigned_width);
    void set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment);

    layout_cacher cacher;

    // cached requirements for cells within this row
    size_t n_cells;
    // horizontal
    calculated_layout_requirements horizontal_cell_requirements;
    counter_type last_horizontal_content_query;

    // data for the grid that this row belongs to
    uniform_grid_data* grid;

    // next row in this grid
    uniform_grid_row_container* next;
};

static void update_horizontal_grid_cell_requirements(
    layout_calculation_context& ctx, uniform_grid_data& grid)
{
    named_block nb(ctx, make_id(&grid.last_horizontal_content_query));
    alia_if (grid.last_horizontal_content_query !=
        grid.container->last_content_change)
    {
        naming_context nc(ctx);
        calculated_layout_requirements& grid_requirements =
            grid.horizontal_cell_requirements;
        grid_requirements = calculated_layout_requirements(0, 0, 0);
        grid.n_columns = 0;
        for (uniform_grid_row_container* row = grid.rows; row; row = row->next)
        {
            named_block nb(nc, make_id(row));
            calculated_layout_requirements& row_requirements =
                row->horizontal_cell_requirements;
            alia_if (row->last_horizontal_content_query !=
                row->last_content_change)
            {
                row->n_cells = 0;
                row_requirements = calculated_layout_requirements(0, 0, 0);
                for (layout_node* child = row->children; child;
                    child = child->next)
                {
                    layout_requirements x = alia::get_horizontal_requirements(
                        ctx, *child);
                    row_requirements.minimum_size = (std::max)(
                        row_requirements.minimum_size, x.minimum_size);
                    ++row->n_cells;
                }
                row->last_horizontal_content_query = row->last_content_change;
            }
            alia_end
            grid_requirements.minimum_size = (std::max)(
                grid_requirements.minimum_size, row_requirements.minimum_size);
            grid.n_columns = (std::max)(grid.n_columns, row->n_cells);
        }
        grid.last_horizontal_content_query =
            grid.container->last_content_change;
    }
    alia_end
}

static calculated_layout_requirements
calculate_uniform_grid_horizontal_requirements(uniform_grid_data const& grid)
{
    layout_scalar minimum_width = layout_scalar(grid.n_columns *
        grid.horizontal_cell_requirements.minimum_size);
    if (grid.n_columns > 0)
    {
        minimum_width += grid.column_spacing *
            layout_scalar(grid.n_columns - 1);
    }
    return calculated_layout_requirements(minimum_width, 0, 0);
}

layout_requirements
uniform_grid_row_container::get_horizontal_requirements(layout_calculation_context& ctx)
{
    horizontal_layout_query query(ctx, cacher,
        grid->container->last_content_change);
    alia_if (query.update_required())
    {
        update_horizontal_grid_cell_requirements(ctx, *grid);
        query.update(
            calculate_uniform_grid_horizontal_requirements(*grid));
    }
    alia_end
    return query.result();
}

std::vector<layout_scalar>
calculate_uniform_grid_column_widths(
    uniform_grid_data& grid, layout_scalar assigned_width)
{
    std::vector<layout_scalar> widths(grid.n_columns);
    layout_scalar remaining_width = assigned_width;
    if (grid.n_columns > 0)
    {
        remaining_width -= grid.column_spacing *
            layout_scalar(grid.n_columns - 1);
    }
    for (size_t i = 0; i != grid.n_columns; ++i)
    {
        layout_scalar this_width = remaining_width /
            layout_scalar(grid.n_columns - i);
        remaining_width -= this_width;
        widths[i] = this_width;
    }
    return widths;
}

struct cached_uniform_grid_vertical_requirements
{
    calculated_layout_requirements requirements;
    counter_type last_update;
};
calculated_layout_requirements
calculate_uniform_grid_vertical_requirements(
    layout_calculation_context& ctx,
    uniform_grid_data& grid,
    layout_scalar assigned_width)
{
    named_block nb(ctx, combine_ids(make_id(&grid), make_id(assigned_width)));
    cached_uniform_grid_vertical_requirements* cache;
    if (get_data(ctx, &cache) ||
        cache->last_update != grid.container->last_content_change)
    {
        update_horizontal_grid_cell_requirements(ctx, grid);

        std::vector<layout_scalar> widths =
            calculate_uniform_grid_column_widths(grid, assigned_width);

        calculated_layout_requirements& grid_requirements =
            cache->requirements;
        grid_requirements = calculated_layout_requirements(0, 0, 0);
        for (uniform_grid_row_container* row = grid.rows; row; row = row->next)
        {
            size_t column_index = 0;
            for (layout_node* child = row->children; child;
                child = child->next, ++column_index)
            {
                layout_requirements x = alia::get_vertical_requirements(
                    ctx, *child, widths[column_index]);
                grid_requirements.minimum_size = (std::max)(
                    grid_requirements.minimum_size, x.minimum_size);
                grid_requirements.minimum_ascent = (std::max)(
                    grid_requirements.minimum_ascent, x.minimum_ascent);
                grid_requirements.minimum_descent = (std::max)(
                    grid_requirements.minimum_descent, x.minimum_descent);
            }
        }

        cache->last_update = grid.container->last_content_change;
    }
    return cache->requirements;
}

layout_requirements
uniform_grid_row_container::get_vertical_requirements(
    layout_calculation_context& ctx, layout_scalar assigned_width)
{
    vertical_layout_query query(ctx, cacher,
        grid->container->last_content_change, assigned_width);
    alia_if (query.update_required())
    {
        query.update(
            calculate_uniform_grid_vertical_requirements(
                ctx, *grid, assigned_width));
    }
    alia_end
    return query.result();
}

void set_grid_row_relative_assignment(
    layout_calculation_context& ctx,
    uniform_grid_data& grid,
    layout_node* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    std::vector<layout_scalar> const& column_widths =
        calculate_uniform_grid_column_widths(grid, assigned_size[0]);
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

void uniform_grid_row_container::set_relative_assignment(
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

void uniform_grid_layout::concrete_begin(
    layout_traversal& traversal, layout const& layout_spec,
    float column_spacing, layout_units spacing_units)
{
    traversal_ = &traversal;

    get_data(*traversal.data, &data_);
    if (traversal.is_refresh_pass)
        data_->rows = 0;

    simple_layout_container* container;
    column_layout_logic* logic;
    get_simple_layout_container(traversal, &container, &logic, layout_spec);
    container_.begin(traversal, container);

    begin_transform(transform_, traversal, container->cacher);

    data_->container = container;

    layout_scalar resolved_spacing =
        resolve_layout_width(traversal, column_spacing, spacing_units);
    detect_layout_change(traversal, &data_->column_spacing, resolved_spacing);
}

void uniform_grid_row::begin(
    uniform_grid_layout const& grid, layout const& layout_spec)
{
    layout_traversal& traversal = *grid.traversal_;

    uniform_grid_row_container* row;
    if (get_data(*traversal.data, &row))
        initialize(traversal, *row);

    // Add this row to the grid's list of children.
    // It doesn't matter what order the list is in, and adding the row to the
    // front of the list is easier.
    if (traversal.is_refresh_pass)
    {
        row->next = grid.data_->rows;
        grid.data_->rows = row;
        row->grid = grid.data_;
    }

    container_.begin(traversal, row);

    // All rows must be set to GROW to ensure they receive equal space from the
    // column that contains them.
    update(traversal, row->cacher, GROW | UNPADDED, GROW | UNPADDED);

    begin_transform(transform_, traversal, row->cacher);
}

void uniform_grid_row::end()
{
    transform_.end();
    container_.end();
}

// OVERLAY LAYOUT

struct overlay_layout_data
{
    layout_node* root_node;
    data_graph measurement_cache, placement_cache;
    layout_vector size;
    overlay_layout_data() : root_node(0) {}
};

void overlay_layout::concrete_begin(
    layout_traversal& traversal,
    layout_vector const& max_size)
{
    get_cached_data(*traversal.data, &data_);

    traversal_ = &traversal;

    old_container_ = traversal.active_container;
    old_next_ptr_ = traversal.next_ptr;

    traversal.active_container = 0;
    traversal.next_ptr = &data_->root_node;

    clipping_reset_.begin(*traversal.geometry);

    max_size_ = max_size;
}

void overlay_layout::end()
{
    if (traversal_)
    {
        clipping_reset_.end();

        if (traversal_->is_refresh_pass)
        {
            layout_vector measured_size =
                get_minimum_size(data_->root_node, data_->measurement_cache);
            for (unsigned i = 0; i != 2; ++i)
                data_->size[i] = (std::min)(max_size_[i], measured_size[i]);
            resolve_layout(data_->root_node, data_->placement_cache,
                data_->size);
        }

        traversal_->active_container = old_container_;
        traversal_->next_ptr = old_next_ptr_;
        traversal_ = 0;
    }
}

layout_vector overlay_layout::size() const
{
    return data_->size;
}

}
