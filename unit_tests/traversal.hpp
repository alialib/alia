#ifndef ALIA_TEST_TRAVERSAL_HPP
#define ALIA_TEST_TRAVERSAL_HPP

#include <alia/flow/events.hpp>
#include <alia/system.hpp>

#include <sstream>

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

#endif
