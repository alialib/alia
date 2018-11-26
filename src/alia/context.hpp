#ifndef ALIA_CONTEXT_HPP
#define ALIA_CONTEXT_HPP

#include <alia/component_collection.hpp>
#include <alia/data_graph.hpp>

namespace alia {

struct data_traversal_tag
{
};
struct data_traversal;

struct event_traversal_tag
{
};
struct event_traversal;

struct any_pointer
{
    any_pointer()
    {
    }

    template<class T>
    any_pointer(T* ptr) : ptr(ptr)
    {
    }

    template<class T>
    operator T*()
    {
        return reinterpret_cast<T*>(ptr);
    }

    void* ptr;
};

template<class T>
bool
operator==(any_pointer p, T* other)
{
    return reinterpret_cast<T*>(p.ptr) == other;
}
template<class T>
bool
operator==(T* other, any_pointer p)
{
    return other == reinterpret_cast<T*>(p.ptr);
}
template<class T>
bool
operator!=(any_pointer p, T* other)
{
    return reinterpret_cast<T*>(p.ptr) != other;
}
template<class T>
bool
operator!=(T* other, any_pointer p)
{
    return other != reinterpret_cast<T*>(p.ptr);
}

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
        return has_component<Tag>(storage.other);
    }
    static void
    add(component_storage& storage, any_pointer data)
    {
        add_component<Tag>(storage.other, data);
    }
    static void
    remove(component_storage& storage)
    {
        remove_component<Tag>(storage.other);
    }
    static any_pointer
    get(component_storage& storage)
    {
        return get_component<Tag>(storage.other);
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
        return storage.event;
    }
};

// The following is the implementation of the interface expected of component
// storage objects. It simply forwards the requests along to the appropriate
// manipulator.

template<class Tag>
bool
has_component(component_storage& storage)
{
    return component_manipulator<Tag>::has(storage);
}

template<class Tag, class Data>
void
add_component(component_storage& storage, Data&& data)
{
    component_manipulator<Tag>::add(storage, std::forward<Data&&>(data));
}

template<class Tag>
void
remove_component(component_storage& storage)
{
    component_manipulator<Tag>::remove(storage);
}

template<class Tag>
auto
get_component(component_storage& storage)
{
    return component_manipulator<Tag>::get(storage);
}

// Finally, the typedef for the context...

typedef add_component_type_t<
    add_component_type_t<
        empty_component_collection<component_storage>,
        event_traversal_tag,
        event_traversal*>,
    data_traversal_tag,
    data_traversal*>
    context;

// And some convenience functions...

inline data_traversal&
get_data_traversal(context& ctx)
{
    return *get_component<data_traversal_tag>(ctx);
}

inline bool
is_refresh_pass(context& ctx)
{
    return get_data_traversal(ctx).gc_enabled;
}

inline event_traversal&
get_event_traversal(context& ctx)
{
    return *get_component<event_traversal_tag>(ctx);
}

} // namespace alia

#endif
