#pragma once

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/layout/utilities.h>
#include <alia/arena.hpp>

namespace alia {

// SCRATCH ARENA UTILITIES

// Claim scratch space from the arena - This is called by the node
// implementation during the horizontal measurement step of the layout process.
// Since that's the first step, the scratch space is assumed to be unused at
// that point, so this will invoke the default constructor for T.
// Note that no destructor is ever called, so T must be trivially destructible.
template<class T>
T&
claim_scratch(alia_arena_view& arena)
{
    return *arena_new<T>(arena);
}

// Use already-claimed scratch space from the arena - This is called by the
// node implementation during subsequent steps of the layout process to re-use
// the scratch space that was claimed and initialized during the horizontal
// measurement step.
template<class T>
T&
use_scratch(alia_arena_view& arena)
{
    return *arena_alloc<T>(arena);
}

} // namespace alia
