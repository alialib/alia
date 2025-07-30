#pragma once

#include <alia/ui/layout/node.hpp>

namespace alia {

template<class T>
T&
claim_scratch(InfiniteArena& arena)
{
    return *arena_new<T>(arena);
}

template<class T>
T&
use_scratch(InfiniteArena& arena)
{
    return *arena_alloc<T>(arena);
}

template<class T>
T&
peek_scratch(InfiniteArena& arena)
{
    return *reinterpret_cast<T*>(arena.peek());
}

} // namespace alia
