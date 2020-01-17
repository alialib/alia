#ifndef ALIA_COMPONENTS_PRIVILEGED_HPP
#define ALIA_COMPONENTS_PRIVILEGED_HPP

#include <alia/components/storage.hpp>

// This file defines the mechanics for storing the more frequently accessed
// components of alia (the 'privileged' ones) in a more efficient manner.

namespace alia {

// The following are the tags for the privileged components...

struct data_component
{
};

struct event_component
{
};

// the actual storage structure
template<class Data, class Event>
struct component_storage
{
    Data* data = 0;
    Event* event = 0;
    // generic storage for other (non-privileged) components
    generic_component_storage<any_pointer> other;
};

// All component access is done through the following 'manipulator' structure.
// Specializations can be defined for tags that have direct storage.

template<class Tag>
struct component_manipulator
{
    template<class Storage>
    static bool
    has(Storage& storage)
    {
        return has_storage_component<Tag>(storage.other);
    }
    template<class Storage>
    static void
    add(Storage& storage, any_pointer data)
    {
        add_storage_component<Tag>(storage.other, data);
    }
    template<class Storage>
    static void
    remove(Storage& storage)
    {
        remove_storage_component<Tag>(storage.other);
    }
    template<class Storage>
    static any_pointer
    get(Storage& storage)
    {
        return get_storage_component<Tag>(storage.other);
    }
};

template<>
struct component_manipulator<data_component>
{
    template<class Storage>
    static bool
    has(Storage& storage)
    {
        return storage.data != nullptr;
    }
    template<class Storage, class Data>
    static void
    add(Storage& storage, Data* data)
    {
        storage.data = data;
    }
    template<class Storage>
    static void
    remove(Storage& storage)
    {
        storage.data = nullptr;
    }
    template<class Storage>
    static auto*
    get(Storage& storage)
    {
#ifdef ALIA_DYNAMIC_COMPONENT_CHECKING
        if (!storage.data)
            throw "missing data component";
#endif
        return storage.data;
    }
};

template<>
struct component_manipulator<event_component>
{
    template<class Storage>
    static bool
    has(Storage& storage)
    {
        return storage.event != 0;
    }
    template<class Storage, class Event>
    static void
    add(Storage& storage, Event* event)
    {
        storage.event = event;
    }
    template<class Storage>
    static void
    remove(Storage& storage)
    {
        storage.event = 0;
    }
    template<class Storage>
    static auto*
    get(Storage& storage)
    {
#ifdef ALIA_DYNAMIC_COMPONENT_CHECKING
        if (!storage.event)
            throw "missing event component";
#endif
        return storage.event;
    }
};

// The following is the implementation of the interface expected of component
// storage objects. It simply forwards the requests along to the appropriate
// manipulator.

template<class Tag, class Data, class Event>
bool
has_storage_component(component_storage<Data, Event>& storage)
{
    return component_manipulator<Tag>::has(storage);
}

template<class Tag, class Component, class Data, class Event>
void
add_storage_component(
    component_storage<Data, Event>& storage, Component&& component)
{
    component_manipulator<Tag>::add(
        storage, std::forward<Component&&>(component));
}

template<class Tag, class Data, class Event>
void
remove_storage_component(component_storage<Data, Event>& storage)
{
    component_manipulator<Tag>::remove(storage);
}

template<class Tag, class Data, class Event>
auto
get_storage_component(component_storage<Data, Event>& storage)
{
    return component_manipulator<Tag>::get(storage);
}

template<class Function, class Data, class Event>
void
for_each_storage_component(component_storage<Data, Event>& storage, Function f)
{
    if (storage.data)
        f(*storage.data);
    if (storage.event)
        f(*storage.event);
    for_each_storage_component(storage.other, f);
}

} // namespace alia

#endif
