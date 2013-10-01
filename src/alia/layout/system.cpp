#include <alia/layout/system.hpp>
#include <alia/layout/utilities.hpp>

namespace alia {

static void initialize_traversal(
    layout_system& system, layout_traversal& traversal,
    bool is_refresh, geometry_context* geometry, layout_style_info* style,
    vector<2,float> const& ppi)
{
    traversal.system = &system;
    traversal.active_container = 0;
    traversal.next_ptr = &system.root_node;
    traversal.is_refresh_pass = is_refresh;
    traversal.refresh_counter = system.refresh_counter;
    traversal.geometry = geometry;
    traversal.style_info = style;
    traversal.ppi = ppi;

    style->font_size = 0;
    style->character_size = make_vector<float>(0, 0);
    style->x_height = 0;
    style->padding_size = make_layout_vector(0, 0);
    style->magnification = 1;
}
void scoped_layout_traversal::begin(
    layout_system& system, layout_traversal& traversal,
    geometry_context& geometry, vector<2,float> const& ppi)
{
    initialize_traversal(system, traversal, false, &geometry,
        &dummy_style_info_, ppi);
}
void scoped_layout_refresh::begin(
    layout_system& system, layout_traversal& traversal,
    vector<2,float> const& ppi)
{
    initialize_traversal(system, traversal, true, 0, &dummy_style_info_, ppi);
}

// scoped_layout_calculation_context sets up a calculation context for a
// layout system.
struct scoped_layout_calculation_context
{
    scoped_layout_calculation_context() {}

    scoped_layout_calculation_context(
        data_graph& cache, layout_calculation_context& ctx)
    { begin(cache, ctx); }

    ~scoped_layout_calculation_context() { end(); }

    void begin(data_graph& cache, layout_calculation_context& ctx);

    void end();

private:
    scoped_data_traversal data_;
};

void scoped_layout_calculation_context::begin(
    data_graph& cache, layout_calculation_context& ctx)
{
    data_.begin(cache, ctx.data);
    ctx.map = retrieve_naming_map(ctx.data);
    ctx.for_measurement = false;
}
void scoped_layout_calculation_context::end()
{
    data_.end();
}

void resolve_layout(layout_node* root_node, data_graph& cache,
    layout_vector const& size)
{
    if (root_node)
    {
        layout_calculation_context ctx;
        scoped_layout_calculation_context slcc(cache, ctx);
        get_horizontal_requirements(ctx, *root_node);
        layout_requirements y =
            get_vertical_requirements(ctx, *root_node, size[0]);
        set_relative_assignment(ctx, *root_node,
            relative_layout_assignment(
                layout_box(make_layout_vector(0, 0), size), y.ascent));
    }
}

void resolve_layout(layout_system& system, layout_vector const& size)
{
    resolve_layout(system.root_node, system.calculation_cache, size);
    // Increment the refresh counter immediately after resolving layout so
    // that any changes detected after this will be associated with the new
    // counter value and thus cause a recalculation.
    ++system.refresh_counter;
}

layout_vector get_minimum_size(layout_node* root_node, data_graph& cache)
{
    if (root_node)
    {
        layout_calculation_context ctx;
        scoped_layout_calculation_context slcc(cache, ctx);
        ctx.for_measurement = true;
        layout_requirements horizontal =
            get_horizontal_requirements(ctx, *root_node);
        layout_requirements vertical =
            get_vertical_requirements(ctx, *root_node, horizontal.size);
        return make_layout_vector(horizontal.size, vertical.size);
    }
    else
        return make_layout_vector(0, 0);
}

layout_vector get_minimum_size(layout_system& system)
{
    return get_minimum_size(system.root_node, system.calculation_cache);
}

}
