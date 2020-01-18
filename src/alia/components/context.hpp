#ifndef ALIA_COMPONENTS_CONTEXT_HPP
#define ALIA_COMPONENTS_CONTEXT_HPP

#include <alia/components/storage.hpp>

namespace alia {

struct data_traversal;
ALIA_DEFINE_COMPONENT_TYPE(data_traversal_tag, data_traversal*)

struct event_traversal;
ALIA_DEFINE_COMPONENT_TYPE(event_traversal_tag, event_traversal*)

// the structure we use to store components - It provides direct storage of the
// commonly-used components in the core of alia.

struct context_component_storage
{
    // directly-stored components
    data_traversal* data = nullptr;
    event_traversal* event = nullptr;

    // generic storage for other components
    generic_component_storage<any_pointer> generic;

    ALIA_IMPLEMENT_STORAGE_COMPONENT_ACCESSORS(context_component_storage)

    template<class Function>
    void
    for_each(Function f)
    {
        if (this->data)
            f(*this->data);
        if (this->event)
            f(*this->event);
        this->generic.for_each(f);
    }
};

ALIA_ADD_DIRECT_COMPONENT_ACCESS(
    context_component_storage, data_traversal_tag, data)
ALIA_ADD_DIRECT_COMPONENT_ACCESS(
    context_component_storage, event_traversal_tag, event)

// the typedefs for the context - There are two because we want to be able to
// represent the context with and without data capabilities.

typedef add_component_type_t<
    empty_component_collection<context_component_storage>,
    event_traversal_tag>
    dataless_context;

typedef add_component_type_t<dataless_context, data_traversal_tag> context;

// And some convenience functions...

template<class Context>
data_traversal&
get_data_traversal(Context ctx)
{
    return *get_component<data_traversal_tag>(ctx);
}

bool
is_refresh_pass(context ctx);

template<class Context>
event_traversal&
get_event_traversal(Context ctx)
{
    return *get_component<event_traversal_tag>(ctx);
}

} // namespace alia

#endif
