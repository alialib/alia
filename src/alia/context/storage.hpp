#ifndef ALIA_CONTEXT_STORAGE_HPP
#define ALIA_CONTEXT_STORAGE_HPP

#include <any>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

#include <alia/context/structural_typing.hpp>

namespace alia { namespace detail {

// generic_tagged_storage is one possible implementation of the underlying
// container for storing the actual data associated with a tag.
// :Data is the type used to the store data.
template<class Data>
struct generic_tagged_storage
{
    std::unordered_map<std::type_index, Data> objects;

    template<class Tag>
    bool
    has() const
    {
        return this->objects.find(std::type_index(typeid(Tag)))
               != this->objects.end();
    }

    template<class Tag, class ObjectData>
    void
    add(ObjectData&& data)
    {
        this->objects[std::type_index(typeid(Tag))]
            = std::forward<ObjectData&&>(data);
    }

    template<class Tag>
    void
    remove()
    {
        this->objects.erase(std::type_index(typeid(Tag)));
    }

    template<class Tag>
    Data&
    get()
    {
        return this->objects.at(std::type_index(typeid(Tag)));
    }
};

template<class T>
struct tagged_data_caster<std::any&, T&>
{
    static T&
    apply(std::any& stored)
    {
        return std::any_cast<std::reference_wrapper<T>>(stored);
    }
};
template<class T>
struct tagged_data_caster<std::any&, T>
{
    static T&
    apply(std::any& stored)
    {
        return std::any_cast<T&>(stored);
    }
};

// The following provides a small framework for defining more specialized
// storage structures with direct storage of frequently used objects. See
// context.hpp for an example of how it's used.

template<class Storage, class Tag>
struct tagged_data_accessor
{
    static bool
    has(Storage const& storage)
    {
        return storage.generic.template has<Tag>();
    }
    static void
    add(Storage& storage, std::any data)
    {
        storage.generic.template add<Tag>(std::move(data));
    }
    static void
    remove(Storage& storage)
    {
        storage.generic.template remove<Tag>();
    }
    static std::any&
    get(Storage& storage)
    {
        return storage.generic.template get<Tag>();
    }
};

#define ALIA_IMPLEMENT_STORAGE_OBJECT_ACCESSORS(Storage)                      \
    template<class Tag>                                                       \
    bool has() const                                                          \
    {                                                                         \
        return detail::tagged_data_accessor<Storage, Tag>::has(*this);        \
    }                                                                         \
                                                                              \
    template<class Tag, class Data>                                           \
    void add(Data&& data)                                                     \
    {                                                                         \
        detail::tagged_data_accessor<Storage, Tag>::add(                      \
            *this, std::forward<Data&&>(data));                               \
    }                                                                         \
                                                                              \
    template<class Tag>                                                       \
    void remove()                                                             \
    {                                                                         \
        detail::tagged_data_accessor<Storage, Tag>::remove(*this);            \
    }                                                                         \
                                                                              \
    template<class Tag>                                                       \
    decltype(auto) get()                                                      \
    {                                                                         \
        return detail::tagged_data_accessor<Storage, Tag>::get(*this);        \
    }

#define ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(Storage, Tag, name)                \
    namespace detail {                                                        \
    template<>                                                                \
    struct tagged_data_accessor<Storage, Tag>                                 \
    {                                                                         \
        static bool                                                           \
        has(Storage const& storage)                                           \
        {                                                                     \
            return storage.name != nullptr;                                   \
        }                                                                     \
        static void                                                           \
        add(Storage& storage, Tag::data_type data)                            \
        {                                                                     \
            storage.name = &data;                                             \
        }                                                                     \
        static void                                                           \
        remove(Storage& storage)                                              \
        {                                                                     \
            storage.name = nullptr;                                           \
        }                                                                     \
        static Tag::data_type                                                 \
        get(Storage& storage)                                                 \
        {                                                                     \
            return *storage.name;                                             \
        }                                                                     \
    };                                                                        \
    }

}} // namespace alia::detail

#endif
