#include <alia/core/signals/adaptors.hpp>

#include <map>
#include <type_traits>

#include <alia/core/actions/operators.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/lambdas.hpp>
#include <alia/core/signals/operators.hpp>
#include <alia/core/signals/state.hpp>

#include <flow/testing.hpp>
#include <move_testing.hpp>
#include <traversal.hpp>

using namespace alia;

TEST_CASE("fake_readability", "[signals][adaptors]")
{
    {
        auto s = fake_readability(
            lambda_reader([&]() { return true; }, [&]() { return 0; }));

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(!signal_is_writable<signal_t>::value);

        REQUIRE(!signal_has_value(s));
    }

    {
        int x = 0;

        auto s = fake_readability(lambda_duplex(
            [&]() { return true; },
            [&]() { return x; },
            [&]() { return true; },
            [&](int v) { x = v; }));

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);

        REQUIRE(s.value_id() == null_id);
        REQUIRE(!signal_has_value(s));
        REQUIRE(signal_ready_to_write(s));
        write_signal(s, 1);
        REQUIRE(x == 1);
    }
}

TEST_CASE("fake_writability", "[signals][adaptors]")
{
    {
        auto s = fake_writability(
            lambda_reader([&]() { return true; }, [&]() { return 0; }));

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(!signal_ready_to_write(s));
    }

    {
        auto s = fake_writability(lambda_duplex(
            [&]() { return true; },
            [&]() { return 0; },
            [&]() { return true; },
            [&](int) {}));

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 0);
        int x = 0;
        REQUIRE(s.value_id() == make_id_by_reference(x));
        REQUIRE(!signal_ready_to_write(s));
    }
}

TEST_CASE("minimize_id_changes", "[signals][adaptors]")
{
    alia::test_system sys;
    initialize_test_system(sys, [](core_context) {});

    std::map<int, std::string> container;

    auto make_controller = [&](auto&& test_code) {
        return [&](core_context ctx) {
            auto unwrapped = direct(container)[value(2)];
            auto signal = minimize_id_changes(ctx, unwrapped);

            // Test various properties of the signal that should always be
            // true.
            typedef decltype(signal) signal_t;
            REQUIRE((std::is_same<signal_t::value_type, std::string>::value));
            REQUIRE(signal_is_readable<signal_t>::value);
            REQUIRE(signal_is_writable<signal_t>::value);
            REQUIRE(signal_has_value(signal));
            REQUIRE(signal_ready_to_write(signal));

            // Do custom tests for this pass.
            test_code(signal);
        };
    };

    captured_id signal_id;

    // Set an initial container value and check that our signal works.
    container = {{2, "a"}, {0, "b"}};
    do_traversal(sys, make_controller([&](auto signal) {
                     signal_id.capture(signal.value_id());
                     REQUIRE(read_signal(signal) == "a");
                 }));

    // If we update the outer container but don't touch the entry for the
    // signal that we're looking at, it shouldn't change the ID.
    container = {{2, "a"}, {0, "c"}};
    do_traversal(sys, make_controller([&](auto signal) {
                     REQUIRE(signal_id.matches(signal.value_id()));
                     REQUIRE(read_signal(signal) == "a");
                 }));

    // If we update the outer container but DO touch the entry for the
    // signal that we're looking at, it SHOULD change the ID.
    container = {{2, "b"}, {0, "c"}};
    do_traversal(sys, make_controller([&](auto signal) {
                     REQUIRE(!signal_id.matches(signal.value_id()));
                     signal_id.capture(signal.value_id());
                     REQUIRE(read_signal(signal) == "b");
                 }));

    // Test that we can successfully back through our signal.
    do_traversal(
        sys, make_controller([&](auto signal) { write_signal(signal, "d"); }));
    REQUIRE(container == (std::map<int, std::string>{{2, "d"}, {0, "c"}}));

    // Test that we observe our writes on the next pass.
    do_traversal(sys, make_controller([&](auto signal) {
                     REQUIRE(!signal_id.matches(signal.value_id()));
                     signal_id.capture(signal.value_id());
                     REQUIRE(read_signal(signal) == "d");
                 }));
}

TEST_CASE("signalize a signal", "[signals][adaptors]")
{
    int x = 12;
    auto s = direct(x);
    auto t = signalize(s);
    REQUIRE(signal_has_value(t));
    REQUIRE(read_signal(t) == 12);
}

TEST_CASE("signalize a value", "[signals][adaptors]")
{
    int x = 12;
    auto t = signalize(x);
    REQUIRE(signal_has_value(t));
    REQUIRE(read_signal(t) == 12);
}

TEST_CASE("signal value movement", "[signals][adaptors]")
{
    // Test that copy counting work.
    REQUIRE(copy_count == 0);
    movable_object m = 2;
    movable_object n = m;
    REQUIRE(copy_count == 1);

    // Test that updating a state value via an action would normally involve
    // copying.
    copy_count = 0;
    state_storage<movable_object> state;
    state.set(std::move(n));
    auto state_signal = make_state_signal(state);
    REQUIRE(copy_count == 0);
    movable_object x(4);
    perform_action(state_signal <<= direct(x));
    REQUIRE(copy_count == 1);
    REQUIRE(state.get().n == 4);

    // Test that the use of move() eliminates the copies.
    copy_count = 0;
    perform_action(state_signal <<= move(direct(x)));
    REQUIRE(copy_count == 0);
    REQUIRE(state.get().n == 4);
}
