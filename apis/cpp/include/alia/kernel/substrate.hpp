#pragma once

#include <alia/abi/kernel/substrate.h>

#include <new>
#include <type_traits>
#include <utility>

// This file defines a thin C++ facade over the substrate C ABI.

namespace alia {

// result of a typed substrate usage
template<class T>
struct use_result
{
    T* ptr = nullptr;
    alia_substrate_block_traversal_mode mode
        = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL;

    // Is this usage visiting a newly allocated node?
    bool
    is_fresh() const
    {
        return mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL;
    }

    // Is this the first *persistent* usage of a newly allocated node?
    bool
    is_init() const
    {
        return mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT;
    }

    T&
    operator*() const
    {
        return *ptr;
    }

    T*
    operator->() const
    {
        return ptr;
    }
};

namespace detail {

template<class T>
void
object_cleanup(alia_substrate_system*, void* ptr, alia_substrate_cleanup_mode)
{
    static_cast<T*>(ptr)->~T();
}

} // namespace detail

// Use raw memory for a `T` from the current substrate block. The caller is
// responsible for initializing fresh nodes.
template<class T>
use_result<T>
use_memory(alia_context* ctx)
{
    alia_substrate_usage_result usage
        = alia_substrate_use_memory(ctx, sizeof(T), alignof(T));
    return {static_cast<T*>(usage.ptr), usage.mode};
}

// Use a C++ object of type `T` from the current substrate block. Fresh nodes
// are default-constructed. Destruction is registered with the substrate.
template<class T>
use_result<T>
use_object(alia_context* ctx)
{
    static_assert(std::is_default_constructible_v<T>);

    alia_substrate_usage_result usage = alia_substrate_use_object(
        ctx, sizeof(T), alignof(T), detail::object_cleanup<T>);

    use_result<T> result{static_cast<T*>(usage.ptr), usage.mode};
    if (result.is_fresh())
        new (result.ptr) T();
    return result;
}

// Use an anchor from the current substrate block.
inline alia_substrate_anchor*
use_anchor(alia_context* ctx)
{
    return alia_substrate_use_anchor(ctx);
}

} // namespace alia
