#include <alia/layout/system.hpp>
#include <alia/layout/utilities.hpp>
#include <alia/layout/api.hpp>
#include <utility>

#define BOOST_TEST_MODULE layout
#include "test.hpp"

struct testing_context
{
    alia::data_traversal* data;
    alia::layout_traversal* layout;
};

alia::data_traversal& get_data_traversal(testing_context& ctx)
{ return *ctx.data; }
alia::layout_traversal& get_layout_traversal(testing_context& ctx)
{ return *ctx.layout; }

struct test_leaf
{
    alia::layout_leaf node;
    alia::layout_vector absolute_position;
};

test_leaf test_leaf1, test_leaf2;

void do_test_leaf(alia::layout_traversal& traversal, test_leaf& leaf,
    alia::layout const& layout_spec)
{
    if (traversal.is_refresh_pass)
    {
        leaf.node.refresh_layout(traversal, layout_spec,
            alia::leaf_layout_requirements(
                alia::make_layout_vector(0, 0), 0, 0));
        add_layout_node(traversal, &leaf.node);
    }
    else
    {
        alia::vector<2,double> p = transform(
            traversal.geometry->transformation_matrix,
            alia::vector<2,double>(leaf.node.assignment().region.corner));
        leaf.absolute_position = alia::make_vector(
            alia::round_to_layout_scalar(p[0]),
            alia::round_to_layout_scalar(p[1]));
    }
}

void do_test_leaf1(
    testing_context& ctx, alia::layout const& layout_spec)
{
    do_test_leaf(*ctx.layout, test_leaf1, layout_spec);
}
void do_test_leaf2(
    testing_context& ctx, alia::layout const& layout_spec)
{
    do_test_leaf(*ctx.layout, test_leaf2, layout_spec);
}

#define DO_1LEAF_TEST(ui_code, assigned_size, expected_region1) \
  { \
    layout_system system; \
    data_graph graph; \
    \
    { \
        alia::data_traversal data_traversal; \
        scoped_data_traversal sdt(graph, data_traversal); \
        alia::layout_traversal layout_traversal; \
        scoped_layout_refresh slr(system, layout_traversal, \
            make_vector<float>(1, 1));        \
        testing_context ctx; \
        ctx.data = &data_traversal; \
        ctx.layout = &layout_traversal; \
        ui_code \
    } \
    \
    resolve_layout(system, assigned_size); \
    \
    { \
        alia::data_traversal data_traversal; \
        scoped_data_traversal sdt(graph, data_traversal); \
        alia::geometry_context geometry; \
        initialize(geometry, box<2,double>(make_vector(0., 0.), \
            vector<2,double>(assigned_size))); \
        alia::layout_traversal layout_traversal; \
        scoped_layout_traversal slr(system, layout_traversal, geometry, \
            make_vector<float>(1, 1)); \
        testing_context ctx; \
        ctx.data = &data_traversal; \
        ctx.layout = &layout_traversal; \
        ui_code \
    } \
    \
    BOOST_CHECK_EQUAL(test_leaf1.absolute_position, \
        expected_region1.corner); \
    BOOST_CHECK_EQUAL(test_leaf1.node.assignment().region.size, \
        expected_region1.size); \
  }

#define DO_2LEAF_TEST(ui_code, assigned_size, expected_region1, \
    expected_region2) \
  { \
    layout_system system; \
    data_graph graph; \
    \
    { \
        alia::data_traversal data_traversal; \
        scoped_data_traversal sdt(graph, data_traversal); \
        alia::layout_traversal layout_traversal; \
        scoped_layout_refresh slr(system, layout_traversal, \
            make_vector<float>(1, 1)); \
        testing_context ctx; \
        ctx.data = &data_traversal; \
        ctx.layout = &layout_traversal; \
        ui_code \
    } \
    \
    resolve_layout(system, assigned_size); \
    \
    { \
        alia::data_traversal data_traversal; \
        scoped_data_traversal sdt(graph, data_traversal); \
        alia::geometry_context geometry; \
        initialize(geometry, box<2,double>(make_vector(0., 0.), \
            vector<2,double>(assigned_size))); \
        alia::layout_traversal layout_traversal; \
        scoped_layout_traversal slr(system, layout_traversal, geometry, \
            make_vector<float>(1, 1)); \
        testing_context ctx; \
        ctx.data = &data_traversal; \
        ctx.layout = &layout_traversal; \
        ui_code \
    } \
    \
    BOOST_CHECK_EQUAL(test_leaf1.absolute_position, \
        expected_region1.corner); \
    BOOST_CHECK_EQUAL(test_leaf1.node.assignment().region.size, \
        expected_region1.size); \
    BOOST_CHECK_EQUAL(test_leaf2.absolute_position, \
        expected_region2.corner); \
    BOOST_CHECK_EQUAL(test_leaf2.node.assignment().region.size, \
        expected_region2.size); \
  }

BOOST_AUTO_TEST_CASE(layout_test)
{
    using namespace alia;

    // LEAF-ONLY TESTS

    DO_1LEAF_TEST(
        {
            do_test_leaf1(ctx, size(50, 100, PIXELS));
        },
        make_layout_vector(50, 100),
        layout_box(make_layout_vector(0, 0), make_layout_vector(50, 100)));

    // ALIGNMENT TESTS

    DO_1LEAF_TEST(
        {
            do_test_leaf1(ctx, layout(size(10, 10, PIXELS), BOTTOM | RIGHT));
        },
        make_layout_vector(100, 100),
        layout_box(make_layout_vector(90, 90), make_layout_vector(10, 10)));
    DO_1LEAF_TEST(
        {
            do_test_leaf1(ctx, layout(size(10, 10, PIXELS), TOP | LEFT));
        },
        make_layout_vector(100, 100),
        layout_box(make_layout_vector(0, 0), make_layout_vector(10, 10)));
    DO_1LEAF_TEST(
        {
            do_test_leaf1(ctx, layout(size(10, 10, PIXELS), BOTTOM | LEFT));
        },
        make_layout_vector(100, 100),
        layout_box(make_layout_vector(0, 90), make_layout_vector(10, 10)));
    DO_1LEAF_TEST(
        {
            do_test_leaf1(ctx, layout(size(10, 10, PIXELS), FILL));
        },
        make_layout_vector(100, 100),
        layout_box(make_layout_vector(0, 0), make_layout_vector(100, 100)));
    DO_1LEAF_TEST(
        {
            do_test_leaf1(ctx, layout(size(10, 10, PIXELS), FILL_X | TOP));
        },
        make_layout_vector(100, 100),
        layout_box(make_layout_vector(0, 0), make_layout_vector(100, 10)));
    DO_1LEAF_TEST(
        {
            do_test_leaf1(ctx, layout(size(10, 10, PIXELS), FILL_Y | RIGHT));
        },
        make_layout_vector(100, 100),
        layout_box(make_layout_vector(90, 0), make_layout_vector(10, 100)));

    // ROW, COLUMN, AND LINEAR TESTS

    DO_2LEAF_TEST(
        {
            row_layout row(ctx);
            do_test_leaf1(ctx, layout(size(100, 100, PIXELS)));
            do_test_leaf2(ctx, GROW);
        },
        make_layout_vector(300, 100),
        layout_box(make_layout_vector(0, 0), make_layout_vector(100, 100)),
        layout_box(make_layout_vector(100, 0), make_layout_vector(200, 100)));

    DO_2LEAF_TEST(
        {
            column_layout column(ctx);
            do_test_leaf1(ctx, layout(size(100, 100, PIXELS)));
            do_test_leaf2(ctx, GROW);
        },
        make_layout_vector(100, 300),
        layout_box(make_layout_vector(0, 0), make_layout_vector(100, 100)),
        layout_box(make_layout_vector(0, 100), make_layout_vector(100, 200)));

    DO_1LEAF_TEST(
        {
            linear_layout column(ctx, VERTICAL_LAYOUT);
            do_test_leaf1(ctx, size(100, 100, PIXELS));
        },
        make_layout_vector(100, 200),
        layout_box(make_layout_vector(0, 0), make_layout_vector(100, 100)));

    DO_1LEAF_TEST(
        {
            linear_layout column(ctx, VERTICAL_LAYOUT);
            do_test_leaf1(ctx, layout(size(100, 100, PIXELS), LEFT));
        },
        make_layout_vector(200, 200),
        layout_box(make_layout_vector(0, 0), make_layout_vector(100, 100)));

    DO_1LEAF_TEST(
        {
            linear_layout column(ctx, VERTICAL_LAYOUT);
            do_test_leaf1(ctx, layout(FILL, 1));
        },
        make_layout_vector(100, 200),
        layout_box(make_layout_vector(0, 0), make_layout_vector(100, 200)));

    DO_1LEAF_TEST(
        {
            linear_layout column(ctx, VERTICAL_LAYOUT);
            do_test_leaf1(ctx, GROW);
        },
        make_layout_vector(100, 200),
        layout_box(make_layout_vector(0, 0), make_layout_vector(100, 200)));

    // LAYERED TESTS

    DO_2LEAF_TEST(
        {
            layered_layout layered(ctx);
            do_test_leaf1(ctx, layout(size(100, 100, PIXELS)));
            do_test_leaf2(ctx, GROW);
        },
        make_layout_vector(300, 100),
        layout_box(make_layout_vector(0, 0), make_layout_vector(100, 100)),
        layout_box(make_layout_vector(0, 0), make_layout_vector(300, 100)));

    DO_2LEAF_TEST(
        {
            column_layout column(ctx);
            do_spacer(ctx, GROW);
            {
                layered_layout layered(ctx);
                do_test_leaf1(ctx, size(10, 10, PIXELS));
                do_test_leaf2(ctx, FILL);
            }
        },
        make_layout_vector(100, 100),
        layout_box(make_layout_vector(0, 90), make_layout_vector(10, 10)),
        layout_box(make_layout_vector(0, 90), make_layout_vector(100, 10)));

    DO_2LEAF_TEST(
        {
            column_layout column(ctx);
            do_spacer(ctx, GROW);
            {
                layered_layout layered(ctx, GROW);
                do_test_leaf1(ctx, size(10, 10, PIXELS));
                do_test_leaf2(ctx, FILL);
            }
        },
        make_layout_vector(100, 100),
        layout_box(make_layout_vector(0, 45), make_layout_vector(10, 10)),
        layout_box(make_layout_vector(0, 45), make_layout_vector(100, 55)));

    // ROTATED TESTS

    DO_2LEAF_TEST(
        {
            column_layout column(ctx);
            {
                rotated_layout rotated(ctx);
                {
                    column_layout column(ctx);
                    do_spacer(ctx, size(10, 20, PIXELS));
                    do_test_leaf1(ctx, GROW);
                }
            }
            do_test_leaf2(ctx, GROW);
        },
        make_layout_vector(100, 100),
        layout_box(make_layout_vector(20, 10), make_layout_vector(10, 80)),
        layout_box(make_layout_vector(0, 10), make_layout_vector(100, 90)));

    // FLOW TESTS

    DO_2LEAF_TEST(
        {
            flow_layout flow(ctx);
            {
                do_spacer(ctx, size(10, 20, PIXELS));
                do_test_leaf1(ctx, size(20, 10, PIXELS));
                do_spacer(ctx, size(30, 20, PIXELS));
                do_test_leaf2(ctx, layout(size(20, 10, PIXELS), FILL));
            }
        },
        make_layout_vector(50, 50),
        layout_box(make_layout_vector(10, 0), make_layout_vector(20, 10)),
        layout_box(make_layout_vector(30, 20), make_layout_vector(20, 20)));

    DO_2LEAF_TEST(
        {
            column_layout column(ctx);
            {
                flow_layout flow(ctx);
                {
                    do_spacer(ctx, size(10, 20, PIXELS));
                    do_test_leaf1(ctx, size(20, 10, PIXELS));
                    do_spacer(ctx, size(30, 20, PIXELS));
                    do_spacer(ctx, layout(size(20, 10, PIXELS), FILL));
                }
            }
            do_test_leaf2(ctx, layout(size(20, 10, PIXELS)));
        },
        make_layout_vector(50, 100),
        layout_box(make_layout_vector(10, 0), make_layout_vector(20, 10)),
        layout_box(make_layout_vector(0, 40), make_layout_vector(20, 10)));

    // VERTICAL FLOW TESTS

    DO_2LEAF_TEST(
        {
            vertical_flow_layout flow(ctx);
            {
                do_spacer(ctx, size(20, 20, PIXELS));
                do_test_leaf1(ctx, layout(size(10, 20, PIXELS), FILL));
                do_spacer(ctx, size(20, 10, PIXELS));
                do_test_leaf2(ctx, layout(size(20, 10, PIXELS), FILL));
            }
        },
        make_layout_vector(50, 50),
        layout_box(make_layout_vector(0, 20), make_layout_vector(20, 20)),
        layout_box(make_layout_vector(20, 10), make_layout_vector(20, 10)));

    DO_2LEAF_TEST(
        {
            column_layout column(ctx);
            {
                vertical_flow_layout flow(ctx);
                {
                    do_spacer(ctx, size(10, 20, PIXELS));
                    do_test_leaf1(ctx, layout(size(10, 20, PIXELS), FILL));
                    do_spacer(ctx, size(20, 10, PIXELS));
                    do_spacer(ctx, layout(size(20, 10, PIXELS), FILL));
                }
            }
            do_test_leaf2(ctx, size(20, 10, PIXELS));
        },
        make_layout_vector(50, 100),
        layout_box(make_layout_vector(0, 20), make_layout_vector(20, 20)),
        layout_box(make_layout_vector(0, 40), make_layout_vector(20, 10)));

    // GRID TESTS

    // simple
    DO_2LEAF_TEST(
        {
            grid_layout grid(ctx);
            {
                grid_row r(grid);
                do_spacer(ctx, size(20, 20, PIXELS));
                do_spacer(ctx, size(30, 10, PIXELS));
            }
            {
                grid_row r(grid);
                do_test_leaf1(ctx, layout(size(10, 20, PIXELS), FILL));
                do_test_leaf2(ctx, layout(size(10, 10, PIXELS), TOP | LEFT));
            }
        },
        make_layout_vector(50, 50),
        layout_box(make_layout_vector(0, 20), make_layout_vector(20, 20)),
        layout_box(make_layout_vector(20, 20), make_layout_vector(10, 10)));

    // with column proportions
    DO_2LEAF_TEST(
        {
            grid_layout grid(ctx);
            {
                grid_row r(grid);
                do_spacer(ctx, size(20, 20, PIXELS));
                do_spacer(ctx, layout(size(30, 10, PIXELS), GROW));
            }
            {
                grid_row r(grid);
                do_test_leaf1(ctx, layout(size(10, 20, PIXELS), FILL));
                do_test_leaf2(ctx, layout(size(10, 10, PIXELS), FILL));
            }
        },
        make_layout_vector(100, 50),
        layout_box(make_layout_vector(0, 20), make_layout_vector(20, 20)),
        layout_box(make_layout_vector(20, 20), make_layout_vector(80, 20)));

    // with column spacing
    DO_2LEAF_TEST(
        {
            grid_layout grid(ctx, default_layout, absolute_length(10, PIXELS));
            {
                grid_row r(grid);
                do_spacer(ctx, size(20, 20, PIXELS));
                do_spacer(ctx, layout(size(30, 10, PIXELS), GROW));
            }
            {
                grid_row r(grid);
                do_test_leaf1(ctx, layout(size(10, 20, PIXELS), FILL));
                do_test_leaf2(ctx, layout(size(10, 10, PIXELS), FILL));
            }
        },
        make_layout_vector(100, 50),
        layout_box(make_layout_vector(0, 20), make_layout_vector(20, 20)),
        layout_box(make_layout_vector(30, 20), make_layout_vector(70, 20)));

    // with backward flow of information
    DO_1LEAF_TEST(
        {
            grid_layout grid(ctx);
            {
                column_layout c(ctx, LEFT);
                {
                    grid_row r(grid);
                    do_spacer(ctx, size(20, 20, PIXELS));
                    do_spacer(ctx, size(20, 10, PIXELS));
                }
                do_test_leaf1(ctx, layout(size(10, 20, PIXELS), FILL));
            }
            {
                grid_row r(grid);
                do_spacer(ctx, size(20, 20, PIXELS));
                do_spacer(ctx, size(50, 10, PIXELS));
            }
        },
        make_layout_vector(100, 50),
        layout_box(make_layout_vector(0, 20), make_layout_vector(70, 20)));

    // UNIFORM GRID TESTS

    // simple
    DO_2LEAF_TEST(
        {
            column_layout c(ctx);
            {
                uniform_grid_layout grid(ctx);
                {
                    uniform_grid_row r(grid);
                    do_spacer(ctx, size(20, 20, PIXELS));
                    do_spacer(ctx, size(30, 10, PIXELS));
                }
                {
                    uniform_grid_row r(grid);
                    do_test_leaf1(ctx, layout(size(10, 20, PIXELS), FILL));
                    do_test_leaf2(ctx, layout(size(10, 10, PIXELS),
                        BOTTOM | LEFT));
                }
            }
        },
        make_layout_vector(60, 60),
        layout_box(make_layout_vector(0, 20), make_layout_vector(30, 20)),
        layout_box(make_layout_vector(30, 30), make_layout_vector(10, 10)));

    // with the grid expanding to fill the column
    DO_2LEAF_TEST(
        {
            column_layout c(ctx);
            {
                uniform_grid_layout grid(ctx, GROW);
                {
                    uniform_grid_row r(grid);
                    do_spacer(ctx, size(20, 20, PIXELS));
                    do_spacer(ctx, size(30, 10, PIXELS));
                }
                {
                    uniform_grid_row r(grid);
                    do_test_leaf1(ctx, layout(size(10, 20, PIXELS), FILL));
                    do_test_leaf2(ctx, layout(size(10, 10, PIXELS),
                        BOTTOM | LEFT));
                }
            }
        },
        make_layout_vector(60, 60),
        layout_box(make_layout_vector(0, 30), make_layout_vector(30, 30)),
        layout_box(make_layout_vector(30, 50), make_layout_vector(10, 10)));

    // with column proportions (which have no effect)
    DO_2LEAF_TEST(
        {
            column_layout c(ctx);
            {
                uniform_grid_layout grid(ctx);
                {
                    uniform_grid_row r(grid);
                    do_spacer(ctx, size(20, 20, PIXELS));
                    do_spacer(ctx, layout(size(30, 10, PIXELS), GROW));
                }
                {
                    uniform_grid_row r(grid);
                    do_test_leaf1(ctx, layout(size(10, 20, PIXELS), FILL));
                    do_test_leaf2(ctx, layout(size(10, 10, PIXELS), FILL));
                }
            }
        },
        make_layout_vector(100, 50),
        layout_box(make_layout_vector(0, 20), make_layout_vector(50, 20)),
        layout_box(make_layout_vector(50, 20), make_layout_vector(50, 20)));

    // with column spacing
    DO_2LEAF_TEST(
        {
            column_layout c(ctx);
            {
                uniform_grid_layout grid(ctx, default_layout,
                    absolute_length(10, PIXELS));
                {
                    uniform_grid_row r(grid);
                    do_spacer(ctx, size(20, 20, PIXELS));
                    do_spacer(ctx, size(30, 10, PIXELS));
                }
                {
                    uniform_grid_row r(grid);
                    do_test_leaf1(ctx, layout(size(10, 20, PIXELS), FILL));
                    do_test_leaf2(ctx, layout(size(10, 10, PIXELS),
                        BOTTOM | LEFT));
                }
            }
        },
        make_layout_vector(100, 60),
        layout_box(make_layout_vector(0, 20), make_layout_vector(45, 20)),
        layout_box(make_layout_vector(55, 30), make_layout_vector(10, 10)));

    // with backward flow of information
    DO_1LEAF_TEST(
        {
            column_layout c(ctx);
            {
                uniform_grid_layout grid(ctx);
                {
                    column_layout c(ctx, LEFT);
                    {
                        uniform_grid_row r(grid);
                        do_spacer(ctx, size(20, 20, PIXELS));
                        do_spacer(ctx, size(20, 10, PIXELS));
                    }
                    do_test_leaf1(ctx, layout(size(10, 20, PIXELS), FILL));
                }
                {
                    uniform_grid_row r(grid);
                    do_spacer(ctx, size(20, 30, PIXELS));
                    do_spacer(ctx, size(40, 10, PIXELS));
                }
            }
        },
        make_layout_vector(100, 100),
        layout_box(make_layout_vector(0, 30), make_layout_vector(80, 20)));
}
