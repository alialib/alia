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
        do_text(ctx, printf(ctx, "hello %s", value("world")));
        do_text(ctx, printf(ctx, "n is %4.1f", value(2.125)));
        // MSVC is too forgiving of bad format strings, so this test doesn't
        // actually work there.
#ifndef _MSC_VER
        do_text(ctx, printf(ctx, "bad format: %q", value(0)));
#endif
    };

    check_traversal(sys, controller, "hello world;n is  2.1;");
}
