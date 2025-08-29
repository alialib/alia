#pragma once

#include <type_traits>

namespace alia {

#define ALIA_ASSERT(expr) assert(expr)

template<typename T>
T*
align_ptr(T* ptr, std::size_t a)
{
    return reinterpret_cast<T*>(
        (reinterpret_cast<std::uintptr_t>(ptr) + a - 1) & ~(a - 1));
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

} // namespace alia
