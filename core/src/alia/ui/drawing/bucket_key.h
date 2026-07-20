#pragma once

#include <alia/abi/ui/drawing/commands.h>
#include <alia/abi/ui/geometry.h>

#include <cstdint>

// draw bucket key layout: [target:16][z:16][clip:16][material:16]

inline uint64_t
make_bucket_key(
    alia_draw_target_id target_id,
    alia_z_index z_index,
    alia_clip_id clip_id,
    alia_draw_material_id material_id)
{
    return (uint64_t(target_id) << 48) | (uint64_t(z_index) << 32)
         | (uint64_t(clip_id) << 16) | uint64_t(material_id);
}

inline alia_draw_target_id
target_from_key(uint64_t key)
{
    return alia_draw_target_id(key >> 48);
}

inline alia_draw_material_id
material_from_key(uint64_t key)
{
    return alia_draw_material_id(key & 0xffff);
}
