#define ALIA_LOWERCASE_MACROS
#include <alia/flow/data_graph.hpp>

#include <alia/flow/macros.hpp>
#include <alia/signals/basic.hpp>

#include "flow/testing.hpp"

TEST_CASE("basic data traversal", "[flow][data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto controller = [](context ctx) { do_int(ctx, 0); };
        do_traversal(graph, controller);
        check_log("initializing int: 0;");
        do_traversal(graph, controller);
        check_log("visiting int: 0;");
    }
    check_log("destructing int;");
}

TEST_CASE("simple named blocks", "[flow][data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](std::vector<int> indices) {
            return [=](context ctx) {
                naming_context nc(ctx);
                for (auto i : indices)
                {
                    named_block nb(nc, make_id(i));
                    do_int(ctx, i);
                }
                do_int(ctx, 0);
            };
        };
        do_traversal(graph, make_controller({1}));
        check_log(
            "initializing int: 1;"
            "initializing int: 0;");
        do_traversal(graph, make_controller({2}));
        check_log(
            "initializing int: 2;"
            "visiting int: 0;"
            "destructing int;");
        do_traversal(graph, make_controller({1, 2}));
        check_log(
            "initializing int: 1;"
            "visiting int: 2;"
            "visiting int: 0;");
        do_traversal(graph, make_controller({2, 3}));
        check_log(
            "visiting int: 2;"
            "initializing int: 3;"
            "visiting int: 0;"
            "destructing int;");
        do_traversal(graph, make_controller({2, 1, 3}));
        check_log(
            "visiting int: 2;"
            "initializing int: 1;"
            "visiting int: 3;"
            "visiting int: 0;");
    }
    check_log(
        "destructing int;"
        "destructing int;"
        "destructing int;"
        "destructing int;");
}

TEST_CASE("mobile named blocks", "[flow][data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](std::vector<int> indices, int divider) {
            return [=](context ctx) {
                naming_context nc(ctx);
                ALIA_FOR(auto i : indices)
                {
                    ;
                    ALIA_IF(i < divider)
                    {
                        named_block nb(nc, make_id(i));
                        do_int(ctx, i);
                    }
                    ALIA_END
                }
                ALIA_END
                alia_for(auto i : indices)
                {
                    ;
                    alia_if(i >= divider)
                    {
                        named_block nb(nc, make_id(i));
                        do_int(ctx, i);
                    }
                    alia_end
                }
                alia_end
            };
        };
        do_traversal(graph, make_controller({3, 2, 1}, 2));
        check_log(
            "initializing int: 1;"
            "initializing int: 3;"
            "initializing int: 2;");
        do_traversal(graph, make_controller({3, 2, 1}, 3));
        check_log(
            "visiting int: 2;"
            "visiting int: 1;"
            "visiting int: 3;");
        do_traversal(graph, make_controller({3, 1}, 3));
        check_log(
            "visiting int: 1;"
            "visiting int: 3;");
    }
    check_log(
        "destructing int;"
        "destructing int;"
        "destructing int;");
}

TEST_CASE("multiple naming contexts", "[flow][data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](std::vector<int> indices) {
            return [=](context ctx) {
                {
                    naming_context nc(ctx);
                    for (auto i : indices)
                    {
                        named_block nb(nc, make_id(i));
                        do_int(ctx, i);
                    }
                }
                // Do the same thing again with the same names but with all the
                // do_int values doubled. This would cause conflicts if they
                // didn't have separate data from the ones above.
                {
                    naming_context nc(ctx);
                    for (auto i : indices)
                    {
                        named_block nb(nc, make_id(i));
                        do_int(ctx, i * 2);
                    }
                }
                do_int(ctx, 0);
            };
        };
        do_traversal(graph, make_controller({1}));
        check_log(
            "initializing int: 1;"
            "initializing int: 2;"
            "initializing int: 0;");
        do_traversal(graph, make_controller({2}));
        check_log(
            "initializing int: 2;"
            "initializing int: 4;"
            "visiting int: 0;"
            "destructing int;"
            "destructing int;");
        do_traversal(graph, make_controller({1, 2}));
        check_log(
            "initializing int: 1;"
            "visiting int: 2;"
            "initializing int: 2;"
            "visiting int: 4;"
            "visiting int: 0;");
        do_traversal(graph, make_controller({2, 3}));
        check_log(
            "visiting int: 2;"
            "initializing int: 3;"
            "visiting int: 4;"
            "initializing int: 6;"
            "visiting int: 0;"
            "destructing int;"
            "destructing int;");
    }
    check_log(
        "destructing int;"
        "destructing int;"
        "destructing int;"
        "destructing int;"
        "destructing int;");
}

TEST_CASE("unexecuted named blocks", "[data_graph]")
{
    // Test that named blocks aren't GC'd if they're in unexecuted blocks.
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition, std::vector<int> indices) {
            return [=](context ctx) {
                ALIA_IF(condition)
                {
                    naming_context nc(ctx);
                    for (auto i : indices)
                    {
                        named_block nb(nc, make_id(i));
                        do_int(ctx, i);
                    }
                }
                ALIA_END
                do_int(ctx, 0);
            };
        };
        do_traversal(graph, make_controller(value(true), {1}));
        check_log(
            "initializing int: 1;"
            "initializing int: 0;");
        do_traversal(graph, make_controller(value(false), {1}));
        check_log("visiting int: 0;");
        do_traversal(graph, make_controller(value(true), {2, 1}));
        check_log(
            "initializing int: 2;"
            "visiting int: 1;"
            "visiting int: 0;");
    }
    check_log(
        "destructing int;"
        "destructing int;"
        "destructing int;");
}

TEST_CASE("GC disabling", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](std::vector<int> indices) {
            return [=](context ctx) {
                {
                    naming_context nc(ctx);
                    for (auto i : indices)
                    {
                        named_block nb(nc, make_id(i));
                        do_int(ctx, i);
                    }
                }
                do_int(ctx, 0);
            };
        };
        // These traversals are fine because they have GC enabled.
        do_traversal(graph, make_controller({1, 2}), true);
        check_log(
            "initializing int: 1;"
            "initializing int: 2;"
            "initializing int: 0;");
        do_traversal(graph, make_controller({2, 1}), true);
        check_log(
            "visiting int: 2;"
            "visiting int: 1;"
            "visiting int: 0;");
        // This traversal is fine because it maintains the previous order.
        do_traversal(graph, make_controller({2, 1}), false);
        check_log(
            "visiting int: 2;"
            "visiting int: 1;"
            "visiting int: 0;");
        // This traversal is an error because it tries to change the order
        // with GC disabled.
        REQUIRE_THROWS_AS(
            do_traversal(graph, make_controller({1, 2}), false),
            named_block_out_of_order);
    }
    check_log(
        "destructing int;"
        "destructing int;"
        "destructing int;");
}

TEST_CASE("manual deletion", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](std::vector<int> indices) {
            return [=](context ctx) {
                naming_context nc(ctx);
                for (auto i : indices)
                {
                    // Odd indices will require manual deletion.
                    named_block nb(nc, make_id(i), manual_delete((i & 1) != 0));
                    do_int(ctx, i);
                }
                do_int(ctx, 0);
            };
        };
        do_traversal(graph, make_controller({1}));
        check_log(
            "initializing int: 1;"
            "initializing int: 0;");
        // Test that manual_delete blocks aren't GC'd when they're not seen.
        do_traversal(graph, make_controller({2, 3}));
        check_log(
            "initializing int: 2;"
            "initializing int: 3;"
            "visiting int: 0;");
        // Test that normal blocks are still GC'd.
        do_traversal(graph, make_controller({1}));
        check_log(
            "visiting int: 1;"
            "visiting int: 0;"
            "destructing int;");
        // Test that normal blocks are still GC'd.
        do_traversal(graph, make_controller({3, 1}));
        check_log(
            "visiting int: 3;"
            "visiting int: 1;"
            "visiting int: 0;");
        // Test that normal blocks are still GC'd.
        do_traversal(graph, make_controller({1}));
        check_log(
            "visiting int: 1;"
            "visiting int: 0;");
        // Test manual deletion.
        delete_named_block(graph, make_id(3));
        check_log("destructing int;");
        // Test that manual deletion has no effect on blocks that are still
        // active.
        delete_named_block(graph, make_id(1));
        check_log("");
        do_traversal(graph, make_controller({1}));
        check_log(
            "visiting int: 1;"
            "visiting int: 0;");
    }
    check_log(
        "destructing int;"
        "destructing int;");
}

TEST_CASE("named block caching", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition, std::vector<int> indices) {
            return [=](context ctx) {
                ALIA_IF(condition)
                {
                    naming_context nc(ctx);
                    for (auto i : indices)
                    {
                        named_block nb(nc, make_id(i));
                        ALIA_IF(value(true))
                        {
                            do_cached_int(ctx, i);
                        }
                        ALIA_END
                    }
                }
                ALIA_END
            };
        };
        do_traversal(graph, make_controller(value(true), {2, 1}));
        check_log(
            "initializing cached int: 2;"
            "initializing cached int: 1;");
        do_traversal(graph, make_controller(value(true), {2, 1}));
        check_log(
            "visiting cached int: 2;"
            "visiting cached int: 1;");
        do_traversal(graph, make_controller(value(true), {1}));
        check_log(
            "visiting cached int: 1;"
            "destructing int;");
        do_traversal(graph, make_controller(value(true), {2, 1}));
        check_log(
            "initializing cached int: 2;"
            "visiting cached int: 1;");
        do_traversal(graph, make_controller(value(false), {2, 1}));
        check_log(
            "destructing int;"
            "destructing int;");
        do_traversal(graph, make_controller(value(true), {2, 1}));
        check_log(
            "initializing cached int: 2;"
            "initializing cached int: 1;");
        do_traversal(graph, make_controller(value(false), {}));
        check_log(
            "destructing int;"
            "destructing int;");
        do_traversal(graph, make_controller(value(true), {2, 1}));
        check_log(
            "initializing cached int: 2;"
            "initializing cached int: 1;");
    }
    check_log(
        "destructing int;"
        "destructing int;");
}

TEST_CASE("naming_map lifetime", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;

        auto make_controller = [](auto condition, std::vector<int> indices) {
            return [=](context ctx) {
                ALIA_IF(condition)
                {
                    naming_context nc(ctx);
                    for (auto i : indices)
                    {
                        named_block nb(nc, make_id(i));
                        {
                            naming_context inner_nc(ctx);
                            named_block inner_nb(inner_nc, make_id("inner"));
                            do_int(ctx, i);
                        }
                    }
                }
                ALIA_END
            };
        };
        do_traversal(graph, make_controller(value(true), {2, 1}));
        check_log(
            "initializing int: 2;"
            "initializing int: 1;");
        do_traversal(graph, make_controller(value(true), {2, 3, 1}));
        check_log(
            "visiting int: 2;"
            "initializing int: 3;"
            "visiting int: 1;");
        do_traversal(graph, make_controller(value(true), {2, 1}));
        check_log(
            "visiting int: 2;"
            "visiting int: 1;"
            "destructing int;");
        do_traversal(graph, make_controller(value(true), {}));
        check_log(
            "destructing int;"
            "destructing int;");
    }
}

TEST_CASE("scoped_cache_clearing_disabler", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition) {
            return [=](context ctx) {
                {
                    scoped_cache_clearing_disabler disabler(ctx);
                    ALIA_IF(condition)
                    {
                        do_cached_int(ctx, 0);
                    }
                    ALIA_ELSE
                    {
                        do_cached_int(ctx, 1);
                    }
                    ALIA_END
                }
                ALIA_IF(condition)
                {
                    do_cached_int(ctx, 2);
                }
                ALIA_END
            };
        };
        // Since we are disabling cache clearing for the first block, the 0 and
        // 1 will persist even when their blocks go inactive.
        do_traversal(graph, make_controller(value(false)));
        check_log("initializing cached int: 1;");
        do_traversal(graph, make_controller(value(true)));
        check_log(
            "initializing cached int: 0;"
            "initializing cached int: 2;");
        do_traversal(graph, make_controller(value(false)));
        check_log(
            "visiting cached int: 1;"
            "destructing int;");
        do_traversal(graph, make_controller(empty<bool>()));
        check_log("");
        do_traversal(graph, make_controller(value(true)));
        check_log(
            "visiting cached int: 0;"
            "initializing cached int: 2;");
        do_traversal(graph, make_controller(value(false)));
        check_log(
            "visiting cached int: 1;"
            "destructing int;");
    }
    check_log(
        "destructing int;"
        "destructing int;");
}

TEST_CASE("keyed_data", "[data_graph]")
{
    keyed_data<int> i;
    REQUIRE(!is_valid(i));

    set(i, 2);
    REQUIRE(is_valid(i));
    REQUIRE(get(i) == 2);

    invalidate(i);
    REQUIRE(!is_valid(i));

    mark_valid(i);
    REQUIRE(is_valid(i));
    REQUIRE(get(i) == 2);

    refresh_keyed_data(i, make_id(0));
    REQUIRE(!is_valid(i));

    set(i, 1);
    REQUIRE(is_valid(i));
    REQUIRE(get(i) == 1);

    refresh_keyed_data(i, make_id(0));
    REQUIRE(is_valid(i));

    refresh_keyed_data(i, make_id(1));
    REQUIRE(!is_valid(i));
}

TEST_CASE("signal-based get_keyed_data", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](int i) {
            return [=](context ctx) {
                do_keyed_int(ctx, i);
                do_keyed_int(ctx, 0);
            };
        };
        do_traversal(graph, make_controller(1));
        check_log(
            // A destruction happens during every initialization.
            "destructing int;"
            "initializing keyed int: 1;"
            "destructing int;"
            "initializing keyed int: 0;");
        do_traversal(graph, make_controller(1));
        check_log(
            "visiting keyed int: 1;"
            "visiting keyed int: 0;");
        do_traversal(graph, make_controller(2));
        check_log(
            "destructing int;"
            "initializing keyed int: 2;"
            "visiting keyed int: 0;");
        do_traversal(graph, make_controller(2));
        check_log(
            "visiting keyed int: 2;"
            "visiting keyed int: 0;");
    }
    check_log(
        "destructing int;"
        "destructing int;");
}

TEST_CASE("low-level get_keyed_data", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](int i) {
            return [=](context ctx) {
                int_object* x;
                if (get_keyed_data(ctx, make_id(i), &x))
                {
                    x->n = i;
                    the_log << "initializing keyed int: " << i << ";";
                }
                else
                {
                    REQUIRE(x->n == i);
                    the_log << "visiting keyed int: " << i << ";";
                }
            };
        };
        do_traversal(graph, make_controller(1));
        check_log("initializing keyed int: 1;");
        do_traversal(graph, make_controller(1));
        check_log("visiting keyed int: 1;");
        do_traversal(graph, make_controller(2));
        check_log(
            "destructing int;"
            "initializing keyed int: 2;");
        do_traversal(graph, make_controller(2));
        check_log("visiting keyed int: 2;");
    }
    check_log("destructing int;");
}
