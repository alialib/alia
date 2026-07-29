#pragma once

#include <alia/abi/kernel/ids.h>

#include <concepts>
#include <functional>
#include <stdint.h>

namespace alia {

// This file defines a thin C++ facade over `alia_id_view`.

using id_view = alia_id_view;

inline id_view
null_id()
{
    return alia_id_view_null();
}

// `make_id(v)` for various common types - This takes care of selecting a
// compatible `alia_id_view` constructor based on the type.

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

// Construct an ID from a pointer. Use with caution! This should only be used
// for immutable objects where address is equivalent to identity (e.g., string
// literals).
template<class T>
id_view
make_pointer_id(T const* ptr)
{
    return alia_id_view_make_pointer(static_cast<void const*>(ptr));
}

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
