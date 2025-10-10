#pragma once

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace alia {

#define ALIA_ASSERT(expr) assert(expr)

template<typename T>
T*
align_ptr(T* ptr, std::size_t align)
{
    return reinterpret_cast<T*>(
        (reinterpret_cast<std::uintptr_t>(ptr) + align - 1) & ~(align - 1));
}

inline std::size_t
align_offset(std::size_t offset, std::size_t align)
{
    return (offset + align - 1) & ~(align - 1);
}

template<typename Outer, typename Inner>
Outer*
downcast(Inner* inner_ptr)
{
    static_assert(
        std::is_same<decltype(Outer::base), Inner>::value,
        "downcast: Outer::base must be of type Inner");
    static_assert(
        offsetof(Outer, base) == 0,
        "downcast: Outer must embed `base` at offset 0");
    return reinterpret_cast<Outer*>(inner_ptr);
}

template<typename Outer, typename Inner>
Outer const*
downcast(Inner const* inner_ptr)
{
    static_assert(
        std::is_same<decltype(Outer::base), Inner>::value,
        "downcast: Outer::base must be of type Inner");
    static_assert(
        offsetof(Outer, base) == 0,
        "downcast: Outer must embed `base` at offset 0");
    return reinterpret_cast<Outer const*>(inner_ptr);
}

template<typename Inner, typename Outer>
Inner*
upcast(Outer* outer_ptr)
{
    static_assert(
        std::is_same<decltype(Outer::base), Inner>::value,
        "upcast: Outer::base must be of type Inner");
    static_assert(
        offsetof(Outer, base) == 0,
        "upcast: Outer must embed `base` at offset 0");
    return &outer_ptr->base;
}

template<typename Inner, typename Outer>
Inner const*
upcast(Outer const* outer_ptr)
{
    static_assert(
        std::is_same<decltype(Outer::base), Inner>::value,
        "upcast: Outer::base must be of type Inner");
    static_assert(
        offsetof(Outer, base) == 0,
        "upcast: Outer must embed `base` at offset 0");
    return &outer_ptr->base;
}

typedef int64_t nanosecond_count;

constexpr nanosecond_count
milliseconds(uint32_t ms)
{
    return nanosecond_count(ms * 1'000'000);
}

} // namespace alia
