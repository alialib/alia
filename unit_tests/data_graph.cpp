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

// Check that the log contains the expected contents and clear it.
static void
check_log(std::string const& expected_contents)
{
    REQUIRE(log_.str() == expected_contents);
    log_.str(std::string());
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

void
do_int(data_traversal& ctx, int n)
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

void
do_cached_int(data_traversal& ctx, int n)
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

void
do_keyed_int(data_traversal& ctx, int n)
{
    keyed_data_signal<int_object> obj;
    if (get_keyed_data(ctx, make_id(n), &obj))
    {
        REQUIRE(!obj.is_readable());
        write_signal(obj, int_object(n * 2));
    }
    else
        REQUIRE(read_signal(obj).n == n * 2);
}

template<class Controller>
void
do_traversal(data_graph& graph, Controller const& controller)
{
    data_traversal ctx;
    scoped_data_traversal sdt(graph, ctx);
    controller(ctx);
}

TEST_CASE("basic data traversal", "[data_graph]")
{
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
    {
        data_graph graph;
        auto make_controller = [](auto condition) {
            return [=](data_traversal& ctx) {
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

TEST_CASE("alia_if/alia_else caching", "[data_graph]")
{
    {
        data_graph graph;
        auto make_controller = [](auto condition) {
            return [=](data_traversal& ctx) {
                ALIA_IF(condition)
                {
                    do_cached_int(ctx, 0);
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
    {
        data_graph graph;
        auto make_controller = [](auto condition1, auto condition2) {
            return [=](data_traversal& ctx) {
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

TEST_CASE("alia_switch", "[data_graph]")
{
    {
        data_graph graph;
        auto make_controller = [](auto n) {
            return [=](data_traversal& ctx) {
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

// void
// do_traversal(
//     data_graph& graph,
//     int n,
//     int a,
//     int b,
//     int c,
//     int d,
//     int backwards,
//     int reentrant)
// {
//     // clang-format off
//     using namespace alia;
//     data_traversal ctx;
//     scoped_data_traversal sdt(graph, ctx);
//     do_int(ctx, 0);
//     do_int(ctx, -2);
//     alia_if(reentrant)
//     {
//         do_traversal(graph, n, a, b, c, d, backwards, 0);
//     }
//     alia_end
//     alia_if(b || c || d)
//     {
//         do_string(ctx, "x");
//         alia_if(b)
//         {
//             do_string(ctx, "y");
//             naming_context nc(ctx);
//             if (backwards)
//             {
//                 for (int i = n + 3; i >= n; --i)
//                 {
//                     named_block nb(nc, make_id(i));
//                     do_int(ctx, i);
//                     do_string(ctx, "p");
//                 }
//             }
//             else
//             {
//                 for (int i = n; i < n + 4; ++i)
//                 {
//                     named_block nb(nc, make_id(i));
//                     do_int(ctx, i);
//                     do_string(ctx, "p");
//                 }
//             }
//             do_string(ctx, "z");
//         }
//         alia_end
//         alia_if(c)
//         {
//             alia_for(int i = 0; i < n; ++i)
//             {
//                 do_int(ctx, i);
//                 do_string(ctx, "q");
//             }
//             alia_end
//         }
//         alia_end
//         do_int(ctx, 6);
//         alia_if(d)
//         {
//             naming_context nc(ctx);
//             do_int(ctx, 2);
//             if (backwards)
//             {
//                 for (int i = n + 3; i >= n; --i)
//                 {
//                     named_block nb(nc, make_id(i));
//                     do_int(ctx, i - 1);
//                 }
//             }
//             else
//             {
//                 for (int i = n; i < n + 4; ++i)
//                 {
//                     named_block nb(nc, make_id(i));
//                     do_int(ctx, i - 1);
//                 }
//             }
//             do_string(ctx, "a");
//         }
//         alia_end
//         do_int(ctx, 0);
//         do_string(ctx, "z");
//     }
//     alia_else_if(a)
//     {
//         do_string(ctx, "alia");
//         alia_if(reentrant)
//         {
//             do_traversal(graph, n, a, b, c, d, backwards, 0);
//         }
//         alia_end
//         naming_context nc(ctx);
//         if (backwards)
//         {
//             for (int i = n + 103; i >= n + 100; --i)
//             {
//                 named_block nb(nc, make_id(i), manual_delete(true));
//                 do_int(ctx, i - 1);
//             }
//         }
//         else
//         {
//             for (int i = n + 100; i < n + 104; ++i)
//             {
//                 named_block nb(nc, make_id(i), manual_delete(true));
//                 do_int(ctx, i - 1);
//             }
//         }
//         do_int(ctx, 42);
//     }
//     alia_else
//     {
//         do_int(ctx, 0);
//         alia_if(n < 0)
//         {
//             int i = 0;
//             alia_while(i-- > n)
//             {
//                 do_int(ctx, i);
//             }
//             alia_end
//         }
//         alia_else
//         {
//             alia_if(reentrant)
//             {
//                 do_traversal(graph, n, a, b, c, d, backwards, 0);
//             }
//             alia_end
//             alia_switch(n)
//             {
//                 alia_case(0):
//                     do_int(ctx, 0);
//                     break;
//                 alia_case(1):
//                     do_int(ctx, 0);
//                 alia_case(2):
//                 alia_case(3):
//                     do_int(ctx, 2);
//                     break;
//                 alia_default:
//                     do_int(ctx, 3);
//             }
//             alia_end
//         }
//         alia_end
//     }
//     alia_end
//     do_int(ctx, -1);
// // clang-format on
// }

// #define check_inits(ic, sc)                                                    \
//     REQUIRE(n_int_inits == ic);                                                \
//     REQUIRE(n_string_inits == sc);

// TEST_CASE("low_level_test", "[data_graph]")
// {
//     n_int_constructs = 0;
//     n_int_destructs = 0;
//     n_int_inits = 0;
//     n_string_constructs = 0;
//     n_string_destructs = 0;
//     n_string_inits = 0;

//     {
//         data_graph graph;

//         int ic = 0, sc = 0;

//         // different cases in the switch statement
//         do_traversal(graph, 0, 0, 0, 0, 0, 0, 0);
//         ic += 5;
//         check_inits(ic, sc);
//         do_traversal(graph, 3, 0, 0, 0, 0, 0, 0);
//         ic += 1;
//         check_inits(ic, sc);
//         do_traversal(graph, 2, 0, 0, 0, 0, 0, 0);
//         check_inits(ic, sc);
//         do_traversal(graph, 6, 0, 0, 0, 0, 0, 0);
//         ic += 1;
//         check_inits(ic, sc);
//         do_traversal(graph, 4, 0, 0, 0, 0, 0, 0);
//         check_inits(ic, sc);
//         do_traversal(graph, 1, 0, 0, 0, 0, 0, 0);
//         ic += 1;
//         check_inits(ic, sc);
//         do_traversal(graph, 0, 0, 0, 0, 0, 0, 0);
//         check_inits(ic, sc);

//         // different numbers of iterations in the while loop
//         do_traversal(graph, -3, 0, 0, 0, 0, 0, 0);
//         ic += 3;
//         check_inits(ic, sc);
//         do_traversal(graph, -2, 0, 0, 0, 0, 0, 0);
//         check_inits(ic, sc);
//         do_traversal(graph, -6, 0, 0, 0, 0, 0, 0);
//         ic += 3;
//         check_inits(ic, sc);
//         do_traversal(graph, -1, 0, 0, 0, 0, 0, 0);
//         check_inits(ic, sc);
//         do_traversal(graph, -7, 0, 0, 0, 0, 0, 0);
//         ic += 1;
//         check_inits(ic, sc);
//         do_traversal(graph, -3, 0, 0, 0, 0, 0, 0);
//         check_inits(ic, sc);
//         do_traversal(graph, -1, 0, 0, 0, 0, 0, 0);
//         check_inits(ic, sc);
//         // and one more case
//         do_traversal(graph, 0, 0, 0, 0, 0, 0, 0);
//         check_inits(ic, sc);

//         do_traversal(graph, 0, 1, 0, 0, 0, 0, 0);
//         ic += 5;
//         sc += 1;
//         check_inits(ic, sc);

//         // varying numbers of iterations in the for loop
//         do_traversal(graph, 0, 0, 0, 1, 0, 0, 0);
//         ic += 2;
//         sc += 2;
//         check_inits(ic, sc);
//         do_traversal(graph, 3, 0, 0, 1, 0, 0, 0);
//         ic += 3;
//         sc += 3;
//         check_inits(ic, sc);
//         do_traversal(graph, 12, 0, 0, 1, 0, 0, 0);
//         ic += 9;
//         sc += 9;
//         check_inits(ic, sc);
//         do_traversal(graph, 6, 0, 0, 1, 0, 0, 0);
//         check_inits(ic, sc);
//         do_traversal(graph, 12, 0, 0, 1, 0, 0, 0);
//         check_inits(ic, sc);

//         do_traversal(graph, 0, 0, 1, 1, 1, 0, 0);
//         ic += 9;
//         sc += 7;
//         check_inits(ic, sc);
//         // At this point, all branches have been followed, so all future
//         // initializations are from named blocks...

//         // Test that named blocks aren't deleted if they're inside unexecuted
//         // branches.
//         do_traversal(graph, 0, 0, 0, 0, 0, 0, 0);
//         check_inits(ic, sc);
//         do_traversal(graph, 0, 0, 1, 1, 1, 0, 0);
//         check_inits(ic, sc);

//         // different sets of named blocks, some in reverse order
//         do_traversal(graph, 1, 0, 0, 0, 1, 0, 0);
//         ic += 1;
//         check_inits(ic, sc);
//         do_traversal(graph, 6, 0, 0, 0, 1, 0, 0);
//         ic += 4;
//         check_inits(ic, sc);
//         do_traversal(graph, 7, 0, 0, 0, 1, 1, 0);
//         ic += 1;
//         check_inits(ic, sc);
//         do_traversal(graph, 1, 0, 0, 0, 1, 0, 0);
//         ic += 4;
//         check_inits(ic, sc);
//         do_traversal(graph, 1, 0, 0, 0, 1, 1, 0);
//         check_inits(ic, sc);
//         do_traversal(graph, 1, 0, 0, 0, 1, 0, 0);
//         check_inits(ic, sc);

//         // Test that blocks with the MANUAL_DELETE flag aren't deleted if
//         // they're not seen.
//         do_traversal(graph, 1, 1, 0, 0, 0, 0, 0);
//         ic += 1;
//         check_inits(ic, sc);
//         do_traversal(graph, 0, 1, 0, 0, 0, 0, 0);
//         check_inits(ic, sc);
//         do_traversal(graph, 6, 1, 0, 0, 0, 0, 0);
//         ic += 4;
//         check_inits(ic, sc);
//         do_traversal(graph, 1, 1, 0, 0, 0, 0, 0);
//         do_traversal(graph, 0, 1, 0, 0, 0, 0, 0);
//         check_inits(ic, sc);
//         do_traversal(graph, 1, 1, 0, 0, 0, 1, 0);
//         check_inits(ic, sc);

//         // Deleting has no effect if the named block is still being invoked.
//         delete_named_block(graph, make_id(101));
//         do_traversal(graph, 1, 1, 0, 0, 0, 0, 0);
//         check_inits(ic, sc);

//         // Now test that manual deletion of blocks actually works.
//         delete_named_block(graph, make_id(100));
//         do_traversal(graph, 0, 1, 0, 0, 0, 0, 0);
//         ic += 1;
//         check_inits(ic, sc);
//         delete_named_block(graph, make_id(100));
//         do_traversal(graph, 6, 1, 0, 0, 0, 0, 0);
//         do_traversal(graph, 0, 1, 0, 0, 0, 0, 0);
//         ic += 2;
//         check_inits(ic, sc);
//         delete_named_block(graph, make_id(106));
//         delete_named_block(graph, make_id(107));
//         do_traversal(graph, 6, 1, 0, 0, 0, 0, 0);
//         ic += 2;
//         check_inits(ic, sc);

//         // Test that recursive traversals don't cause any leaks.
//         // (There was a bug where it caused leaks with named blocks.)
//         do_traversal(graph, 6, 0, 0, 0, 0, 0, 1);
//         do_traversal(graph, 0, 1, 0, 0, 0, 0, 1);
//         do_traversal(graph, 1, 0, 0, 0, 0, 0, 1);
//     }

//     REQUIRE(n_int_inits == n_int_constructs);
//     REQUIRE(n_int_inits == n_int_destructs);

//     REQUIRE(n_string_inits == n_string_constructs);
//     REQUIRE(n_string_inits == n_string_destructs);
// }

// The following tests that the low-level mechanics of cached data work.

// void
// do_traversal(data_graph& graph, int n, int a, int b)
// {
//     // clang-format off
//     data_traversal ctx;
//     scoped_data_traversal sdt(graph, ctx);
//     do_cached_int(ctx, 0);
//     do_int(ctx, -2);
//     alia_if(a)
//     {
//         alia_for(int i = 0; i < n; ++i)
//         {
//             do_cached_int(ctx, i);
//         }
//         alia_end do_int(ctx, 0);
//     }
//     alia_else_if(b)
//     {
//         naming_context nc(ctx);
//         for (int i = n + 103; i >= n + 100; --i)
//         {
//             named_block nb(nc, make_id(i));
//             do_int(ctx, 1 - i);
//             do_cached_int(ctx, i - 1);
//         }
//         do_cached_int(ctx, 1);
//     }
//     alia_end
//     alia_switch(a)
//     {
//         alia_case(0):
//             do_cached_int(ctx, 0);
//             break;
//         alia_case(1):
//             do_cached_int(ctx, 1);
//             break;
//     }
//     alia_end
//     do_cached_int(ctx, -1);
//     do_keyed_int(ctx, a + b);
//     // clang-format on
// }

// TEST_CASE("cached_data_test", "[data_graph]")
// {
//     n_int_constructs = 0;
//     n_int_destructs = 0;
//     n_int_inits = 0;
//     n_string_constructs = 0;
//     n_string_destructs = 0;
//     n_string_inits = 0;

//     {
//         data_graph graph;

//         int ic = 0, sc = 0;

//         do_traversal(graph, 0, 0, 0);
//         ic += 5;
//         check_inits(ic, sc);
//         do_traversal(graph, 3, 1, 0);
//         ic += 6;
//         check_inits(ic, sc);
//         do_traversal(graph, 2, 0, 1);
//         ic += 10;
//         check_inits(ic, sc);
//         do_traversal(graph, 3, 1, 0);
//         ic += 4;
//         check_inits(ic, sc);
//         do_traversal(graph, 2, 0, 1);
//         ic += 6;
//         check_inits(ic, sc);
//     }

//     REQUIRE(n_int_constructs == n_int_destructs);

//     REQUIRE(n_string_inits == n_string_constructs);
//     REQUIRE(n_string_inits == n_string_destructs);
// }
