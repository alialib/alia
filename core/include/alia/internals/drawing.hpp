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
    std::vector<alia_draw_material> materials;
};

struct alia_draw_bucket_table
{
    std::unordered_map<uint64_t, alia_draw_bucket> buckets;
    std::vector<uint64_t> keys;
};

} // extern "C"

namespace alia {

static inline uint64_t
make_bucket_key(alia_z_index z_index, alia_draw_material_id material_id)
{
    return (uint64_t(uint32_t(z_index)) << 32) | uint64_t(material_id);
}

} // namespace alia
