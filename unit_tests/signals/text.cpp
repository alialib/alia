#define ALIA_LOWERCASE_MACROS

#include <alia/signals/text.hpp>

#include <catch.hpp>

#include <alia/signals/basic.hpp>

#include "traversal.hpp"

using namespace alia;

TEST_CASE("printf", "[signals][text]")
{
    alia::system sys;

    auto controller = [&](context ctx) {
        do_text(ctx, printf(ctx, "hello %s", val("world")));
        do_text(ctx, printf(ctx, "n is %4.1f", val(2.125)));
        // MSVC is too forgiving of bad format strings, so this test doesn't
        // actually work there.
#ifndef _MSC_VER
        do_text(ctx, printf(ctx, "bad format: %q", val(0)));
#endif
    };

    check_traversal(sys, controller, "hello world;n is  2.1;");
}

TEST_CASE("as_text", "[signals][text]")
{
    alia::system sys;

    auto controller = [&](context ctx) {
        auto no_text = as_text(ctx, empty<int>());
        REQUIRE(!signal_is_readable(no_text));

        do_text(ctx, as_text(ctx, val(-121)));
        do_text(ctx, as_text(ctx, val(121u)));
        do_text(ctx, as_text(ctx, val("hello!")));
        do_text(ctx, as_text(ctx, val(1.2)));
    };

    check_traversal(sys, controller, "-121;121;hello!;1.2;");
}

TEST_CASE("as_bidirectional_text", "[signals][text]")
{
    alia::system sys;

    auto controller = [&](context ctx) {
        auto no_text = as_bidirectional_text(ctx, empty<int>());
        REQUIRE(!signal_is_readable(no_text));

        {
            int x = 12;
            auto x_text = as_bidirectional_text(ctx, direct(x));
            REQUIRE(signal_is_writable(x_text));
            write_signal(x_text, "4");
            REQUIRE(x == 4);
        }

        {
            double x = 1.2;
            auto x_text = as_bidirectional_text(ctx, direct(x));
            REQUIRE(signal_is_writable(x_text));
            write_signal(x_text, "4.5");
            REQUIRE(x == 4.5);
        }

        {
            std::string x = "hello";
            auto x_text = as_bidirectional_text(ctx, direct(x));
            REQUIRE(signal_is_writable(x_text));
            write_signal(x_text, "world");
            REQUIRE(x == "world");
        }

        do_text(ctx, as_bidirectional_text(ctx, val(-121)));
        do_text(ctx, as_bidirectional_text(ctx, val(121u)));
        do_text(ctx, as_bidirectional_text(ctx, val("hello!")));
        do_text(ctx, as_bidirectional_text(ctx, val(1.2)));
    };

    check_traversal(sys, controller, "-121;121;hello!;1.2;");
}
