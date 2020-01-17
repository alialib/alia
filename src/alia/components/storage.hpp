#ifndef ALIA_COMPONENTS_STORAGE_HPP
#define ALIA_COMPONENTS_STORAGE_HPP

#include <typeindex>
#include <unordered_map>

namespace alia {

// generic_component_storage is one possible implementation of the underlying
// container for storing components and their associated data.
// :Data is the type used to store component data.
template<class Data>
struct generic_component_storage
{
    std::unordered_map<std::type_index, Data> components;
};

// Does the storage object have a component with the given tag?
template<class Tag, class Data>
bool
has_storage_component(generic_component_storage<Data>& storage)
{
    return storage.components.find(std::type_index(typeid(Tag)))
           != storage.components.end();
}

// Store a component.
template<class Tag, class StorageData, class ComponentData>
void
add_storage_component(
    generic_component_storage<StorageData>& storage, ComponentData&& data)
{
    storage.components[std::type_index(typeid(Tag))]
        = std::forward<ComponentData&&>(data);
}

// Remove a component.
template<class Tag, class Data>
void
remove_storage_component(generic_component_storage<Data>& storage)
{
    storage.components.erase(std::type_index(typeid(Tag)));
}

// Retrieve the data for a component.
template<class Tag, class Data>
Data&
get_storage_component(generic_component_storage<Data>& storage)
{
    return storage.components.at(std::type_index(typeid(Tag)));
}

// Invoke f on each component within the storage object.
template<class Data, class Function>
void
for_each_storage_component(generic_component_storage<Data>& storage, Function f)
{
    for (auto& i : storage.components)
    {
        f(i.second);
    }
}

// any_pointer is a simple way to store pointers to any type in a
// generic_component_storage object.
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

// The following provides a small framework for defining more specialized
// component storage structures with direct storage of frequently used
// components. See context.hpp for an example of how it's used.

template<class Storage, class Tag>
struct component_accessor
{
    static bool
    has(Storage& storage)
    {
        return has_storage_component<Tag>(storage.generic);
    }
    static void
    add(Storage& storage, any_pointer data)
    {
        add_storage_component<Tag>(storage.generic, data);
    }
    static void
    remove(Storage& storage)
    {
        remove_storage_component<Tag>(storage.generic);
    }
    static any_pointer
    get(Storage& storage)
    {
        return get_storage_component<Tag>(storage.generic);
    }
};

#define ALIA_IMPLEMENT_STORAGE_COMPONENT_ACCESSORS(Storage)                    \
    template<class Tag>                                                        \
    bool has_storage_component(Storage& storage)                               \
    {                                                                          \
        return component_accessor<Storage, Tag>::has(storage);                 \
    }                                                                          \
                                                                               \
    template<class Tag, class Data>                                            \
    void add_storage_component(Storage& storage, Data&& data)                  \
    {                                                                          \
        component_accessor<Storage, Tag>::add(                                 \
            storage, std::forward<Data&&>(data));                              \
    }                                                                          \
                                                                               \
    template<class Tag>                                                        \
    void remove_storage_component(Storage& storage)                            \
    {                                                                          \
        component_accessor<Storage, Tag>::remove(storage);                     \
    }                                                                          \
                                                                               \
    template<class Tag>                                                        \
    auto get_storage_component(Storage& storage)                               \
    {                                                                          \
        return component_accessor<Storage, Tag>::get(storage);                 \
    }

#define ALIA_ADD_DIRECT_COMPONENT_ACCESS(Storage, Tag, name)                   \
    template<>                                                                 \
    struct component_accessor<Storage, Tag>                                    \
    {                                                                          \
        static bool                                                            \
        has(Storage& storage)                                                  \
        {                                                                      \
            return storage.name != nullptr;                                    \
        }                                                                      \
        static void                                                            \
        add(Storage& storage, Tag::data_type data)                             \
        {                                                                      \
            storage.name = data;                                               \
        }                                                                      \
        static void                                                            \
        remove(Storage& storage)                                               \
        {                                                                      \
            storage.name = nullptr;                                            \
        }                                                                      \
        static Tag::data_type                                                  \
        get(Storage& storage)                                                  \
        {                                                                      \
            return storage.name;                                               \
        }                                                                      \
    };

} // namespace alia

#endif
