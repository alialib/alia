#include <alia/data_graph.hpp>

#include <sstream>

#include <boost/lexical_cast.hpp>

#include <catch.hpp>

using namespace alia;

// The following define a small framework for testing data traversal mechanics.
// The idea is that we execute various traversals over test data graphs
// (generally multiple times over each graph) and log various events (allocating
// new nodes, destroying old ones, and visiting existing nodes). We can then
// check that the log matches expectations.

static std::stringstream log_;

// Clear the log.
// It's best to do this explicitly at the beginning of each test in case the
// previous one failed and left the log in a bad state.
static void
clear_log()
{
    log_.str(std::string());
}

// Check that the log contains the expected contents and clear it.
static void
check_log(std::string const& expected_contents)
{
    REQUIRE(log_.str() == expected_contents);
    clear_log();
}

struct int_object
{
    int_object() : n(-1)
    {
    }
    int_object(int n) : n(n)
    {
    }
    ~int_object()
    {
        log_ << "destructing int;";
    }
    int n;
};

template<class Context>
void
do_int(Context& ctx, int n)
{
    int_object* obj;
    if (get_data(ctx, &obj))
    {
        REQUIRE(obj->n == -1);
        obj->n = n;
        log_ << "initializing int: " << n << ";";
    }
    else
    {
        REQUIRE(obj->n == n);
        log_ << "visiting int: " << n << ";";
    }
}

template<class Context>
void
do_cached_int(Context& ctx, int n)
{
    int_object* obj;
    if (get_cached_data(ctx, &obj))
    {
        REQUIRE(obj->n == -1);
        obj->n = n;
        log_ << "initializing cached int: " << n << ";";
    }
    else
    {
        REQUIRE(obj->n == n);
        log_ << "visiting cached int: " << n << ";";
    }
}

template<class Context>
void
do_keyed_int(Context& ctx, int n)
{
    keyed_data_signal<int_object> obj;
    if (get_keyed_data(ctx, make_id(n), &obj))
    {
        REQUIRE(!obj.is_readable());
        write_signal(obj, int_object(n * 2));
        log_ << "initializing keyed int: " << n << ";";
    }
    else
    {
        REQUIRE(read_signal(obj).n == n * 2);
        REQUIRE(obj.value_id() == make_id(n));
        log_ << "visiting keyed int: " << n << ";";
    }
}

template<class Controller>
void
do_traversal(
    data_graph& graph, Controller const& controller, bool with_gc = true)
{
    data_traversal ctx;
    scoped_data_traversal sdt(graph, ctx);
    if (!with_gc)
        disable_gc(ctx);
    controller(ctx);
}

// This is used to test that the utilities work with a custom context (rather
// than invoking them directly on a data_traversal).
struct custom_context
{
    custom_context(data_traversal& traversal) : traversal(traversal)
    {
    }

    data_traversal& traversal;
};
static data_traversal&
get_data_traversal(custom_context& ctx)
{
    return ctx.traversal;
}

TEST_CASE("basic data traversal", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto controller = [](data_traversal& ctx) { do_int(ctx, 0); };
        do_traversal(graph, controller);
        check_log("initializing int: 0;");
        do_traversal(graph, controller);
        check_log("visiting int: 0;");
    }
    check_log("destructing int;");
}

TEST_CASE("basic alia_if", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition) {
            return [=](data_traversal& ctx) {
                ALIA_IF(condition)
                {
                    do_int(ctx, 0);
                }
                ALIA_END
                do_int(ctx, 1);
            };
        };
        do_traversal(graph, make_controller(value(false)));
        check_log("initializing int: 1;");
        do_traversal(graph, make_controller(value(true)));
        check_log(
            "initializing int: 0;"
            "visiting int: 1;");
        do_traversal(graph, make_controller(value(false)));
        check_log("visiting int: 1;");
        do_traversal(graph, make_controller(empty<bool>()));
        check_log("visiting int: 1;");
    }
    check_log(
        "destructing int;"
        "destructing int;");
}

TEST_CASE("alia_if/alia_else", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition) {
            return [=](custom_context ctx) {
                ALIA_IF(condition)
                {
                    do_int(ctx, 0);
                }
                ALIA_ELSE
                {
                    do_int(ctx, 1);
                }
                ALIA_END
                do_int(ctx, 2);
            };
        };
        do_traversal(graph, make_controller(value(false)));
        check_log(
            "initializing int: 1;"
            "initializing int: 2;");
        do_traversal(graph, make_controller(value(true)));
        check_log(
            "initializing int: 0;"
            "visiting int: 2;");
        do_traversal(graph, make_controller(value(false)));
        check_log(
            "visiting int: 1;"
            "visiting int: 2;");
        do_traversal(graph, make_controller(empty<bool>()));
        check_log("visiting int: 2;");
    }
    check_log(
        "destructing int;"
        "destructing int;"
        "destructing int;");
}

TEST_CASE("non-signal alia_if/alia_else", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition) {
            return [=](custom_context ctx) {
                ALIA_IF(condition)
                {
                    do_int(ctx, 0);
                }
                ALIA_ELSE
                {
                    do_int(ctx, 1);
                }
                ALIA_END
                do_int(ctx, 2);
            };
        };
        do_traversal(graph, make_controller(false));
        check_log(
            "initializing int: 1;"
            "initializing int: 2;");
        do_traversal(graph, make_controller(true));
        check_log(
            "initializing int: 0;"
            "visiting int: 2;");
        do_traversal(graph, make_controller(false));
        check_log(
            "visiting int: 1;"
            "visiting int: 2;");
    }
    check_log(
        "destructing int;"
        "destructing int;"
        "destructing int;");
}

TEST_CASE("alia_if/alia_else caching", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition) {
            return [=](custom_context ctx) {
                ALIA_IF(condition)
                {
                    ; // This somehow stops ClangFormat from doing weird stuff
                      // with this block;
                    ALIA_IF(value(true))
                    {
                        // This is nested inside an additional level of data
                        // blocks, so it triggers a different case in the cache
                        // clearing code.
                        do_cached_int(ctx, 0);
                    }
                    ALIA_END
                }
                ALIA_ELSE
                {
                    do_cached_int(ctx, 1);
                }
                ALIA_END
                do_int(ctx, 2);
            };
        };
        // Cached data isn't retained inside inactive parts of the traversal, so
        // our cached ints will get destructed and recreated from one traversal
        // to another.
        do_traversal(graph, make_controller(value(false)));
        check_log(
            "initializing cached int: 1;"
            "initializing int: 2;");
        do_traversal(graph, make_controller(value(true)));
        check_log(
            "initializing cached int: 0;"
            "destructing int;"
            "visiting int: 2;");
        do_traversal(graph, make_controller(value(false)));
        check_log(
            "destructing int;"
            "initializing cached int: 1;"
            "visiting int: 2;");
        do_traversal(graph, make_controller(empty<bool>()));
        check_log(
            "destructing int;"
            "visiting int: 2;");
        do_traversal(graph, make_controller(value(true)));
        check_log(
            "initializing cached int: 0;"
            "visiting int: 2;");
    }
    check_log(
        "destructing int;"
        "destructing int;");
}

TEST_CASE("alia_if/alia_else_if/alia_else", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition1, auto condition2) {
            return [=](custom_context ctx) {
                ALIA_IF(condition1)
                {
                    do_int(ctx, 0);
                }
                ALIA_ELSE_IF(condition2)
                {
                    do_int(ctx, 1);
                }
                ALIA_ELSE
                {
                    do_int(ctx, 2);
                }
                ALIA_END
                do_int(ctx, 3);
            };
        };
        do_traversal(graph, make_controller(value(false), value(true)));
        check_log(
            "initializing int: 1;"
            "initializing int: 3;");
        do_traversal(graph, make_controller(value(true), value(false)));
        check_log(
            "initializing int: 0;"
            "visiting int: 3;");
        do_traversal(graph, make_controller(value(true), value(true)));
        check_log(
            "visiting int: 0;"
            "visiting int: 3;");
        do_traversal(graph, make_controller(value(false), value(false)));
        check_log(
            "initializing int: 2;"
            "visiting int: 3;");
        do_traversal(graph, make_controller(empty<bool>(), value(false)));
        check_log("visiting int: 3;");
        do_traversal(graph, make_controller(empty<bool>(), empty<bool>()));
        check_log("visiting int: 3;");
        do_traversal(graph, make_controller(value(false), empty<bool>()));
        check_log("visiting int: 3;");
        do_traversal(graph, make_controller(value(false), value(true)));
        check_log(
            "visiting int: 1;"
            "visiting int: 3;");
    }
    check_log(
        "destructing int;"
        "destructing int;"
        "destructing int;"
        "destructing int;");
}

TEST_CASE("alia_pass_dependent_if", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition) {
            return [=](data_traversal& ctx) {
                ALIA_PASS_DEPENDENT_IF(condition)
                {
                    do_cached_int(ctx, 0);
                }
                ALIA_END
                do_int(ctx, 1);
            };
        };
        do_traversal(graph, make_controller(value(false)));
        check_log("initializing int: 1;");
        do_traversal(graph, make_controller(value(true)));
        check_log(
            "initializing cached int: 0;"
            "visiting int: 1;");
        do_traversal(graph, make_controller(value(false)));
        check_log("visiting int: 1;");
        do_traversal(graph, make_controller(empty<bool>()));
        check_log("visiting int: 1;");
        do_traversal(graph, make_controller(value(true)));
        check_log(
            "visiting cached int: 0;"
            "visiting int: 1;");
    }
    check_log(
        "destructing int;"
        "destructing int;");
}

TEST_CASE("alia_switch", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto n) {
            return [=](custom_context ctx) {
                // clang-format off
                ALIA_SWITCH(n)
                {
                    ALIA_CASE(0):
                        do_int(ctx, 0);
                        break;
                    ALIA_CASE(1):
                        do_int(ctx, 1);
                    ALIA_CASE(2):
                    ALIA_CASE(3):
                        do_int(ctx, 2);
                        do_cached_int(ctx, 3);
                        break;
                    ALIA_DEFAULT:
                        do_int(ctx, 4);
                }
                ALIA_END
                do_int(ctx, -2);
                // clang-format on
            };
        };
        do_traversal(graph, make_controller(value(0)));
        check_log(
            "initializing int: 0;"
            "initializing int: -2;");
        do_traversal(graph, make_controller(value(2)));
        check_log(
            "initializing int: 2;"
            "initializing cached int: 3;"
            "visiting int: -2;");
        do_traversal(graph, make_controller(value(1)));
        check_log(
            "initializing int: 1;"
            "visiting int: 2;"
            "visiting cached int: 3;"
            "visiting int: -2;");
        do_traversal(graph, make_controller(value(17)));
        check_log(
            "initializing int: 4;"
            "visiting int: -2;"
            "destructing int;");
        do_traversal(graph, make_controller(value(1)));
        check_log(
            "visiting int: 1;"
            "visiting int: 2;"
            "initializing cached int: 3;"
            "visiting int: -2;");
        do_traversal(graph, make_controller(value(2)));
        check_log(
            "visiting int: 2;"
            "visiting cached int: 3;"
            "visiting int: -2;");
    }
    check_log(
        "destructing int;"
        "destructing int;"
        "destructing int;"
        "destructing int;"
        "destructing int;"
        "destructing int;");
}

TEST_CASE("non-signal alia_switch", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto n) {
            return [=](custom_context ctx) {
                // clang-format off
                ALIA_SWITCH(n)
                {
                    ALIA_CASE(0):
                        do_int(ctx, 0);
                        break;
                    ALIA_CASE(1):
                        do_int(ctx, 1);
                    ALIA_CASE(2):
                    ALIA_CASE(3):
                        do_int(ctx, 2);
                        do_cached_int(ctx, 3);
                        break;
                    ALIA_DEFAULT:
                        do_int(ctx, 4);
                }
                ALIA_END
                do_int(ctx, -2);
                // clang-format on
            };
        };
        do_traversal(graph, make_controller(0));
        check_log(
            "initializing int: 0;"
            "initializing int: -2;");
        do_traversal(graph, make_controller(2));
        check_log(
            "initializing int: 2;"
            "initializing cached int: 3;"
            "visiting int: -2;");
        do_traversal(graph, make_controller(1));
        check_log(
            "initializing int: 1;"
            "visiting int: 2;"
            "visiting cached int: 3;"
            "visiting int: -2;");
        do_traversal(graph, make_controller(17));
        check_log(
            "initializing int: 4;"
            "visiting int: -2;"
            "destructing int;");
        do_traversal(graph, make_controller(1));
        check_log(
            "visiting int: 1;"
            "visiting int: 2;"
            "initializing cached int: 3;"
            "visiting int: -2;");
        do_traversal(graph, make_controller(2));
        check_log(
            "visiting int: 2;"
            "visiting cached int: 3;"
            "visiting int: -2;");
    }
    check_log(
        "destructing int;"
        "destructing int;"
        "destructing int;"
        "destructing int;"
        "destructing int;"
        "destructing int;");
}

TEST_CASE("alia_for", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](int n) {
            return [=](custom_context ctx) {
                ALIA_FOR(int i = 1; i <= n; ++i)
                {
                    do_int(ctx, i);
                }
                ALIA_END
                do_int(ctx, 0);
            };
        };
        do_traversal(graph, make_controller(2));
        check_log(
            "initializing int: 1;"
            "initializing int: 2;"
            "initializing int: 0;");
        do_traversal(graph, make_controller(1));
        check_log(
            "visiting int: 1;"
            "destructing int;"
            "visiting int: 0;");
        do_traversal(graph, make_controller(4));
        check_log(
            "visiting int: 1;"
            "initializing int: 2;"
            "initializing int: 3;"
            "initializing int: 4;"
            "visiting int: 0;");
        do_traversal(graph, make_controller(0));
        check_log(
            "destructing int;"
            "destructing int;"
            "destructing int;"
            "destructing int;"
            "visiting int: 0;");
        do_traversal(graph, make_controller(3));
        check_log(
            "initializing int: 1;"
            "initializing int: 2;"
            "initializing int: 3;"
            "visiting int: 0;");
    }
    check_log(
        "destructing int;"
        "destructing int;"
        "destructing int;"
        "destructing int;");
}

TEST_CASE("alia_while", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](int n) {
            return [=](custom_context ctx) {
                int i = 1;
                ALIA_WHILE(i <= n)
                {
                    do_int(ctx, i);
                    ++i;
                }
                ALIA_END
                do_int(ctx, 0);
            };
        };
        do_traversal(graph, make_controller(2));
        check_log(
            "initializing int: 1;"
            "initializing int: 2;"
            "initializing int: 0;");
        do_traversal(graph, make_controller(1));
        check_log(
            "visiting int: 1;"
            "destructing int;"
            "visiting int: 0;");
        do_traversal(graph, make_controller(4));
        check_log(
            "visiting int: 1;"
            "initializing int: 2;"
            "initializing int: 3;"
            "initializing int: 4;"
            "visiting int: 0;");
        do_traversal(graph, make_controller(0));
        check_log(
            "destructing int;"
            "destructing int;"
            "destructing int;"
            "destructing int;"
            "visiting int: 0;");
        do_traversal(graph, make_controller(3));
        check_log(
            "initializing int: 1;"
            "initializing int: 2;"
            "initializing int: 3;"
            "visiting int: 0;");
    }
    check_log(
        "destructing int;"
        "destructing int;"
        "destructing int;"
        "destructing int;");
}

TEST_CASE("named blocks", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](std::vector<int> indices) {
            return [=](data_traversal& ctx) {
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

TEST_CASE("mobile named blocks", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](std::vector<int> indices, int divider) {
            return [=](data_traversal& ctx) {
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
                ALIA_FOR(auto i : indices)
                {
                    ;
                    ALIA_IF(i >= divider)
                    {
                        named_block nb(nc, make_id(i));
                        do_int(ctx, i);
                    }
                    ALIA_END
                }
                ALIA_END
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

TEST_CASE("multiple naming contexts", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](std::vector<int> indices) {
            return [=](custom_context ctx) {
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
            return [=](custom_context ctx) {
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
            return [=](custom_context ctx) {
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
            return [=](custom_context ctx) {
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

TEST_CASE("scoped_cache_clearing_disabler", "[data_graph]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition) {
            return [=](custom_context ctx) {
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
            return [=](custom_context ctx) {
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
            return [=](custom_context ctx) {
                int_object* x;
                if (get_keyed_data(ctx, make_id(i), &x))
                {
                    x->n = i;
                    log_ << "initializing keyed int: " << i << ";";
                }
                else
                {
                    REQUIRE(x->n == i);
                    log_ << "visiting keyed int: " << i << ";";
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
