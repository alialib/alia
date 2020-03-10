#ifndef ALIA_COMPONENTS_STORAGE_HPP
#define ALIA_COMPONENTS_STORAGE_HPP

#include <type_traits>
#include <typeindex>
#include <unordered_map>

#include <alia/components/typing.hpp>

namespace alia {

// generic_component_storage is one possible implementation of the underlying
// container for storing components and their associated data.
// :Data is the type used to store component data.
template<class Data>
struct generic_component_storage
{
    std::unordered_map<std::type_index, Data> components;

    template<class Tag>
    bool
    has() const
    {
        return this->components.find(std::type_index(typeid(Tag)))
               != this->components.end();
    }

    template<class Tag, class ComponentData>
    void
    add(ComponentData&& data)
    {
        this->components[std::type_index(typeid(Tag))]
            = std::forward<ComponentData&&>(data);
    }

    template<class Tag>
    void
    remove()
    {
        this->components.erase(std::type_index(typeid(Tag)));
    }

    template<class Tag>
    Data&
    get()
    {
        return this->components.at(std::type_index(typeid(Tag)));
    }
};

// any_ref is a simple way to store references to any type in a
// generic_component_storage object.
struct any_ref
{
    any_ref()
    {
    }

    template<class T>
    any_ref(std::reference_wrapper<T> ref) : ptr(&ref.get())
    {
    }

    void* ptr;
};

template<class T>
struct tagged_data_caster<any_ref&, T&>
{
    static T&
    apply(any_ref stored)
    {
        return *reinterpret_cast<T*>(stored.ptr);
    }
};
template<class T>
struct tagged_data_caster<any_ref, T&>
{
    static T&
    apply(any_ref stored)
    {
        return *reinterpret_cast<T*>(stored.ptr);
    }
};

// The following provides a small framework for defining more specialized
// component storage structures with direct storage of frequently used
// components. See context.hpp for an example of how it's used.

template<class Storage, class Tag>
struct component_accessor
{
    static bool
    has(Storage const& storage)
    {
        return storage.generic.template has<Tag>();
    }
    static void
    add(Storage& storage, any_ref data)
    {
        storage.generic.template add<Tag>(data);
    }
    static void
    remove(Storage& storage)
    {
        storage.generic.template remove<Tag>();
    }
    static any_ref
    get(Storage& storage)
    {
        return storage.generic.template get<Tag>();
    }
};

#define ALIA_IMPLEMENT_STORAGE_COMPONENT_ACCESSORS(Storage)                    \
    template<class Tag>                                                        \
    bool has() const                                                           \
    {                                                                          \
        return component_accessor<Storage, Tag>::has(*this);                   \
    }                                                                          \
                                                                               \
    template<class Tag, class Data>                                            \
    void add(Data&& data)                                                      \
    {                                                                          \
        component_accessor<Storage, Tag>::add(                                 \
            *this, std::forward<Data&&>(data));                                \
    }                                                                          \
                                                                               \
    template<class Tag>                                                        \
    void remove()                                                              \
    {                                                                          \
        component_accessor<Storage, Tag>::remove(*this);                       \
    }                                                                          \
                                                                               \
    template<class Tag>                                                        \
    decltype(auto) get()                                                       \
    {                                                                          \
        return component_accessor<Storage, Tag>::get(*this);                   \
    }

#define ALIA_ADD_DIRECT_COMPONENT_ACCESS(Storage, Tag, name)                   \
    template<>                                                                 \
    struct component_accessor<Storage, Tag>                                    \
    {                                                                          \
        static bool                                                            \
        has(Storage const& storage)                                            \
        {                                                                      \
            return storage.name != nullptr;                                    \
        }                                                                      \
        static void                                                            \
        add(Storage& storage, Tag::data_type data)                             \
        {                                                                      \
            storage.name = &data;                                              \
        }                                                                      \
        static void                                                            \
        remove(Storage& storage)                                               \
        {                                                                      \
            storage.name = nullptr;                                            \
        }                                                                      \
        static Tag::data_type                                                  \
        get(Storage& storage)                                                  \
        {                                                                      \
            return *storage.name;                                              \
        }                                                                      \
    };

} // namespace alia

#endif
