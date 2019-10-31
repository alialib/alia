#define ALIA_LOWERCASE_MACROS

#include <alia/signals/text.hpp>

#include <catch.hpp>

#include <alia/flow/events.hpp>
#include <alia/signals/basic.hpp>
#include <alia/system.hpp>

using namespace alia;

namespace {

struct ostream_event
{
    std::ostream* stream;
};

void
do_text(context ctx, input<std::string> const& text)
{
    ostream_event* oe;
    if (detect_event(ctx, &oe) && signal_is_readable(text))
    {
        *oe->stream << read_signal(text) << ";";
    }
}

template<class Controller>
void
check_traversal(
    alia::system& sys,
    Controller const& controller,
    std::string const& expected_output)
{
    sys.controller = controller;
    {
        ostream_event oe;
        std::ostringstream s;
        oe.stream = &s;
        dispatch_event(sys, oe);
        REQUIRE(s.str() == expected_output);
    }
}

} // namespace

TEST_CASE("printf", "[signals]")
{
    alia::system sys;

    auto controller = [&](context ctx) {
        do_text(ctx, printf(ctx, "hello %s", value("world")));
        do_text(ctx, printf(ctx, "n is %4.1f", value(2.125)));
        do_text(ctx, printf(ctx, "bad format: %q", value(0)));
    };

    check_traversal(sys, controller, "hello world;n is  2.1;");
}
