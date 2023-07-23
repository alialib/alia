#define ALIA_LOWERCASE_MACROS

#include <alia/core/signals/text.hpp>

#include <catch2/catch_test_macros.hpp>

#include <alia/core/flow/macros.hpp>
#include <alia/core/flow/try_catch.hpp>

#include "traversal.hpp"

using namespace alia;

TEST_CASE("printf", "[signals][text]")
{
    alia::system sys;
    initialize_system<context>(sys, [](context) {});

    auto controller = [&](context ctx) {
        do_text(ctx, printf(ctx, "hello %s", value("world")));
        do_text(ctx, printf(ctx, "n is %4.1f", value(2.125)));
        // MSVC is too forgiving of bad format strings, so this test doesn't
        // actually work there.
#ifndef _MSC_VER
        alia_try
        {
            do_text(ctx, printf(ctx, "bad format: %q", value(0)));
        }
        alia_catch(...)
        {
            do_text(ctx, value("(error)"));
        }
        alia_end
#else
        do_text(ctx, value("(error)"));
#endif
    };

    check_traversal(sys, controller, "hello world;n is  2.1;(error);");
}

TEST_CASE("text conversions", "[signals][text]")
{
    {
        int x;
        from_string(&x, "17");
        REQUIRE(x == 17);
        from_string(&x, "-1");
        REQUIRE(x == -1);
        REQUIRE_THROWS_AS(from_string(&x, "a17"), validation_error);
        REQUIRE_THROWS_AS(from_string(&x, "1 04"), validation_error);
        REQUIRE_THROWS_AS(from_string(&x, "1 ;"), validation_error);
    }
    {
        signed short x;
        from_string(&x, "17");
        REQUIRE(x == 17);
        from_string(&x, "-1");
        REQUIRE(x == -1);
        REQUIRE_THROWS_AS(from_string(&x, "a17"), validation_error);
        REQUIRE_THROWS_AS(from_string(&x, "40000"), validation_error);
    }
    {
        unsigned short x;
        from_string(&x, "40000");
        REQUIRE(x == 40000);
        REQUIRE_THROWS_AS(from_string(&x, "a17"), validation_error);
        REQUIRE_THROWS_AS(from_string(&x, "-1"), validation_error);
        REQUIRE_THROWS_AS(from_string(&x, "70000"), validation_error);
    }
    {
        double x;
        from_string(&x, "4.5");
        REQUIRE(x == 4.5);
        REQUIRE_THROWS_AS(from_string(&x, "a17"), validation_error);
    }
}

TEST_CASE("as_text", "[signals][text]")
{
    alia::system sys;
    initialize_system<context>(sys, [](context) {});

    auto controller = [&](context ctx) {
        auto no_text = as_text(ctx, empty<int>());
        REQUIRE(!signal_has_value(no_text));

        do_text(ctx, as_text(ctx, value(-121)));
        do_text(ctx, as_text(ctx, value(121u)));
        do_text(ctx, as_text(ctx, value("hello!")));
        do_text(ctx, as_text(ctx, value(1.2)));
    };

    check_traversal(sys, controller, "-121;121;hello!;1.2;");
}

TEST_CASE("as_duplex_text", "[signals][text]")
{
    alia::system sys;
    initialize_system<context>(sys, [](context) {});

    auto controller = [&](context ctx) {
        auto no_text = as_duplex_text(ctx, empty<int>());
        REQUIRE(!signal_has_value(no_text));

        {
            int x = 12;
            auto x_text = as_duplex_text(ctx, direct(x));
            REQUIRE(signal_ready_to_write(x_text));
            write_signal(x_text, "4");
            REQUIRE(x == 4);
        }

        {
            double x = 1.2;
            auto x_text = as_duplex_text(ctx, direct(x));
            REQUIRE(signal_ready_to_write(x_text));
            write_signal(x_text, "4.5");
            REQUIRE(x == 4.5);
        }

        {
            std::string x = "hello";
            auto x_text = as_duplex_text(ctx, direct(x));
            REQUIRE(signal_ready_to_write(x_text));
            write_signal(x_text, "world");
            REQUIRE(x == "world");
        }

        do_text(ctx, as_duplex_text(ctx, value(-121)));
        do_text(ctx, as_duplex_text(ctx, value(121u)));
        do_text(ctx, as_duplex_text(ctx, value("hello!")));
        do_text(ctx, as_duplex_text(ctx, value(1.2)));
    };

    check_traversal(sys, controller, "-121;121;hello!;1.2;");
}

TEST_CASE("as_duplex_text value_id", "[signals][text]")
{
    alia::system sys;
    initialize_system<context>(sys, [](context) {});

    int x = 1;
    captured_id signal_id;

    auto make_controller = [&](std::string const& new_x) {
        return [&](context ctx) {
            auto x_text = as_duplex_text(ctx, direct(x));
            REQUIRE(signal_ready_to_write(x_text));
            write_signal(x_text, new_x);
            signal_id.capture(x_text.value_id());
        };
    };

    do_traversal(sys, make_controller("4"));
    REQUIRE(x == 4);
    captured_id last_id = signal_id;

    do_traversal(sys, make_controller("7"));
    REQUIRE(x == 7);
    REQUIRE(last_id != signal_id);
}
