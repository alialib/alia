#pragma once

#include <alia/abi/base/memory.h>
#include <alia/abi/ui/regions.h>

namespace alia {

// durable region tree owned by `alia_ui_system` - The tree lives in `pool`
// after each REGIONS pass and remains valid until the next rebuild.
struct regions_state
{
    alia_memory_pool_id pool = 0;
    alia_region* root = nullptr;
};

// Append CLIP/TRANSLATE nodes under the current regions-pass parent. Callers
// must only invoke these when `alia_ctx_regions` is set.
void
regions_push_clip(alia_ui_context* ctx, alia_box absolute_clip);

void
regions_pop_clip(alia_ui_context* ctx);

void
regions_push_translation(alia_ui_context* ctx, alia_vec2f delta);

void
regions_pop_translation(alia_ui_context* ctx);

} // namespace alia
