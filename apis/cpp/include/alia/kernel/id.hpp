#pragma once

#include <alia/abi/kernel/ids.h>

#include <concepts>
#include <functional>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <utility>

namespace alia {

// This file defines a thin C++ facade over `alia_id_view`.

using id_view = alia_id_view;

inline id_view
null_id()
{
    return alia_id_view_null();
}

inline id_view
unit_id()
{
    return alia_id_view_unit();
}

// value ID type tag for signals with a value that doesn't change over time
struct constant_value_tag
{
};

// `make_id(v)` for various common types - This takes care of selecting a
// compatible `alia_id_view` constructor based on the type.
// Note that this is only defined for types that can be fully stored within
// the `alia_id_view`.

inline id_view
make_id(bool v)
{
    return alia_id_view_make_u32(v ? 1u : 0u);
}

// This covers all integral types (excluding boolean) and selects the
// appropriate ID constructor based on the size and signedness of the type.
template<class T>
    requires std::integral<T> && (!std::same_as<T, bool>)
id_view
make_id(T v)
{
    if constexpr (sizeof(T) <= sizeof(int32_t))
    {
        if constexpr (std::signed_integral<T>)
            return alia_id_view_make_i32(static_cast<int32_t>(v));
        else
            return alia_id_view_make_u32(static_cast<uint32_t>(v));
    }
    else
    {
        if constexpr (std::signed_integral<T>)
            return alia_id_view_make_i64(static_cast<int64_t>(v));
        else
            return alia_id_view_make_u64(static_cast<uint64_t>(v));
    }
}

inline id_view
make_id(float v)
{
    return alia_id_view_make_bytes(
        reinterpret_cast<char const*>(&v), sizeof(float));
}

inline id_view
make_id(double v)
{
    static_assert(
        sizeof(double) <= ALIA_ID_INLINE_CAPACITY,
        "double must fit in alia_id_view inline payload");
    return alia_id_view_make_bytes(
        reinterpret_cast<char const*>(&v), sizeof(double));
}

// Construct an ID from a pointer. Use this only for immutable objects where
// address is equivalent to identity, such as string literals.
template<class T>
id_view
make_pointer_id(T const* ptr)
{
    return alia_id_view_make_pointer(static_cast<void const*>(ptr));
}

// `make_id_by_reference(v)` makes an ID for an existing object. The ID may
// borrow memory from `v`, so `v` must outlive any use of the returned
// `id_view`. This is designed to be overloaded for custom types.

inline id_view
make_id_by_reference(bool const& v)
{
    return make_id(v);
}

template<class T>
    requires std::integral<T> && (!std::same_as<T, bool>)
id_view
make_id_by_reference(T const& v)
{
    return make_id(v);
}

inline id_view
make_id_by_reference(float const& v)
{
    return make_id(v);
}

inline id_view
make_id_by_reference(double const& v)
{
    return make_id(v);
}

inline id_view
make_id_by_reference(std::string const& v)
{
    return alia_id_view_make_bytes(v.data(), static_cast<uint32_t>(v.size()));
}

template<class T>
    requires std::is_trivially_copyable_v<T> && (!std::integral<T>)
          && (!std::floating_point<T>)
id_view
make_id_by_reference(T const& v)
{
    return alia_id_view_make_bytes(
        reinterpret_cast<char const*>(&v), static_cast<uint32_t>(sizeof(T)));
}

// `identifiable<T>` is true iff an ID can be formed from a live `T` via
// `make_id_by_reference`.
template<class T>
concept identifiable = requires(T const& v) {
    { make_id_by_reference(v) } -> std::convertible_to<id_view>;
};

// Combine two IDs into a pair. `storage` must outlive the returned view.
inline id_view
make_id_pair(alia_id_pair& storage, id_view left, id_view right)
{
    return alia_id_view_make_pair(&storage, left, right);
}

// `erased_id_storage<T>` holds any persistent state needed to erase a typed
// value ID of type `T` into an `id_view`. Leaf types need no storage. Pair
// types nest child storage and an `alia_id_pair` node.
template<class T>
struct erased_id_storage
{
};

template<class A, class B>
struct erased_id_storage<std::pair<A, B>>
{
    // nested storage for the left ID
    [[no_unique_address]] erased_id_storage<A> left;
    // nested storage for the right ID
    [[no_unique_address]] erased_id_storage<B> right;
    // pair node backing the erased view
    alia_id_pair pair{};
};

// `to_id_view(storage, id)` converts a typed value ID into a C-compatible
// `id_view`. This is the general form that supports types that require storage
// outside of the id_view.
//
// For types that don't require storage, `to_id_view(id)` is also provided as a
// convenience.

inline id_view
to_id_view(id_view id)
{
    return id;
}

inline id_view
to_id_view(constant_value_tag)
{
    return unit_id();
}

inline id_view
to_id_view(bool v)
{
    return make_id(v);
}

template<class T>
    requires std::integral<T> && (!std::same_as<T, bool>)
id_view
to_id_view(T v)
{
    return make_id(v);
}

inline id_view
to_id_view(float v)
{
    return make_id(v);
}

inline id_view
to_id_view(double v)
{
    return make_id(v);
}

// A `char const*` value ID is erased as a pointer identity. (Using a `char
// const*` as the ID implies that it points to immutable text (e.g., a string
// literal).)
inline id_view
to_id_view(char const* p)
{
    return make_pointer_id(p);
}

template<class T>
    requires identifiable<T> && (!std::same_as<T, id_view>)
          && (!std::same_as<T, constant_value_tag>) && (!std::integral<T>)
          && (!std::floating_point<T>) && (!std::same_as<T, bool>)
          && (!std::is_pointer_v<std::remove_cvref_t<T>>)
id_view
to_id_view(T const& v)
{
    return make_id_by_reference(v);
}

template<class T>
id_view
to_id_view(erased_id_storage<T>&, T const& v)
{
    return to_id_view(v);
}

template<class A, class B>
id_view
to_id_view(
    erased_id_storage<std::pair<A, B>>& storage, std::pair<A, B> const& v)
{
    return make_id_pair(
        storage.pair,
        to_id_view(storage.left, v.first),
        to_id_view(storage.right, v.second));
}

// `typed_value_id<T>` is true iff `T` can be used as a concrete signal value
// ID. The name avoids colliding with the `value_id()` member function.
template<class T>
concept typed_value_id
    = std::copyable<T>
   && requires(erased_id_storage<T>& storage, T const& id) {
          { to_id_view(storage, id) } -> std::convertible_to<id_view>;
      };

} // namespace alia

// std::hash interface
namespace std {
template<>
struct hash<alia_id_view>
{
    size_t
    operator()(alia_id_view id) const noexcept
    {
        return static_cast<size_t>(alia_id_view_hash(id));
    }
};
} // namespace std
