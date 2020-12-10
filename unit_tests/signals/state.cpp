#include <alia/signals/state.hpp>

#include <testing.hpp>

#include <alia/signals/basic.hpp>
#include <alia/signals/operators.hpp>

#include "traversal.hpp"

using namespace alia;

TEST_CASE("state_storage", "[signals][state]")
{
    state_storage<int> s;
    REQUIRE(!s.is_initialized());
    REQUIRE(s.version() == 0);
    unsigned version = 0;

    s.set(1);
    REQUIRE(s.is_initialized());
    REQUIRE(s.version() != 0);
    version = s.version();
    REQUIRE(s.get() == 1);

    s.nonconst_ref() = 4;
    REQUIRE(s.is_initialized());
    REQUIRE(s.version() != version);
    version = s.version();
    REQUIRE(s.get() == 4);

    s.clear();
    REQUIRE(!s.is_initialized());
    REQUIRE(s.version() != version);
    version = s.version();
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
    });
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, value(12));

        if (read_signal(state) != 13)
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
    });
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, empty<int>());

        if (!signal_has_value(state))
            write_signal(state, 1);
    });
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, empty<int>());

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
    });
    do_traversal(sys, [&](context ctx) {
        auto state = get_state(ctx, 12);

        if (read_signal(state) != 13)
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
        scoped_component_container srr(ctx);
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

    // And some benchmarking...
#ifdef NDEBUG
    BENCHMARK("deep state changes")
    {
        refresh_system(sys);

        my_state_change_event e;
        dispatch_event(sys, e);
    };
#endif
}

#ifdef NDEBUG
TEST_CASE("get_state benchmarks", "[signals][state]")
{
    BENCHMARK("get_state")
    {
        alia::system sys;
        initialize_system(sys, [&](context ctx) {
            scoped_component_container srr(ctx);
            // The get_state call is all we really want to benchmark, but
            // somehow, invoking the BENCHMARK macro inside component code
            // causes a stack overflow, so we just repeat it many times so that
            // it dominates the results.
            for (int i = 0; i != 20; ++i)
                get_state(ctx, 12);
        });
        refresh_system(sys);
    };
}
#endif

TEST_CASE("get_transient_state", "[signals][state]")
{
    // The first few passes are all the same as the normal state tests...
    alia::system sys;
    initialize_system(sys, [](context) {});
    do_traversal(sys, [&](context ctx) {
        ALIA_IF(true)
        {
            auto state = get_transient_state(ctx, empty<int>());

            REQUIRE(!signal_has_value(state));
            REQUIRE(signal_ready_to_write(state));
        }
        ALIA_END
    });
    captured_id state_id;
    do_traversal(sys, [&](context ctx) {
        ALIA_IF(true)
        {
            auto state = get_transient_state(ctx, value(12));

            REQUIRE(signal_has_value(state));
            REQUIRE(read_signal(state) == 12);
            REQUIRE(signal_ready_to_write(state));
            state_id.capture(state.value_id());
        }
        ALIA_END
    });
    do_traversal(sys, [&](context ctx) {
        ALIA_IF(true)
        {
            auto state = get_transient_state(ctx, value(12));

            if (read_signal(state) != 13)
                write_signal(state, 13);
        }
        ALIA_END
    });
    do_traversal(sys, [&](context ctx) {
        ALIA_IF(true)
        {
            auto state = get_transient_state(ctx, value(12));

            REQUIRE(read_signal(state) == 13);
            REQUIRE(!state_id.matches(state.value_id()));
            state_id.capture(state.value_id());
        }
        ALIA_END
    });
    // Now test that if the state goes inactivate for a pass, it's reset.
    do_traversal(sys, [&](context ctx) {
        ALIA_IF(false)
        {
            auto state = get_transient_state(ctx, value(12));
        }
        ALIA_END
    });
    do_traversal(sys, [&](context ctx) {
        ALIA_IF(true)
        {
            auto state = get_transient_state(ctx, value(12));

            REQUIRE(signal_has_value(state));
            REQUIRE(read_signal(state) == 12);
            REQUIRE(!state_id.matches(state.value_id()));
            REQUIRE(signal_ready_to_write(state));
        }
        ALIA_END
    });
}
