#include <alia/signals/state.hpp>

#include <testing.hpp>

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
    initialize_system(sys, [](context) {});
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, empty<int>());

        REQUIRE(!signal_has_value(state));
        REQUIRE(signal_ready_to_write(state));
    });
    captured_id state_id;
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, value(12));

        REQUIRE(signal_has_value(state));
        REQUIRE(read_signal(state) == 12);
        REQUIRE(signal_ready_to_write(state));
        state_id.capture(state.value_id());

        write_signal(state, 13);
    });
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, value(12));

        REQUIRE(read_signal(state) == 13);
        REQUIRE(!state_id.matches(state.value_id()));
    });
}

TEST_CASE("writing to uninitialized state", "[signals][state]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, empty<int>());

        REQUIRE(!signal_has_value(state));
        REQUIRE(signal_ready_to_write(state));
        write_signal(state, 1);
        REQUIRE(signal_has_value(state));
        REQUIRE(signal_ready_to_write(state));
        REQUIRE(read_signal(state) == 1);
    });
}

TEST_CASE("get_state with raw initial value", "[signals][state]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});
    captured_id state_id;
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, 12);

        REQUIRE(signal_has_value(state));
        REQUIRE(read_signal(state) == 12);
        REQUIRE(signal_ready_to_write(state));
        state_id.capture(state.value_id());

        write_signal(state, 13);
    });
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, 12);

        REQUIRE(read_signal(state) == 13);
        REQUIRE(!state_id.matches(state.value_id()));
    });
}

struct my_state_change_event
{
};

TEST_CASE("state changes and component dirtying", "[signals][state]")
{
    std::ostringstream log;

    alia::system sys;
    initialize_system(sys, [&](context ctx) {
        scoped_routing_region srr(ctx);
        log << (srr.is_dirty() ? "dirty;" : "clean;");
        auto state = get_state(ctx, 12);
        log << read_signal(state) << ";";
        on_event<my_state_change_event>(ctx, [&](auto, auto&) {
            log << "writing;";
            write_signal(state, 13);
        });
    });

    refresh_system(sys);

    my_state_change_event e;
    dispatch_event(sys, e);

    refresh_system(sys);

    REQUIRE(log.str() == "clean;12;clean;12;writing;dirty;13;clean;13;");
}
