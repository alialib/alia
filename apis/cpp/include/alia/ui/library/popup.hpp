#pragma once

#include <alia/abi/ui/geometry/api.h>
#include <alia/abi/ui/layout/detached.h>
#include <alia/abi/ui/layout/popup.h>
#include <alia/abi/ui/system/api.h>
#include <alia/context.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/substrate.hpp>

#include <utility>

namespace alia {

// Emit `content` as a detached popup anchored to `anchor`.
// `anchor` is a layout box from this frame. Call this only while the popup
// should be open. On non-refresh passes this measures the content, places it
// within the surface, assigns that size, resets the clip to the surface, and
// raises the z-order by one layer.
// `min_size` is a floor in the same space as `anchor`. A non-positive
// component means no floor on that axis.
template<class Content>
void
popup(ui_context& ctx, alia_box anchor, alia_vec2f min_size, Content&& content)
{
    auto data = use_memory<alia_layout_detached>(alia_ctx_kernel(&ctx));
    if (data.is_fresh())
        *data = alia_layout_detached{};

    if (is_refresh_event(ctx))
    {
        alia_layout_detached_begin(&ctx, data.ptr);
        std::forward<Content>(content)();
        alia_layout_detached_end(&ctx);
        return;
    }

    ALIA_ASSERT(alia_ctx_system(&ctx));
    ALIA_ASSERT(alia_ctx_scratch(alia_ctx_kernel(&ctx))->arena);

    alia_vec2f const offset = alia_ctx_geometry(&ctx)->offset;
    alia_box const anchor_surface = alia_box_translate(anchor, offset);
    alia_box const bounds = alia_box_make(
        alia_vec2f_make(0.f, 0.f),
        alia_vec2i_to_vec2f(alia_ui_surface_get_size(alia_ctx_system(&ctx))));
    alia_vec2f const preferred = alia_layout_detached_measure(&ctx, data.ptr);
    alia_box const placed
        = alia_popup_place(anchor_surface, preferred, min_size, bounds);
    alia_layout_detached_assign(&ctx, data.ptr, placed.size);

    alia_geometry_push_clip_box(&ctx, bounds);
    alia_geometry_push_z_offset(&ctx, ALIA_Z_LAYER_SPACING);
    alia_geometry_push_translation(&ctx, alia_vec2f_sub(placed.min, offset));

    alia_layout_detached_begin(&ctx, data.ptr);
    std::forward<Content>(content)();
    alia_layout_detached_end(&ctx);

    alia_geometry_pop_translation(&ctx);
    alia_geometry_pop_z(&ctx);
    alia_geometry_pop_clip_box(&ctx);
}

// Emit `content` as a detached popup anchored to `anchor`.
// The popup is at least as wide as `anchor`. There is no minimum height.
template<class Content>
void
popup(ui_context& ctx, alia_box anchor, Content&& content)
{
    popup(
        ctx,
        anchor,
        alia_vec2f_make(anchor.size.x, 0.f),
        std::forward<Content>(content));
}

} // namespace alia
