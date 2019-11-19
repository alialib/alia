#include <alia/signals/state.hpp>

#include <catch.hpp>

#include <alia/signals/basic.hpp>
#include <alia/signals/operators.hpp>

#include "traversal.hpp"

using namespace alia;

TEST_CASE("state_holder", "[signals]")
{
    state_holder<int> s;
    REQUIRE(!s.is_initialized());
    REQUIRE(s.version() == 0);
    s.set(1);
    REQUIRE(s.is_initialized());
    REQUIRE(s.version() == 1);
    REQUIRE(s.get() == 1);
    s.nonconst_get() = 4;
    REQUIRE(s.is_initialized());
    REQUIRE(s.version() == 2);
    REQUIRE(s.get() == 4);
}

TEST_CASE("basic get_state", "[signals]")
{
    alia::system sys;
    auto make_controller = [&](bool initial_value_ready) {
        return [&](context ctx) {
            auto initial_value = conditional(
                value(initial_value_ready), value(12), empty<int>());

            auto state = get_state(ctx, initial_value);

            if (initial_value_ready)
            {
                REQUIRE(signal_is_readable(state));
                REQUIRE(read_signal(state) == 12);
                REQUIRE(signal_is_writable(state));
                write_signal(state, 13);
                REQUIRE(read_signal(state) == 13);
            }
            else
            {
                REQUIRE(!signal_is_readable(state));
                REQUIRE(signal_is_writable(state));
            }
        };
    };

    do_traversal(sys, make_controller(false));
    do_traversal(sys, make_controller(true));
}

TEST_CASE("writing to uninitialized state", "[signals]")
{
    alia::system sys;
    auto controller = [&](context ctx) {
        auto state = get_state(ctx, empty<int>());

        REQUIRE(!signal_is_readable(state));
        REQUIRE(signal_is_writable(state));
        write_signal(state, 1);
        REQUIRE(signal_is_readable(state));
        REQUIRE(signal_is_writable(state));
        REQUIRE(read_signal(state) == 1);
    };

    do_traversal(sys, controller);
}
