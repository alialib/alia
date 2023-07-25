#ifndef ALIA_INDIE_EVENTS_UI_HPP
#define ALIA_INDIE_EVENTS_UI_HPP

#include <alia/indie/common.hpp>

namespace alia { namespace indie {

enum class event_category
{
    REFRESH,
    LAYOUT,
    REGION,
    RENDER,
    INPUT
};

struct ui_event
{
    event_category category;
};

// query_event provides a structure for representing events that are queries.
// (i.e., They are designed to gather information from one or more components,
// so they carry fields that represent parameters from the event creator as
// well as fields to represent the response from the components(s).)
template<class Parameters, class Response>
struct query_event
{
    Parameters parameters;
    std::optional<Response> response;

    query_event(Parameters parameters) : parameters(std::move(parameters))
    {
    }
};

}} // namespace alia::indie

#endif
