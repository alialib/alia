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

    template<class Function>
    void
    for_each(Function f)
    {
        for (auto& i : this->components)
        {
            f(i.second);
        }
    }
};

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

    void* ptr;
};

template<class Pointer>
struct component_caster<any_pointer, Pointer*>
{
    static Pointer*
    apply(any_pointer stored)
    {
        return reinterpret_cast<Pointer*>(stored.ptr);
    }
};

// any_value is a way of storing any object by value as a component. It's very
// similar to boost/std::any and those could be used as the implementation if it
// didn't complicate the dependencies of alia so much...
struct untyped_cloneable_value_holder
{
    virtual ~untyped_cloneable_value_holder()
    {
    }
    virtual untyped_cloneable_value_holder*
    clone() const = 0;
};
template<class T>
struct typed_cloneable_value_holder : untyped_cloneable_value_holder
{
    explicit typed_cloneable_value_holder(T const& value) : value(value)
    {
    }
    explicit typed_cloneable_value_holder(T&& value)
        : value(static_cast<T&&>(value))
    {
    }
    untyped_cloneable_value_holder*
    clone() const
    {
        return new typed_cloneable_value_holder(value);
    }
    T value;
};
struct any_value
{
    // default constructor
    any_value() : holder_(nullptr)
    {
    }
    // destructor
    ~any_value()
    {
        delete holder_;
    }
    // copy constructor
    any_value(any_value const& other)
        : holder_(other.holder_ ? other.holder_->clone() : nullptr)
    {
    }
    // move constructor
    any_value(any_value&& other) : holder_(other.holder_)
    {
        other.holder_ = nullptr;
    }
    // constructor for concrete values
    template<class T>
    explicit any_value(T const& value)
        : holder_(new typed_cloneable_value_holder<T>(value))
    {
    }
    // constructor for concrete values (by rvalue)
    template<typename T>
    any_value(
        T&& value,
        std::enable_if_t<
            !std::is_same<any_value&, T>::value
            && !std::is_const<T>::value>* = nullptr)
        : holder_(
              new typed_cloneable_value_holder<typename std::decay<T>::type>(
                  static_cast<T&&>(value)))
    {
    }
    // swap
    void
    swap(any_value& other)
    {
        std::swap(holder_, other.holder_);
    }
    // assignment operator
    template<class T>
    any_value&
    operator=(T&& value)
    {
        any_value(static_cast<T&&>(value)).swap(*this);
        return *this;
    }
    // value holder
    untyped_cloneable_value_holder* holder_;
};
static inline void
swap(any_value& a, any_value& b)
{
    a.swap(b);
}
template<class T>
T const*
any_cast(any_value const* a)
{
    typed_cloneable_value_holder<T> const* ptr
        = dynamic_cast<typed_cloneable_value_holder<T> const*>(a->holder_);
    return ptr ? &ptr->value : nullptr;
}
template<class T>
T*
any_cast(any_value* a)
{
    typed_cloneable_value_holder<T>* ptr
        = dynamic_cast<typed_cloneable_value_holder<T>*>(a->holder_);
    return ptr ? &ptr->value : nullptr;
}

template<class Value>
struct component_caster<any_value&, Value>
{
    static Value&
    apply(any_value& stored)
    {
        return *any_cast<Value>(&stored);
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
    add(Storage& storage, any_pointer data)
    {
        storage.generic.template add<Tag>(data);
    }
    static void
    remove(Storage& storage)
    {
        storage.generic.template remove<Tag>();
    }
    static any_pointer
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
    auto get()                                                                 \
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
