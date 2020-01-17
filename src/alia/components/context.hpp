#ifndef ALIA_COMPONENTS_CONTEXT_HPP
#define ALIA_COMPONENTS_CONTEXT_HPP

#include <alia/components/typing.hpp>

namespace alia {

struct data_traversal;
struct data_traversal_tag
{
};

struct event_traversal;
struct event_traversal_tag
{
};

template<class Tag>
struct component_manipulator;

// The structure we use to store components. It provides direct storage of the
// commonly-used components in the core of alia.

struct component_storage
{
    data_traversal* data = 0;
    event_traversal* event = 0;
    // generic storage for other components
    generic_component_storage<any_pointer> other;
};

// All component access is done through the following 'manipulator' structure.
// Specializations can be defined for tags that have direct storage.

template<class Tag>
struct component_manipulator
{
    static bool
    has(component_storage& storage)
    {
        return has_storage_component<Tag>(storage.other);
    }
    static void
    add(component_storage& storage, any_pointer data)
    {
        add_storage_component<Tag>(storage.other, data);
    }
    static void
    remove(component_storage& storage)
    {
        remove_storage_component<Tag>(storage.other);
    }
    static any_pointer
    get(component_storage& storage)
    {
        return get_storage_component<Tag>(storage.other);
    }
};

template<>
struct component_manipulator<data_traversal_tag>
{
    static bool
    has(component_storage& storage)
    {
        return storage.data != 0;
    }
    static void
    add(component_storage& storage, data_traversal* data)
    {
        storage.data = data;
    }
    static void
    remove(component_storage& storage)
    {
        storage.data = 0;
    }
    static data_traversal*
    get(component_storage& storage)
    {
#ifdef ALIA_DYNAMIC_COMPONENT_CHECKING
        if (!storage.data)
            throw "missing data traversal component";
#endif
        return storage.data;
    }
};

template<>
struct component_manipulator<event_traversal_tag>
{
    static bool
    has(component_storage& storage)
    {
        return storage.event != 0;
    }
    static void
    add(component_storage& storage, event_traversal* event)
    {
        storage.event = event;
    }
    static void
    remove(component_storage& storage)
    {
        storage.event = 0;
    }
    static event_traversal*
    get(component_storage& storage)
    {
#ifdef ALIA_DYNAMIC_COMPONENT_CHECKING
        if (!storage.event)
            throw "missing event traversal component";
#endif
        return storage.event;
    }
};

// The following is the implementation of the interface expected of component
// storage objects. It simply forwards the requests along to the appropriate
// manipulator.

template<class Tag>
bool
has_storage_component(component_storage& storage)
{
    return component_manipulator<Tag>::has(storage);
}

template<class Tag, class Data>
void
add_storage_component(component_storage& storage, Data&& data)
{
    component_manipulator<Tag>::add(storage, std::forward<Data&&>(data));
}

template<class Tag>
void
remove_storage_component(component_storage& storage)
{
    component_manipulator<Tag>::remove(storage);
}

template<class Tag>
auto
get_storage_component(component_storage& storage)
{
    return component_manipulator<Tag>::get(storage);
}

template<class Function>
void
for_each_storage_component(component_storage& storage, Function f)
{
    if (storage.data)
        f(*storage.data);
    if (storage.event)
        f(*storage.event);
    for_each_storage_component(storage.other, f);
}

// Finally, the typedef for the context...

typedef add_component_type_t<
    empty_component_collection<component_storage>,
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
