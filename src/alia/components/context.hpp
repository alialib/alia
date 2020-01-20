#ifndef ALIA_COMPONENTS_CONTEXT_HPP
#define ALIA_COMPONENTS_CONTEXT_HPP

#include <alia/components/storage.hpp>

namespace alia {

struct data_traversal;
ALIA_DEFINE_COMPONENT_TYPE(data_traversal_tag, data_traversal*)

struct event_traversal;
ALIA_DEFINE_COMPONENT_TYPE(event_traversal_tag, event_traversal*)

struct system;
ALIA_DEFINE_COMPONENT_TYPE(system_tag, system*)

// the structure we use to store components - It provides direct storage of the
// commonly-used components in the core of alia.

struct context_component_storage
{
    // directly-stored components
    system* sys = nullptr;
    event_traversal* event = nullptr;
    data_traversal* data = nullptr;

    // generic storage for other components
    generic_component_storage<any_pointer> generic;

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
    system* sys,
    event_traversal* event,
    data_traversal* data);

template<class Context>
event_traversal&
get_event_traversal(Context ctx)
{
    return *get_component<event_traversal_tag>(ctx);
}

template<class Context>
data_traversal&
get_data_traversal(Context ctx)
{
    return *get_component<data_traversal_tag>(ctx);
}

} // namespace alia

#endif
