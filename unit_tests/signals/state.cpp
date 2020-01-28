#include <alia/signals/state.hpp>

#include <catch.hpp>

#include <alia/signals/basic.hpp>
#include <alia/signals/operators.hpp>

#include "traversal.hpp"

using namespace alia;

TEST_CASE("state_holder", "[signals][state]")
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

TEST_CASE("basic get_state", "[signals][state]")
{
    alia::system sys;
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, empty<int>());

        REQUIRE(!signal_is_readable(state));
        REQUIRE(signal_is_writable(state));
    });
    captured_id state_id;
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, val(12));

        REQUIRE(signal_is_readable(state));
        REQUIRE(read_signal(state) == 12);
        REQUIRE(signal_is_writable(state));
        state_id.capture(state.value_id());

        write_signal(state, 13);
    });
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, val(12));

        REQUIRE(read_signal(state) == 13);
        REQUIRE(!state_id.matches(state.value_id()));
    });
}

TEST_CASE("writing to uninitialized state", "[signals][state]")
{
    alia::system sys;
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, empty<int>());

        REQUIRE(!signal_is_readable(state));
        REQUIRE(signal_is_writable(state));
        write_signal(state, 1);
        REQUIRE(signal_is_readable(state));
        REQUIRE(signal_is_writable(state));
        REQUIRE(read_signal(state) == 1);
    });
}
