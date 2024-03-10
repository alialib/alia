#include <alia/core/flow/conditionals.hpp>

#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/operators.hpp>

#include <flow/testing.hpp>

using namespace alia;

TEST_CASE("conditional functions", "[flow][macros]")
{
    clear_log();
    {
        data_graph graph;
        auto make_controller = [](auto n) {
            return [=](core_context ctx) {
                if_(ctx, n < 0, [&] {
                    do_int(ctx, -1);
                }).else_if_(n == 0, [&] {
                      do_int(ctx, 0);
                  }).else_([&] { do_int(ctx, 1); });

                if_(ctx, n != 17, [&] { do_int(ctx, 2); });
            };
        };

        do_traversal(graph, make_controller(value(-4)));
        check_log(
            "initializing int: -1;"
            "initializing int: 2;");
        do_traversal(graph, make_controller(value(1)));
        check_log(
            "initializing int: 1;"
            "visiting int: 2;");
        do_traversal(graph, make_controller(value(0)));
        check_log(
            "initializing int: 0;"
            "visiting int: 2;");
        do_traversal(graph, make_controller(value(1)));
        check_log(
            "visiting int: 1;"
            "visiting int: 2;");
        do_traversal(graph, make_controller(empty<int>()));
        check_log("");
    }
    check_log(
        "destructing int: 2;"
        "destructing int: 1;"
        "destructing int: 0;"
        "destructing int: -1;");
}
