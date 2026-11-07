#include <alia/abi/ui/library.h>

#include <alia/abi/base/geometry.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/drawing/primitives.h>
#include <alia/abi/ui/geometry/api.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/styling/environment.h>
#include <alia/abi/ui/styling/paint.h>
#include <alia/abi/ui/text.h>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>
#include <alia/impl/ui/styling/color_program.hpp>

namespace alia {
using namespace color;

struct button_scope
{
    alia_element_id id{};
    alia_button_flags_t flags = 0;
    alia_box box{};
};

static void
button_style_finish_emit(
    alia_button_style* style, alia_color_program_emitter* emitter)
{
    style->steps = alia_color_program_emitter_steps(emitter);
}

// Build the accent seed from recipe H/C channel sources. Starts from a
// palette slot for L scaffolding, then overlays H and C as requested.
static color_program_expr
button_accent_seed(alia_button_recipe const& recipe)
{
    alia_style_color_slot base_slot = ALIA_STYLE_COLOR_PRIMARY;
    if (recipe.hue.source == ALIA_STYLE_CHANNEL_SLOT)
        base_slot = recipe.hue.slot;
    else if (recipe.chroma.source == ALIA_STYLE_CHANNEL_SLOT)
        base_slot = recipe.chroma.slot;

    color_program_expr e = sample(base_slot);

    if (recipe.hue.source == ALIA_STYLE_CHANNEL_VALUE)
        e = e | set_hue(recipe.hue.value);
    else if (
        recipe.hue.source == ALIA_STYLE_CHANNEL_SLOT
        && recipe.hue.slot != base_slot)
        e = e | match_hue(recipe.hue.slot);

    if (recipe.chroma.source == ALIA_STYLE_CHANNEL_VALUE)
        e = e | set_chroma(recipe.chroma.value);
    else if (
        recipe.chroma.source == ALIA_STYLE_CHANNEL_SLOT
        && recipe.chroma.slot != base_slot)
        e = e | match_chroma(recipe.chroma.slot);

    return e;
}

// Apply filled/gradient tone placement onto an accent seed.
static color_program_expr
button_accent_toned(color_program_expr seed, color::tone t)
{
    switch (t)
    {
        case tone::dark:
            return seed | set_lightness_float(ALIA_STYLE_VAR_DARK_L)
                 | scale_chroma_float(ALIA_STYLE_VAR_ABSOLUTE_CHROMA);
        case tone::light:
            return seed | set_lightness_float(ALIA_STYLE_VAR_LIGHT_L)
                 | scale_chroma_float(ALIA_STYLE_VAR_ABSOLUTE_CHROMA);
        case tone::inverse:
            return seed | match_lightness(surface)
                 | add_lightness_delta_float(
                       surface, text, ALIA_STYLE_VAR_INVERSE_STEP)
                 | scale_chroma_float(ALIA_STYLE_VAR_INVERSE_CHROMA);
        case tone::match:
        default:
            return seed | match_lightness(surface)
                 | add_lightness_delta_float(
                       surface, text, ALIA_STYLE_VAR_MATCH_STEP)
                 | scale_chroma_float(ALIA_STYLE_VAR_MATCH_CHROMA);
    }
}

// Black/white on-color against a toned accent fill.
static color_program_expr
button_accent_on_color(
    color_program_expr toned_fill, float contrast_scale = 1.5f)
{
    color_program_expr e = toned_fill;
    e.push_step(alia_color_step_dup_color());
    emit_set_channel_immediate(e, ALIA_COLOR_CHANNEL_C, 0.f);
    emit_set_channel_immediate(e, ALIA_COLOR_CHANNEL_L, 1.f);
    e.push_step(alia_color_step_swap_color());
    emit_load_var(e, ALIA_STYLE_VAR_CONTRAST_TARGET);
    emit_immediate(e, contrast_scale);
    e.push_step(alia_color_step_mul());
    e.push_step(alia_color_step_contrast_ex(
        ALIA_COLOR_L_AUTO, ALIA_COLOR_CONTRAST_REQUIRE));
    return e;
}

static void
button_style_apply_filled(
    alia_button_style* style,
    alia_button_recipe const& recipe,
    alia_color_program_emitter* emitter)
{
    color::tone const t = (color::tone) recipe.tone;
    color_program_expr const fill
        = button_accent_toned(button_accent_seed(recipe), t);

    style->paint.fill = emit_responsive(
        emitter,
        {.rest = fill,
         .hovered = color_program_expr{}
                  | strengthen_var(ALIA_STYLE_VAR_HOVER_STRENGTHEN),
         .active = color_program_expr{}
                 | strengthen_var(ALIA_STYLE_VAR_ACTIVE_STRENGTHEN),
         .disabled = sample(well)});

    style->paint.border = emit_responsive(emitter, {.rest = fill});
    style->paint.fill_end = style->paint.fill;
    style->paint.border_end = style->paint.border;
    style->paint.gradient = false;
    style->paint.angle = 0.f;
    style->paint.border_width = 0.f;

    style->label = emit_responsive(
        emitter,
        {.rest = button_accent_on_color(fill), .disabled = sample(text)});

    button_style_finish_emit(style, emitter);
}

static void
button_style_apply_outline(
    alia_button_style* style,
    alia_button_recipe const& recipe,
    alia_style_seeds const& seeds,
    alia_color_program_emitter* emitter)
{
    color_program_expr const accent = button_accent_seed(recipe);

    // Freeze H/C on fill rest so hover/active alpha washes keep the recipe
    // accent rather than drifting with a live role sample alone.
    style->paint.fill = emit_responsive(
        emitter,
        {.rest = accent | set_alpha(0.f),
         .hovered = color_program_expr{} | set_alpha(0x20 / 255.f),
         .active = color_program_expr{} | set_alpha(0x40 / 255.f),
         .disabled = color_program_expr{} | set_alpha(0.f)});

    style->paint.border = emit_responsive(
        emitter,
        {.rest = accent | scale_chroma(0.8f), .disabled = sample(well)});

    style->paint.fill_end = style->paint.fill;
    style->paint.border_end = style->paint.border;
    style->paint.gradient = false;
    style->paint.angle = 0.f;
    style->paint.border_width = seeds.outline_width * seeds.scale;

    style->label = emit_responsive(
        emitter,
        {.rest = accent | scale_chroma(0.7f)
               | ensure_contrast_var(
                     surface,
                     ALIA_STYLE_VAR_CONTRAST_TARGET,
                     1.5f,
                     ALIA_COLOR_L_AUTO,
                     ALIA_COLOR_CONTRAST_PREFER),
         .disabled = sample(text)});

    button_style_finish_emit(style, emitter);
}

static void
button_style_apply_gradient(
    alia_button_style* style,
    alia_button_recipe const& recipe,
    alia_color_program_emitter* emitter)
{
    color::tone const t = (color::tone) recipe.tone;
    color_program_expr const base
        = button_accent_toned(button_accent_seed(recipe), t);

    responsive_color_expr const fill_expr{
        .rest = base | shift_hue(2) | darken(0.02f),
        .hovered = color_program_expr{}
                 | strengthen_var(ALIA_STYLE_VAR_HOVER_STRENGTHEN),
        .active = color_program_expr{}
                | strengthen_var(ALIA_STYLE_VAR_ACTIVE_STRENGTHEN),
        .disabled = sample(well)};
    responsive_color_expr const fill_end_expr{
        .rest = base | shift_hue(-2) | lighten(0.02f),
        .hovered = color_program_expr{}
                 | strengthen_var(ALIA_STYLE_VAR_HOVER_STRENGTHEN),
        .active = color_program_expr{}
                | strengthen_var(ALIA_STYLE_VAR_ACTIVE_STRENGTHEN),
        .disabled = sample(well)};

    style->paint.fill = emit_responsive(emitter, fill_expr);
    style->paint.fill_end = emit_responsive(emitter, fill_end_expr);
    style->paint.border = style->paint.fill;
    style->paint.border_end = style->paint.fill_end;
    style->paint.gradient = true;
    style->paint.angle = 90.f;
    style->paint.border_width = 0.f;

    style->label = emit_responsive(
        emitter,
        {.rest = button_accent_on_color(base), .disabled = sample(text)});

    button_style_finish_emit(style, emitter);
}

static void
button_style_apply_recipe(
    alia_button_style* style,
    alia_button_recipe const& recipe,
    alia_style_seeds const& seeds,
    alia_color_program_emitter* emitter)
{
    if (recipe.chrome == ALIA_BUTTON_CHROME_OUTLINE)
        button_style_apply_outline(style, recipe, seeds, emitter);
    else if (recipe.chrome == ALIA_BUTTON_CHROME_GRADIENT)
        button_style_apply_gradient(style, recipe, emitter);
    else
        button_style_apply_filled(style, recipe, emitter);
}

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

alia_button_recipe
alia_button_recipe_default(void)
{
    alia_button_recipe recipe{};
    recipe.chrome = ALIA_BUTTON_CHROME_FILLED;
    recipe.tone = ALIA_COLOR_TONE_MATCH;
    recipe.hue = alia_style_channel_from_slot(ALIA_STYLE_COLOR_PRIMARY);
    recipe.chroma = alia_style_channel_from_slot(ALIA_STYLE_COLOR_PRIMARY);
    return recipe;
}

void
alia_button_style_generate(
    alia_button_style* out,
    alia_style_seeds const* seeds,
    alia_color_program_emitter* emitter)
{
    ALIA_ASSERT(out);
    ALIA_ASSERT(emitter);
    alia_style_seeds const s = seeds ? *seeds : alia_style_seeds_default();
    *out = alia_button_style{};
    out->padding_x = s.spacing * 1.4f * s.scale;
    out->padding_y = s.spacing * 0.7f * s.scale;
    out->corner_radius = 6.f * s.scale * s.roundness;
    alia_button_recipe const recipe = alia_button_recipe_default();
    button_style_apply_recipe(out, recipe, s, emitter);
}

void
alia_button_style_apply_recipe(
    alia_button_style* style,
    alia_button_recipe const* recipe,
    alia_style_seeds const* seeds,
    alia_color_program_emitter* emitter)
{
    ALIA_ASSERT(style);
    ALIA_ASSERT(recipe);
    ALIA_ASSERT(seeds);
    ALIA_ASSERT(emitter);
    button_style_apply_recipe(style, *recipe, *seeds, emitter);
}

alia_button_result_t
alia_button_begin(
    alia_ui_context* ctx,
    alia_button_flags_t flags,
    alia_layout_flags_t layout_flags)
{
    alia_substrate_usage_result const usage
        = alia_substrate_use_memory(alia_ctx_kernel(ctx), 1, 1);
    alia_element_id const id
        = alia_make_element_id(alia_ctx_kernel(ctx), usage);

    alia_button_style const* const style = alia_button_style_active(ctx);
    bool const is_disabled = (flags & ALIA_BUTTON_DISABLED) != 0;
    alia_button_result_t pass_result = ALIA_BUTTON_RESULT_NONE;

    auto& scope = stack_push<button_scope>(alia_ctx_kernel(ctx));
    scope = button_scope{
        .id = id,
        .flags = flags,
        .box = {},
    };

    // Default LEFT/TOP so content-sized buttons do not stretch in columns
    // (containers fill when no alignment is set). Callers can still pass FILL.
    // Alignment and GROW live on an outer override; edge_offsets stay pure
    // insets (margin = leaf spacing, padding = chrome, PROVIDE_BOX on
    // padding).
    ALIA_ASSERT((layout_flags & ALIA_DEFAULT_X_ALIGNMENT_MASK) == 0);
    ALIA_ASSERT((layout_flags & ALIA_DEFAULT_Y_ALIGNMENT_MASK) == 0);
    layout_flags |= ALIA_DEFAULT_ALIGN_LEFT | ALIA_DEFAULT_ALIGN_TOP;
    float const layout_spacing
        = (layout_flags & ALIA_FLUSH) != 0 ? 0.f : alia_spacing_active(ctx);
    alia_edge_offsets const margin
        = alia_edge_offsets_make_uniform(layout_spacing);
    alia_edge_offsets const padding
        = alia_edge_offsets_make_xy(style->padding_x, style->padding_y);

    alia_layout_alignment_override_begin(ctx, layout_flags);
    alia_layout_edge_offsets_begin(ctx, margin, 0);
    alia_layout_edge_offsets_begin(ctx, padding, ALIA_PROVIDE_BOX);
    // Row keeps label/content as one flow fragment so PROVIDE_BOX is a single
    // box (edge_offsets alone pass children through to the outer flow).
    alia_layout_row_begin(ctx, 0, 0.f);

    if (!is_refresh_event(*ctx))
    {
        alia_layout_box_array const boxes = alia_layout_consume_box_array(ctx);
        scope.box = boxes.count > 0 ? boxes.boxes[0] : alia_box{};

        alia_interaction_status_t const interaction_status
            = alia_element_get_interaction_status(
                ctx, id, is_disabled ? ALIA_INTERACTION_STATUS_DISABLED : 0);

        alia_text_color_push(
            ctx,
            alia_responsive_color_program_resolve(
                alia_ctx_style(ctx)->environment,
                style->steps,
                &style->label,
                interaction_status));

        alia_event_category const category = get_event_category(*ctx);
        switch (category)
        {
            case ALIA_CATEGORY_INPUT: {
                if (is_disabled)
                    break;
                // TODO: Wire focus order / keyboard activate once set_focus
                // is implemented. Honor ALIA_BUTTON_SKIP_FOCUS when that
                // lands.
                if (alia_element_detect_click(ctx, id, ALIA_BUTTON_LEFT))
                    pass_result = ALIA_BUTTON_RESULT_ACTIVATED;
                else if (alia_element_detect_click(ctx, id, ALIA_BUTTON_RIGHT))
                    pass_result = ALIA_BUTTON_RESULT_CONTEXT;
                break;
            }
            case ALIA_CATEGORY_DRAWING: {
                alia_style_environment const* env
                    = alia_ctx_style(ctx)->environment;
                float const corner = alia_px(ctx, style->corner_radius);
                alia_z_index const z = alia_z(ctx, ALIA_Z_PLANE_BACKGROUND);
                alia_resolved_box_paint const paint
                    = alia_responsive_box_paint_program_resolve(
                        env,
                        style->steps,
                        &style->paint,
                        interaction_status,
                        alia_ctx_geometry(ctx)->scale);
                alia_draw_box(
                    ctx,
                    z,
                    scope.box,
                    {.fill_color = paint.fill,
                     .corner_radius = corner,
                     .border_width = paint.border_width,
                     .border_color = paint.border,
                     .fill_end_color = paint.fill_end,
                     .border_end_color = paint.border_end,
                     .gradient = paint.gradient,
                     .gradient_angle = paint.angle});
                break;
            }
            default:
                break;
        }
    }

    return pass_result;
}

void
alia_button_end(alia_ui_context* ctx)
{
    alia_layout_row_end(ctx);
    alia_layout_edge_offsets_end(ctx);
    alia_layout_edge_offsets_end(ctx);
    alia_layout_alignment_override_end(ctx);

    if (!is_refresh_event(*ctx))
        alia_text_color_pop(ctx);

    auto scope = stack_pop<button_scope>(alia_ctx_kernel(ctx));

    if (is_refresh_event(*ctx))
        return;

    bool const is_disabled = (scope.flags & ALIA_BUTTON_DISABLED) != 0;

    // Register after children so the button owns the hit (last writer wins).
    alia_element_box_region(
        ctx,
        scope.id,
        &scope.box,
        ALIA_Z_PLANE_CONTENT,
        ALIA_HIT_TEST_MOUSE,
        is_disabled ? ALIA_CURSOR_DEFAULT : ALIA_CURSOR_POINTER);
}

ALIA_EXTERN_C_END
