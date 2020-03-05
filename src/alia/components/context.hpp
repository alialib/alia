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

struct timing_component;
ALIA_DEFINE_COMPONENT_TYPE(timing_tag, timing_component&)

// the structure we use to store components - It provides direct storage of the
// commonly-used components in the core of alia.

struct context_component_storage
{
    // directly-stored components
    system* sys = nullptr;
    event_traversal* event = nullptr;
    data_traversal* data = nullptr;
    timing_component* timing = nullptr;

    // generic storage for other components
    generic_component_storage<any_ref> generic;

    ALIA_IMPLEMENT_STORAGE_COMPONENT_ACCESSORS(context_component_storage)
};

ALIA_ADD_DIRECT_COMPONENT_ACCESS(context_component_storage, system_tag, sys)
ALIA_ADD_DIRECT_COMPONENT_ACCESS(
    context_component_storage, event_traversal_tag, event)
ALIA_ADD_DIRECT_COMPONENT_ACCESS(
    context_component_storage, data_traversal_tag, data)
ALIA_ADD_DIRECT_COMPONENT_ACCESS(context_component_storage, timing_tag, timing)

// the typedefs for the context - There are two because we want to be able to
// represent the context with and without data capabilities.

typedef add_component_types_t<
    empty_component_collection<context_component_storage>,
    system_tag,
    event_traversal_tag,
    timing_tag>
    dataless_context;

typedef add_component_type_t<dataless_context, data_traversal_tag> context;

// And some convenience functions...

context
make_context(
    context_component_storage* storage,
    system& sys,
    event_traversal& event,
    data_traversal& data,
    timing_component& timing);

template<class Context>
Context
copy_context(Context ctx)
{
    context_component_storage* new_storage;
    get_data(ctx, &new_storage);

    Context new_ctx(new_storage);
    *new_storage = *ctx.storage;

    return new_ctx;
}

template<class Tag, class Context, class Data>
auto
extend_context(Context ctx, Data& data)
{
    return add_component<Tag>(ctx, std::ref(data));
}

template<class Collection, class... Tag>
struct extend_context_type : add_component_types<Collection, Tag...>
{
};
template<class Collection, class... Tag>
using extend_context_type_t =
    typename extend_context_type<Collection, Tag...>::type;

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

} // namespace alia

#endif
