#include <alia/core/signals/adaptors.hpp>

#include <map>
#include <optional>
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

TEST_CASE("signal_cast", "[signals][adaptors]")
{
    int x = 1;
    auto s = signal_cast<double>(direct(x));

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, double>::value));
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 1.0);
    REQUIRE(signal_ready_to_write(s));
    write_signal(s, 0.0);
    REQUIRE(x == 0);
}

TEST_CASE("has_value", "[signals][adaptors]")
{
    bool hv = false;
    int x = 1;

    {
        auto s = has_value(
            lambda_reader([&]() { return hv; }, [&]() { return x; }));

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(!signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == false);
    }

    hv = true;

    {
        // Recreate the signal to circumvent internal caching.
        auto s = has_value(
            lambda_reader([&]() { return hv; }, [&]() { return x; }));

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == true);
    }
}

TEST_CASE("ready_to_write", "[signals][adaptors]")
{
    bool ready = false;
    int x = 1;

    {
        auto s = ready_to_write(lambda_duplex(
            always_has_value,
            [&]() { return x; },
            [&]() { return ready; },
            [&](int v) { x = v; }));

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(!signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == false);
    }

    ready = true;

    {
        // Recreate the signal to circumvent internal caching.
        auto s = ready_to_write(lambda_duplex(
            always_has_value,
            [&]() { return x; },
            [&]() { return ready; },
            [&](int v) { x = v; }));

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == true);
    }
}

TEST_CASE("add_default", "[signals][adaptors]")
{
    int p = 1;
    int f = 0;
    auto make_default = [&](bool primary_has_value,
                            bool primary_ready_to_write,
                            bool default_has_value) {
        return add_default(
            lambda_duplex(
                [=]() { return primary_has_value; },
                [=]() { return p; },
                [=]() { return primary_ready_to_write; },
                [&p](int x) { p = x; }),
            lambda_duplex(
                [=]() { return default_has_value; },
                [=]() { return f; },
                always_ready,
                [&f](int x) { f = x; }));
    };

    {
        typedef decltype(make_default(true, true, true)) signal_t;
        REQUIRE(signal_is_move_activated<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);
    }

    {
        typedef decltype(add_default(value(0), value(1))) signal_t;
        REQUIRE(signal_is_move_activated<signal_t>::value);
        REQUIRE(!signal_is_writable<signal_t>::value);
    }

    {
        p = 1;
        auto s = make_default(true, true, true);
        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(move_signal(s) == 1);
        REQUIRE(signal_ready_to_write(s));
        write_signal(s, 2);
        REQUIRE(p == 2);
        REQUIRE(f == 0);
    }

    {
        p = 1;
        auto s = make_default(false, true, true);
        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 0);
        REQUIRE(move_signal(s) == 0);
        REQUIRE(signal_ready_to_write(s));
        write_signal(s, 2);
        REQUIRE(p == 2);
        REQUIRE(f == 0);
    }

    {
        p = 1;
        auto s = make_default(false, true, false);
        REQUIRE(!signal_has_value(s));
        REQUIRE(signal_ready_to_write(s));
        write_signal(s, 2);
        REQUIRE(p == 2);
        REQUIRE(f == 0);
    }

    {
        p = 1;
        auto s = make_default(true, false, false);
        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(move_signal(s) == 1);
        REQUIRE(!signal_ready_to_write(s));
    }

    {
        // Check that the overall signal produces a different value ID when
        // using the primary vs the default, even when the two component
        // signals have the same value ID.
        p = 0;
        auto s = make_default(true, false, false);
        auto t = make_default(false, false, true);
        REQUIRE(signal_has_value(s));
        REQUIRE(signal_has_value(t));
        REQUIRE(read_signal(s) == read_signal(t));
        REQUIRE(move_signal(s) == move_signal(t));
        REQUIRE(s.value_id() != t.value_id());
    }
}

TEST_CASE("simplify_id", "[signals][adaptors]")
{
    using namespace alia;

    auto c = std::map<int, std::string>{{2, "1"}, {0, "3"}};
    auto c_signal = direct(c);
    auto unwrapped = c_signal[value(2)];
    auto s = simplify_id(unwrapped);

    typedef decltype(s) signal_t;
    REQUIRE((std::is_same<signal_t::value_type, std::string>::value));
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(signal_is_writable<signal_t>::value);

    REQUIRE(s.value_id() != unwrapped.value_id());
    REQUIRE(s.value_id() == make_id_by_reference(c[2]));
    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == "1");
    REQUIRE(signal_ready_to_write(s));
    write_signal(s, "7");
    REQUIRE((c == std::map<int, std::string>{{2, "7"}, {0, "3"}}));
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

TEST_CASE("mask a duplex signal", "[signals][adaptors]")
{
    {
        int x = 1;
        auto d = direct(x);
        auto s = mask(d, true);

        typedef decltype(s) signal_t;
        REQUIRE((std::is_same<signal_t::value_type, int>::value));
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(s.value_id() == d.value_id());
        REQUIRE(signal_ready_to_write(s));
        write_signal(s, 0);
        REQUIRE(x == 0);
    }
    {
        int x = 1;
        auto s = mask(direct(x), false);
        REQUIRE(!signal_has_value(s));
        REQUIRE(!signal_ready_to_write(s));
        REQUIRE(s.value_id() == null_id);
    }
}

TEST_CASE("mask a read-only signal", "[signals][adaptors]")
{
    {
        int x = 1;
        auto d = value(x);
        auto s = mask(d, true);

        typedef decltype(s) signal_t;
        REQUIRE((std::is_same<signal_t::value_type, int>::value));
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(!signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(s.value_id() == d.value_id());
    }
    {
        int x = 1;
        auto s = mask(value(x), false);
        REQUIRE(!signal_has_value(s));
        REQUIRE(s.value_id() == null_id);
    }
}

TEST_CASE("mask a raw value", "[signals][adaptors]")
{
    int x = 12;
    {
        auto s = mask(x, true);
        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == x);
    }
    {
        auto s = mask(x, false);
        REQUIRE(!signal_has_value(s));
    }
}

TEST_CASE("mask/disable_writes", "[signals][adaptors]")
{
    // duplex signal, unmasked
    {
        int x = 1;
        auto wrapped = direct(x);
        auto s = mask_writes(wrapped, value(true));

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(s.value_id() == wrapped.value_id());
        REQUIRE(signal_ready_to_write(s));
        write_signal(s, 0);
        REQUIRE(x == 0);
    }

    // duplex signal, masked (via disable_writes)
    {
        int x = 1;
        auto wrapped = direct(x);
        auto s = disable_writes(wrapped);

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(s.value_id() == wrapped.value_id());
        REQUIRE(!signal_ready_to_write(s));
    }

    // duplex signal, masked (via empty signal)
    {
        int x = 1;
        auto wrapped = direct(x);
        auto s = mask_writes(wrapped, empty<bool>());

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(s.value_id() == wrapped.value_id());
        REQUIRE(!signal_ready_to_write(s));
    }

    // duplex signal, masked (via raw flag)
    {
        int x = 1;
        auto wrapped = direct(x);
        auto s = mask_writes(wrapped, false);

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(s.value_id() == wrapped.value_id());
        REQUIRE(!signal_ready_to_write(s));
    }

    // read-only signal, unmasked
    {
        auto wrapped = value(1);
        auto s = mask_writes(wrapped, true);

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(!signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(s.value_id() == wrapped.value_id());
    }

    // read-only signal, masked
    {
        auto wrapped = value(1);
        auto s = mask_writes(wrapped, false);

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(!signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(s.value_id() == wrapped.value_id());
    }
}

TEST_CASE("mask/disable_reads", "[signals][adaptors]")
{
    // duplex signal, unmasked
    {
        int x = 1;
        auto wrapped = direct(x);
        auto s = mask_reads(wrapped, value(true));

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(s.value_id() == wrapped.value_id());
        REQUIRE(signal_ready_to_write(s));
        write_signal(s, 0);
        REQUIRE(x == 0);
    }

    // duplex signal, masked (via disable_reads)
    {
        int x = 1;
        auto wrapped = direct(x);
        auto s = disable_reads(wrapped);

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);

        REQUIRE(!signal_has_value(s));
        REQUIRE(signal_ready_to_write(s));
        write_signal(s, 0);
        REQUIRE(x == 0);
    }

    // duplex signal, masked (via empty signal)
    {
        int x = 1;
        auto wrapped = direct(x);
        auto s = mask_reads(wrapped, empty<bool>());

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);

        REQUIRE(!signal_has_value(s));
        REQUIRE(signal_ready_to_write(s));
        write_signal(s, 0);
        REQUIRE(x == 0);
    }

    // duplex signal, masked (via raw flag)
    {
        int x = 1;
        auto wrapped = direct(x);
        auto s = mask_reads(wrapped, false);

        typedef decltype(s) signal_t;
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);

        REQUIRE(!signal_has_value(s));
        REQUIRE(signal_ready_to_write(s));
        write_signal(s, 0);
        REQUIRE(x == 0);
    }
}

TEST_CASE("unwrap a duplex signal", "[signals][adaptors]")
{
    {
        auto x = std::optional<int>(1);
        auto d = direct(x);
        auto s = unwrap(d);

        typedef decltype(s) signal_t;
        REQUIRE((std::is_same<signal_t::value_type, int>::value));
        REQUIRE(signal_is_readable<signal_t>::value);
        REQUIRE(signal_is_writable<signal_t>::value);

        REQUIRE(signal_has_value(s));
        REQUIRE(read_signal(s) == 1);
        REQUIRE(s.value_id() == d.value_id());
        REQUIRE(signal_ready_to_write(s));
        write_signal(s, 0);
        REQUIRE(x.has_value());
        REQUIRE(*x == 0);
    }
    {
        auto x = std::optional<int>();
        auto s = unwrap(direct(x));
        REQUIRE(!signal_has_value(s));
        REQUIRE(s.value_id() == null_id);
        REQUIRE(signal_ready_to_write(s));
        write_signal(s, 0);
        REQUIRE(x.has_value());
        REQUIRE(*x == 0);
    }
    {
        auto x = std::optional<int>(1);
        auto s = unwrap(direct(x));
        clear_signal(s);
        REQUIRE(!x.has_value());
    }
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
