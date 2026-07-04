#pragma once

#include <alia/abi/base/arena.h>
#include <alia/abi/ui/drawing.h>

#include <alia/base/arena.h>

#include <unordered_map>
#include <vector>

extern "C" {

struct alia_draw_material
{
    alia_material_vtable vtable;
    void* user;
};

struct alia_draw_system
{
    alia_draw_material_id next_material_id;
    std::vector<alia_draw_material> materials;
    // Per-frame bump storage for draw commands emitted during the draw pass.
    alia_arena command_arena;
};

struct alia_draw_bucket_table
{
    std::unordered_map<uint64_t, alia_draw_bucket> buckets;
    std::vector<uint64_t> keys;
};

} // extern "C"
