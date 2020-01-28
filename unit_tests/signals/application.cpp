#define ALIA_LOWERCASE_MACROS

#include <alia/signals/application.hpp>

#include <catch.hpp>

#include <alia/signals/basic.hpp>

#include "traversal.hpp"

using namespace alia;

TEST_CASE("lazy_apply", "[signals][application]")
{
    auto s1 = lazy_apply([](int i) { return 2 * i; }, val(1));

    typedef decltype(s1) signal_t1;
    REQUIRE(signal_can_read<signal_t1>::value);
    REQUIRE(!signal_can_write<signal_t1>::value);

    REQUIRE(signal_is_readable(s1));
    REQUIRE(read_signal(s1) == 2);

    auto s2
        = lazy_apply([](int i, int j) { return i + j; }, val(1), val(6));

    typedef decltype(s2) signal_t2;
    REQUIRE(signal_can_read<signal_t2>::value);
    REQUIRE(!signal_can_write<signal_t2>::value);

    REQUIRE(signal_is_readable(s2));
    REQUIRE(read_signal(s2) == 7);
    REQUIRE(s1.value_id() != s2.value_id());

    // Create some similar signals to make sure that they produce different
    // value IDs.
    auto s3
        = lazy_apply([](int i, int j) { return i + j; }, val(2), val(6));
    auto s4
        = lazy_apply([](int i, int j) { return i + j; }, val(1), val(0));
    REQUIRE(s2.value_id() != s3.value_id());
    REQUIRE(s2.value_id() != s4.value_id());
    REQUIRE(s3.value_id() != s4.value_id());
}

TEST_CASE("lazy_lift", "[signals][application]")
{
    auto s = lazy_lift([](int i) { return 2 * i; })(val(1));

    typedef decltype(s) signal_t;
    REQUIRE(signal_can_read<signal_t>::value);
    REQUIRE(!signal_can_write<signal_t>::value);

    REQUIRE(signal_is_readable(s));
    REQUIRE(read_signal(s) == 2);
}

TEST_CASE("simple apply", "[signals][application]")
{
    int f_call_count = 0;
    auto f = [&](int x, int y) {
        ++f_call_count;
        return x * 2 + y;
    };

    captured_id signal_id;

    alia::system sys;
    auto make_controller = [&](int x, int y) {
        return [=, &signal_id](context ctx) {
            auto s = apply(ctx, f, val(x), val(y));

            typedef decltype(s) signal_t;
            REQUIRE(signal_can_read<signal_t>::value);
            REQUIRE(!signal_can_write<signal_t>::value);

            REQUIRE(signal_is_readable(s));
            REQUIRE(read_signal(s) == x * 2 + y);

            signal_id.capture(s.value_id());
        };
    };

    do_traversal(sys, make_controller(1, 2));
    REQUIRE(f_call_count == 1);
    captured_id last_id = signal_id;

    do_traversal(sys, make_controller(1, 2));
    REQUIRE(f_call_count == 1);
    REQUIRE(last_id == signal_id);
    last_id = signal_id;

    do_traversal(sys, make_controller(2, 2));
    REQUIRE(f_call_count == 2);
    REQUIRE(last_id != signal_id);
    last_id = signal_id;

    do_traversal(sys, make_controller(2, 2));
    REQUIRE(f_call_count == 2);
    REQUIRE(last_id == signal_id);
    last_id = signal_id;

    do_traversal(sys, make_controller(2, 3));
    REQUIRE(f_call_count == 3);
    REQUIRE(last_id != signal_id);
}

TEST_CASE("unready apply", "[signals][application]")
{
    int f_call_count = 0;
    auto f = [&](int x, int y) {
        ++f_call_count;
        return x * 2 + y;
    };

    {
        alia::system sys;
        auto make_controller = [=](auto x, auto y) {
            return [=](context ctx) {
                auto s = apply(ctx, f, x, y);

                typedef decltype(s) signal_t;
                REQUIRE(signal_can_read<signal_t>::value);
                REQUIRE(!signal_can_write<signal_t>::value);

                REQUIRE(!signal_is_readable(s));
            };
        };

        do_traversal(sys, make_controller(empty<int>(), val(2)));
        REQUIRE(f_call_count == 0);

        do_traversal(sys, make_controller(val(1), empty<int>()));
        REQUIRE(f_call_count == 0);
    }
}

TEST_CASE("failed apply", "[signals][application]")
{
    auto f = [&](int x, int y) -> int { throw "failed"; };

    {
        alia::system sys;
        auto make_controller = [=](auto x, auto y) {
            return [=](context ctx) {
                auto s = apply(ctx, f, x, y);

                typedef decltype(s) signal_t;
                REQUIRE(signal_can_read<signal_t>::value);
                REQUIRE(!signal_can_write<signal_t>::value);

                REQUIRE(!signal_is_readable(s));
            };
        };

        do_traversal(sys, make_controller(val(1), val(2)));
    }
}

TEST_CASE("lift", "[signals][application]")
{
    int f_call_count = 0;
    auto f = [&](int x) {
        ++f_call_count;
        return x + 1;
    };

    {
        alia::system sys;
        auto controller = [=](context ctx) {
            auto f_lifted = lift(ctx, f);
            auto s = f_lifted(val(0));

            typedef decltype(s) signal_t;
            REQUIRE(signal_can_read<signal_t>::value);
            REQUIRE(!signal_can_write<signal_t>::value);

            REQUIRE(signal_is_readable(s));
            REQUIRE(read_signal(s) == 1);
        };

        do_traversal(sys, controller);
        REQUIRE(f_call_count == 1);
    }
}

TEST_CASE("alia_method", "[signals][application]")
{
    auto v = val("test text");
    REQUIRE(read_signal(lazy_apply(ALIA_METHOD(length), v)) == 9);
    REQUIRE(
        read_signal(lazy_apply(alia_method(substr), v, val(5))) == "text");
}
