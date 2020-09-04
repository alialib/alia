#define ALIA_LOWERCASE_MACROS
#include <alia/flow/macros.hpp>

#include <alia/signals/basic.hpp>

#include <flow/testing.hpp>

TEST_CASE("condition_is_true/false/etc", "[flow][macros]")
{
    REQUIRE(condition_is_true(value(true)));
    REQUIRE(!condition_is_true(value(false)));
    REQUIRE(!condition_is_true(empty<bool>()));

    REQUIRE(condition_is_false(value(false)));
    REQUIRE(!condition_is_false(value(true)));
    REQUIRE(!condition_is_false(empty<bool>()));
}

TEST_CASE("basic alia_if", "[flow][macros]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition) {
            return [=](context ctx) {
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

TEST_CASE("alia_if/alia_else", "[flow][macros]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition) {
            return [=](context ctx) {
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

TEST_CASE("non-signal alia_if/alia_else", "[flow][macros]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition) {
            return [=](context ctx) {
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

TEST_CASE("alia_if/alia_else caching", "[flow][macros]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition) {
            return [=](context ctx) {
                ALIA_IF(condition)
                {
                    ALIA_IF(value(true))
                    {
                        // This is nested inside an additional level of
                        // data blocks, so it triggers a different case
                        // in the cache clearing code.
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
        // Cached data isn't retained inside inactive parts of the traversal,
        // so our cached ints will get destructed and recreated from one
        // traversal to another.
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

TEST_CASE("alia_if/alia_else_if/alia_else", "[flow][macros]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto condition1, auto condition2) {
            return [=](context ctx) {
                alia_if(condition1)
                {
                    do_int(ctx, 0);
                }
                alia_else_if(condition2)
                {
                    do_int(ctx, 1);
                }
                alia_else
                {
                    do_int(ctx, 2);
                }
                alia_end

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

#ifdef __clang__
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#elif __GNUC__ >= 7
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

TEST_CASE("alia_switch", "[flow][macros]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto n) {
            return [=](context ctx) {
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

TEST_CASE("non-signal alia_switch", "[flow][macros]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto n) {
            return [=](context ctx) {
                // clang-format off
                alia_switch(n)
                {
                    alia_case(0):
                        do_int(ctx, 0);
                        break;
                    alia_case(1):
                        do_int(ctx, 1);
                    alia_case(2):
                    alia_case(3):
                        do_int(ctx, 2);
                        do_cached_int(ctx, 3);
                        break;
                    alia_default:
                        do_int(ctx, 4);
                }
                alia_end
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

TEST_CASE("alia_for", "[flow][macros]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](int n) {
            return [=](context ctx) {
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

TEST_CASE("alia_while", "[flow][macros]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](int n) {
            return [=](context ctx) {
                int i = 1;
                alia_while(i <= n)
                {
                    do_int(ctx, i);
                    ++i;
                }
                alia_end;
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

TEST_CASE("alia_untracked_if", "[flow][macros]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](int n) {
            return [=](context ctx) {
                REQUIRE(ctx.has<data_traversal_tag>());
                alia_untracked_if(n > 2)
                {
                    REQUIRE(!ctx.has<data_traversal_tag>());
                }
                alia_untracked_else_if(n > 1)
                {
                    REQUIRE(!ctx.has<data_traversal_tag>());
                }
                alia_untracked_else
                {
                    REQUIRE(!ctx.has<data_traversal_tag>());
                }
                alia_end;
                do_int(ctx, 0);
            };
        };
        do_traversal(graph, make_controller(1));
        check_log("initializing int: 0;");
        do_traversal(graph, make_controller(2));
        do_traversal(graph, make_controller(3));
    }
}

TEST_CASE("alia_untracked_switch", "[flow][macros]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](int n) {
            return [=](context ctx) {
                REQUIRE(ctx.has<data_traversal_tag>());

                auto f = [](context ctx, int x) {
                    REQUIRE(ctx.has<data_traversal_tag>());
                    return x;
                };

                // clang-format off
                ALIA_UNTRACKED_SWITCH(f(ctx, n))
                {
                    case 0:
                        REQUIRE(!ctx.has<data_traversal_tag>());
                        break;
                    default:
                        REQUIRE(!ctx.has<data_traversal_tag>());
                        break;
                }
                ALIA_END
                // clang-format on

                do_int(ctx, 0);
            };
        };
        do_traversal(graph, make_controller(1));
        check_log("initializing int: 0;");
        do_traversal(graph, make_controller(0));
    }
}

TEST_CASE("alia_event_dependent_if", "[flow][macros]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](int n) {
            return [=](context ctx) {
                ALIA_IF(n > 1)
                {
                    do_cached_int(ctx, 3);
                }
                ALIA_END

                ALIA_EVENT_DEPENDENT_IF(n > 1)
                {
                    do_cached_int(ctx, 2);
                }
                ALIA_END

                do_cached_int(ctx, 1);
            };
        };
        do_traversal(graph, make_controller(4));
        check_log(
            "initializing cached int: 3;"
            "initializing cached int: 2;"
            "initializing cached int: 1;");
        do_traversal(graph, make_controller(0));
        check_log(
            "destructing int;"
            "visiting cached int: 1;");
        do_traversal(graph, make_controller(4));
        check_log(
            "initializing cached int: 3;"
            "visiting cached int: 2;"
            "visiting cached int: 1;");
    }
}
