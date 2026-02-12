#pragma once

#include <alia/abi/base/stack.h>
#include <alia/abi/context.h>

#include <alia/prelude.hpp>

namespace alia {

template<typename T>
T&
stack_push(alia_context* ctx)
{
    static_assert(
        std::is_trivially_destructible<T>::value,
        "T must be trivially destructible");
    static_assert(
        std::is_standard_layout<T>::value, "T must be standard layout");
    void* ptr;
    if constexpr (alignof(T) % ALIA_MIN_ALIGN == 0)
    {
        ptr = alia_stack_push(ctx->stack, sizeof(T), nullptr);
    }
    else
    {
        ptr = alia_stack_push_aligned(
            ctx->stack, sizeof(T), alignof(T), nullptr);
    }
    return *(new (ptr) T);
}

template<typename T>
T&
stack_pop(alia_context* ctx)
{
    T* ptr = reinterpret_cast<T*>(alia_stack_peek_payload(ctx->stack));
    alia_stack_pop(ctx->stack);
    return *ptr;
}

} // namespace alia
