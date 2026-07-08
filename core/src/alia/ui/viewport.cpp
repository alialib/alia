#include <alia/abi/ui/viewport.h>

#include <alia/abi/kernel/routing.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/geometry.h>
#include <alia/abi/ui/layout/api.h>

#include <alia/impl/events.hpp>

using namespace alia::operators;

namespace alia {

struct viewport_data
{
    uint32_t reserved;
};

static alia_viewport_style const default_viewport_style = {
    .min_size = {40.f, 40.f},
};

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

alia_viewport_style const*
alia_default_viewport_style(void)
{
    return &default_viewport_style;
}

alia_gl_viewport
alia_viewport_region_to_gl_viewport(alia_box region, alia_vec2f surface_size)
{
    return ALIA_BRACED_INIT(
        alia_gl_viewport,
        (int) (region.min.x + 0.5f),
        (int) (surface_size.y - (region.min.y + region.size.y) + 0.5f),
        (int) (region.size.x + 0.5f),
        (int) (region.size.y + 0.5f));
}

alia_element_id
alia_do_viewport(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    alia_layout_flags_t layout_flags,
    alia_viewport_style const* style)
{
    alia_substrate_usage_result result = alia_substrate_use_memory(
        ctx, sizeof(viewport_data), alignof(viewport_data));
    viewport_data* data = (viewport_data*) result.ptr;
    if (result.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
        data->reserved = 0;

    alia_element_id const id = alia_make_element_id(ctx, result);

    alia_viewport_style const* const effective_style
        = style != nullptr ? style : &default_viewport_style;

    alia_event_category const category = get_event_category(*ctx);
    if (category == ALIA_CATEGORY_REFRESH)
    {
        alia_layout_leaf_emit(
            ctx,
            alia_layout_content_metrics_make(alia_vec2f{
                alia_px(ctx, effective_style->min_size.x),
                alia_px(ctx, effective_style->min_size.y)}),
            layout_flags);
        return id;
    }

    alia_box const box = alia_layout_consume_box(ctx);

    switch (category)
    {
        case ALIA_CATEGORY_SPATIAL:
        case ALIA_CATEGORY_INPUT:
            break;

        case ALIA_CATEGORY_DRAWING: {
            alia_viewport_draw_command* command
                = (alia_viewport_draw_command*) alia_draw_command_alloc(
                    ctx,
                    z_index,
                    material_id,
                    ALIA_MIN_ALIGNED_SIZE(sizeof(alia_viewport_draw_command)));
            command->region = box;
            command->scene_to_surface = alia_affine2_identity();
            break;
        }
    }

    return id;
}

ALIA_EXTERN_C_END
