#ifndef ALIA_CONTEXT_HPP
#define ALIA_CONTEXT_HPP

#include <alia/component_collection.hpp>

namespace alia {

struct data_traversal_tag
{
};

struct data_traversal;

// The structure we use to store components. It provides direct storage of the
// commonly-used components in the core of alia.
struct component_storage
{
    data_traversal* data;
};

// All component access is done through the following 'manipulator' structure.
// Specializations can be defined for tags that have direct storage.

template<class Tag>
struct component_manipulator
{
};

template<>
struct component_manipulator<data_traversal_tag>
{
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

// The following is the implementation of the interface expected of component
// storage objects. It simply forwards the requests along to the appropriate
// manipulator.

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
    empty_component_collection<component_storage>,
    data_traversal_tag,
    data_traversal*>
    context;

} // namespace alia

#endif
