#include <alia/flow/for_each.hpp>

#include <sstream>

#include <alia/flow/events.hpp>
#include <alia/signals.hpp>
#include <alia/system.hpp>

#include <catch.hpp>

using namespace alia;

namespace {

struct ostream_event
{
    std::ostream* stream;
};

void
do_ostream_text(context ctx, input<std::string> const& text)
{
    ostream_event* oe;
    if (detect_event(ctx, &oe) && signal_is_readable(text))
    {
        *oe->stream << read_signal(text) << ";";
    }
}

void
do_vector_item(context ctx, bidirectional<std::string> const& item)
{
    do_ostream_text(ctx, item);
    REQUIRE(signal_is_writable(item));
}

void
do_vector_collection(context ctx)
{
    static std::vector<std::string> container{"foo", "bar", "baz"};
    for_each(ctx, direct(container), do_vector_item);
}

struct traversal_function
{
    void
    operator()(context ctx)
    {
        do_vector_collection(ctx);
    }
};

void
check_traversal(std::string const& expected_output)
{
    alia::system sys;
    sys.controller = traversal_function();
    {
        ostream_event oe;
        std::ostringstream s;
        oe.stream = &s;
        dispatch_event(sys, oe);
        REQUIRE(s.str() == expected_output);
    }
}

} // namespace

TEST_CASE("vector for_each", "[for_each]")
{
    check_traversal("foo;bar;baz;");
}
