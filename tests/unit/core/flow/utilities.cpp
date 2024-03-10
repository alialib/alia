#define ALIA_LOWERCASE_MACROS

#include <alia/core/flow/utilities.hpp>

#include <alia/core/signals/basic.hpp>

#include <catch2/catch_test_macros.hpp>

#include "traversal.hpp"

using namespace alia;

using std::string;

TEST_CASE("make_returnable_ref", "[flow][for_each]")
{
    alia::test_system sys;
    initialize_test_system(sys, [](core_context) {});

    auto function_that_returns = [](core_context ctx) -> readable<string> {
        return make_returnable_ref(ctx, value(string("something")));
    };

    auto controller = [&](core_context ctx) {
        auto s = function_that_returns(ctx);
        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == "something");
    };

    do_traversal(sys, controller);
}