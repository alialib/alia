#ifndef ALIA_CORE_UNIT_TESTS_TRAVERSAL_HPP
#define ALIA_CORE_UNIT_TESTS_TRAVERSAL_HPP

#include <alia/core/flow/events.hpp>
#include <alia/core/system/internals.hpp>

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
        detail::dispatch_event(sys, oe);
        REQUIRE(s.str() == expected_output);
    }
}

} // namespace

#endif
