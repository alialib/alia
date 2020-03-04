#ifndef ALIA_TEST_TRAVERSAL_HPP
#define ALIA_TEST_TRAVERSAL_HPP

#include <alia/components/system.hpp>
#include <alia/flow/events.hpp>

#include <sstream>

using namespace alia;

namespace {

struct ostream_event
{
    std::ostream* stream;
};

template<class Text>
void
do_text(context ctx, Text const& text)
{
    ostream_event* oe;
    if (detect_event(ctx, &oe) && signal_has_value(text))
    {
        *oe->stream << read_signal(text) << ";";
    }
}

template<class Controller>
void
do_traversal(alia::system& sys, Controller const& controller)
{
    sys.controller = controller;
    refresh_system(sys);
}

template<class Controller>
void
check_traversal(
    alia::system& sys,
    Controller const& controller,
    std::string const& expected_output)
{
    sys.controller = controller;
    refresh_system(sys);
    {
        ostream_event oe;
        std::ostringstream s;
        oe.stream = &s;
        impl::dispatch_event(sys, oe);
        REQUIRE(s.str() == expected_output);
    }
}

} // namespace

#endif
