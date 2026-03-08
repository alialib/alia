#pragma once

#include <alia/abi/base/arena.h>
#include <alia/abi/ui/drawing.h>

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
    alia_vec2f surface_size;
    float dpi;
    alia_draw_material_id next_material_id;
    std::vector<alia_draw_material> materials;
};

typedef struct alia_draw_bucket
{
    alia_draw_command* head;
    alia_draw_command* tail;
    uint32_t count;
    // TODO: Add generalized, material-specific summary info.
    uint32_t instance_count;
} alia_draw_bucket;

struct alia_draw_bucket_table
{
    std::unordered_map<uint64_t, alia_draw_bucket> buckets;
    std::vector<uint64_t> keys;
};

} // extern "C"
