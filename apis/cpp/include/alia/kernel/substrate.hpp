#pragma once

#include <alia/abi/kernel/substrate.h>

#include <new>
#include <optional>
#include <type_traits>
#include <utility>

// This file defines a thin C++ facade over the substrate C ABI.

namespace alia {

// general allocator associated with the substrate system on `ctx`
inline alia_general_allocator*
substrate_allocator(alia_context* ctx)
{
    return alia_substrate_system_allocator(alia_ctx_substrate_system(ctx));
}

// result of a typed substrate usage
template<class T>
struct use_result
{
    T* ptr = nullptr;
    alia_substrate_block_traversal_mode mode
        = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL;
    // Is `*ptr` newly created? For `use_memory` and `use_object`, this true
    // for any non-NORMAL traversal mode. For `use_cache`, it is also true when
    // the value was re-created after a cache clear (mode may still be NORMAL).
    bool freshly_constructed = false;

    // Is this usage visiting a freshly constructed value?
    bool
    is_fresh() const
    {
        return freshly_constructed;
    }

    // Is this the first persistent usage of the node?
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
concept cache_clearable = requires(T& t) { t.clear_cache(); };

template<class T>
void
object_cleanup(
    alia_substrate_system*, void* ptr, alia_substrate_cleanup_mode mode)
{
    auto* obj = static_cast<T*>(ptr);
    if (mode == ALIA_SUBSTRATE_CLEAR_CACHE)
    {
        if constexpr (cache_clearable<T>)
            obj->clear_cache();
        return;
    }
    obj->~T();
}

// storage for a regenerable cached value
template<class T>
struct cache_storage
{
    std::optional<T> value;

    void
    clear_cache()
    {
        value.reset();
    }
};

} // namespace detail

// Use raw memory for a `T` from the current substrate block. The caller is
// responsible for initializing fresh nodes.
template<class T>
use_result<T>
use_memory(alia_context* ctx)
{
    alia_substrate_usage_result usage
        = alia_substrate_use_memory(ctx, sizeof(T), alignof(T));
    bool const fresh = usage.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL;
    return {static_cast<T*>(usage.ptr), usage.mode, fresh};
}

// Use a persistent C++ object of type `T` from the current substrate block.
// Fresh nodes are default-constructed. On `ALIA_SUBSTRATE_DESTROY`, the
// destructor runs. On `ALIA_SUBSTRATE_CLEAR_CACHE`, `t.clear_cache()` is
// called if that member exists. Otherwise, the clear is a no-op and the
// object remains constructed.
template<class T>
use_result<T>
use_object(alia_context* ctx)
{
    static_assert(std::is_default_constructible_v<T>);

    alia_substrate_usage_result usage = alia_substrate_use_object(
        ctx, sizeof(T), alignof(T), detail::object_cleanup<T>);

    use_result<T> result{
        static_cast<T*>(usage.ptr),
        usage.mode,
        usage.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL};
    if (result.is_fresh())
        new (result.ptr) T();
    return result;
}

// Use a regenerable cached `T` from the current substrate block.
// The value is destroyed on cache clear and re-created on the next visit
// (`is_fresh()` is true when it was just constructed).
template<class T>
use_result<T>
use_cache(alia_context* ctx)
{
    static_assert(std::is_default_constructible_v<T>);

    alia_substrate_usage_result usage = alia_substrate_use_object(
        ctx,
        sizeof(detail::cache_storage<T>),
        alignof(detail::cache_storage<T>),
        detail::object_cleanup<detail::cache_storage<T>>);

    auto* storage = static_cast<detail::cache_storage<T>*>(usage.ptr);
    if (usage.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL)
        new (storage) detail::cache_storage<T>();

    bool const fresh = !storage->value.has_value();
    if (fresh)
        storage->value.emplace();

    return {&*storage->value, usage.mode, fresh};
}

// Use an anchor from the current substrate block.
inline alia_substrate_anchor*
use_anchor(alia_context* ctx)
{
    return alia_substrate_use_anchor(ctx);
}

} // namespace alia
