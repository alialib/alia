#pragma once

#include <alia/abi/base/memory.h>
#include <alia/abi/ui/drawing/system.h>

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
    // pool ID for draw commands emitted during the draw pass
    alia_memory_pool_id command_pool;
};

struct alia_draw_bucket_table
{
    std::unordered_map<uint64_t, alia_draw_bucket> buckets;
    std::vector<uint64_t> keys;
};

} // extern "C"
