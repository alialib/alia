#pragma once

#include <alia/ui/layout/node.hpp>

#include <algorithm>

namespace alia {

template<class T>
T&
claim_scratch(LayoutScratchArena& arena)
{
    static_assert(std::is_trivially_destructible_v<T>);
    void* raw = arena.allocate(sizeof(T), alignof(T));
    return *std::construct_at(reinterpret_cast<T*>(raw));
}

template<class T>
T&
use_scratch(LayoutScratchArena& arena)
{
    void* raw = arena.allocate(sizeof(T), alignof(T));
    return *reinterpret_cast<T*>(raw);
}

template<class T>
T&
peek_scratch(LayoutScratchArena& arena)
{
    return *reinterpret_cast<T*>(arena.peek());
}

} // namespace alia
