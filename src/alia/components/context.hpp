#ifndef ALIA_COMPONENTS_CONTEXT_HPP
#define ALIA_COMPONENTS_CONTEXT_HPP

#include <alia/components/storage.hpp>
#include <alia/signals/basic.hpp>

namespace alia {

struct data_traversal;
ALIA_DEFINE_COMPONENT_TYPE(data_traversal_tag, data_traversal&)

struct event_traversal;
ALIA_DEFINE_COMPONENT_TYPE(event_traversal_tag, event_traversal&)

struct system;
ALIA_DEFINE_COMPONENT_TYPE(system_tag, system&)

// the structure we use to store components - It provides direct storage of the
// commonly-used components in the core of alia.

struct context_component_storage
{
    // directly-stored components
    system* sys = nullptr;
    event_traversal* event = nullptr;
    data_traversal* data = nullptr;

    // generic storage for other components
    generic_component_storage<any_ref> generic;

    ALIA_IMPLEMENT_STORAGE_COMPONENT_ACCESSORS(context_component_storage)
};

ALIA_ADD_DIRECT_COMPONENT_ACCESS(context_component_storage, system_tag, sys)
ALIA_ADD_DIRECT_COMPONENT_ACCESS(
    context_component_storage, event_traversal_tag, event)
ALIA_ADD_DIRECT_COMPONENT_ACCESS(
    context_component_storage, data_traversal_tag, data)

// the typedefs for the context - There are two because we want to be able to
// represent the context with and without data capabilities.

typedef add_component_type_t<
    add_component_type_t<
        empty_component_collection<context_component_storage>,
        system_tag>,
    event_traversal_tag>
    dataless_context;

typedef add_component_type_t<dataless_context, data_traversal_tag> context;

// And some convenience functions...

context
make_context(
    context_component_storage* storage,
    system& sys,
    event_traversal& event,
    data_traversal& data);

template<class Context>
event_traversal&
get_event_traversal(Context ctx)
{
    return get_component<event_traversal_tag>(ctx);
}

template<class Context>
data_traversal&
get_data_traversal(Context ctx)
{
    return get_component<data_traversal_tag>(ctx);
}

// And some functions for accessing the context/system timing capabilities...

// Currently, alia's only sense of time is that of a monotonically increasing
// millisecond counter. It's understood to have an arbitrary start point and is
// allowed to wrap around, so 'unsigned' is considered sufficient.
typedef unsigned millisecond_count;

// Request that the UI context refresh again quickly enough for smooth
// animation.
void
request_animation_refresh(dataless_context ctx);

// Get the value of the millisecond tick counter associated with the given
// UI context. This counter is updated every refresh pass, so it's consistent
// within a single frame.
// When this is called, it's assumed that something is currently animating, so
// it also requests a refresh.
millisecond_count
get_raw_animation_tick_count(dataless_context ctx);

// Same as above, but returns a signal rather than a raw integer.
value_signal<millisecond_count>
get_animation_tick_count(dataless_context ctx);

// Get the number of ticks remaining until the given end time.
// If the time has passed, this returns 0.
// This ensures that the UI context refreshes until the end time is reached.
millisecond_count
get_raw_animation_ticks_left(dataless_context ctx, millisecond_count end_tick);

} // namespace alia

#endif
