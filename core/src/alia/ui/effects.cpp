#include <alia/abi/ui/effects.h>

#include <alia/abi/base/arena.h>
#include <alia/abi/kernel/routing.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/geometry.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/system/renderer.h>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/events.hpp>
#include <alia/ui/drawing.h>
#include <alia/ui/system/object.h>

#include <cstring>

using namespace alia::operators;

namespace alia {

struct effect_data
{
    uint32_t reserved;
};

static alia_effect_style const default_effect_style = {
    .min_size = {40.f, 40.f},
};

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

alia_effect_style const*
alia_default_effect_style(void)
{
    return &default_effect_style;
}

int
alia_ui_register_effect(
    alia_ui_system* ui,
    alia_effect_desc const* desc,
    alia_draw_material_id* out_material_id)
{
    ALIA_ASSERT(ui);
    ALIA_ASSERT(desc);
    ALIA_ASSERT(out_material_id);
    if (!ui->renderer.register_effect)
        return -1;
    return ui->renderer.register_effect(
        ui->renderer.user, desc, out_material_id);
}

void
alia_draw_effect(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    alia_box region,
    void const* params,
    size_t params_size)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->draw);
    ALIA_ASSERT(params_size == 0 || params != nullptr);
    ALIA_ASSERT(params_size <= UINT16_MAX);

    alia_effect_draw_command* command
        = (alia_effect_draw_command*) alia_draw_command_alloc(
            ctx,
            z_index,
            material_id,
            ALIA_MIN_ALIGNED_SIZE(sizeof(alia_effect_draw_command)));

    command->region = alia_box_translate(region, ctx->geometry->offset);
    command->params_size = (uint16_t) params_size;
    command->params = nullptr;

    if (params_size > 0)
    {
        void* blob = alia_arena_ptr(
            &ctx->draw->arena,
            alia_arena_alloc(&ctx->draw->arena, params_size));
        std::memcpy(blob, params, params_size);
        command->params = blob;
    }
}

alia_element_id
alia_do_effect(
    alia_context* ctx,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    void const* params,
    size_t params_size,
    alia_layout_flags_t layout_flags,
    alia_effect_style const* style)
{
    alia_substrate_usage_result result = alia_substrate_use_memory(
        ctx, sizeof(effect_data), alignof(effect_data));
    effect_data* data = (effect_data*) result.ptr;
    if (result.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
        data->reserved = 0;

    alia_element_id const id = alia_make_element_id(ctx, result);

    alia_effect_style const* const effective_style
        = style != nullptr ? style : &default_effect_style;

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
            alia_effect_draw_command* command
                = (alia_effect_draw_command*) alia_draw_command_alloc(
                    ctx,
                    z_index,
                    material_id,
                    ALIA_MIN_ALIGNED_SIZE(sizeof(alia_effect_draw_command)));
            command->region = box;
            command->params_size = (uint16_t) params_size;
            command->params = nullptr;
            if (params_size > 0)
            {
                ALIA_ASSERT(params != nullptr);
                ALIA_ASSERT(params_size <= UINT16_MAX);
                void* blob = alia_arena_ptr(
                    &ctx->draw->arena,
                    alia_arena_alloc(&ctx->draw->arena, params_size));
                std::memcpy(blob, params, params_size);
                command->params = blob;
            }
            break;
        }
    }

    return id;
}

ALIA_EXTERN_C_END
