#include <alia/abi/ui/library.h>

#include <alia/abi/ui/context.h>
#include <alia/abi/ui/drawing/primitives.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/palette.h>
#include <alia/impl/events.hpp>

using namespace alia;

ALIA_EXTERN_C_BEGIN

void
alia_separator_style_generate(
    alia_separator_style* out, alia_style_seeds const* seeds)
{
    alia_style_seeds const s = seeds ? *seeds : alia_style_seeds_default();
    *out = alia_separator_style{
        .color = alia_palette_color_make(
            alia_palette_index_foundation_ramp(
                ALIA_PALETTE_FOUNDATION_RAMP_STRUCTURAL,
                ALIA_PALETTE_RAMP_LEVEL_BASE),
            0xff),
        .thickness = 2.f * s.scale,
    };
}

void
alia_separator(alia_context* ctx, alia_layout_flags_t layout_flags)
{
    ALIA_ASSERT((layout_flags & ALIA_DEFAULT_CROSS_ALIGNMENT_MASK) == 0);
    layout_flags |= ALIA_DEFAULT_FILL_CROSS;

    alia_separator_style const* const style = alia_separator_style_active(ctx);
    float const thickness = alia_px(ctx, style->thickness);

    alia_event_category const category = get_event_category(*ctx);
    if (category == ALIA_CATEGORY_REFRESH)
    {
        alia_layout_leaf_emit(
            ctx,
            alia_layout_content_metrics_make(
                alia_vec2f{thickness, thickness}),
            layout_flags);
        return;
    }

    alia_box const box = alia_layout_consume_box(ctx);

    if (category == ALIA_CATEGORY_DRAWING)
    {
        alia_srgba8 const color = alia_palette_color_resolve(
            alia_ctx_palette(ctx), style->color);
        alia_draw_box(
            ctx,
            ctx->geometry->z_base + 1,
            box,
            {.fill_color = color,
             .corner_radius = 0.f,
             .border_width = 0.f,
             .border_color = color});
    }
}

ALIA_EXTERN_C_END
