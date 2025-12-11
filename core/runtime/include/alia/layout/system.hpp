#pragma once

#include <alia/arenas.hpp>
#include <alia/geometry.hpp>
#include <alia/internals/scratch.hpp>
#include <alia/layout/container.hpp>

namespace alia {

struct layout_system
{
    lazy_commit_arena_allocator allocator;
    alia_scratch_arena node_arena;
    alia_scratch_arena scratch_arena;
    layout_container root;
    alia_scratch_arena placement_arena;
};

void
initialize(layout_system& system);

void
resolve_layout(layout_system& system, vec2 available_space);

} // namespace alia
