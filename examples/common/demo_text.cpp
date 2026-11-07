#include "demo_text.hpp"

#include <alia/abi/base/arena.h>
#include <alia/abi/prelude.h>
#include <alia/base/flags.hpp>
#include <alia/impl/ui/styling/color_program.hpp>

#include "alia_fonts.h"

namespace {

demo_fonts g_fonts{};
alia_app* g_app = nullptr;

void
fill_resolved_font(
    alia_ui_system* ui,
    alia_typeface_id typeface,
    float size,
    alia_resolved_font* out)
{
    alia_resolved_typeface const resolved
        = alia_typeface_resolve(ui, typeface);
    ALIA_ASSERT(
        resolved.engine && resolved.engine->vtable
        && resolved.engine->vtable->get_font_metrics);
    out->typeface = resolved;
    out->size = size;
    resolved.engine->vtable->get_font_metrics(
        resolved.engine, resolved.engine_handle, size, &out->metrics);
}

void
refresh_resolved_fonts(size_t body_index, size_t heading_index, float scale)
{
    ALIA_ASSERT(g_app);
    alia_ui_system* ui = alia_app_ui(g_app);
    ALIA_ASSERT(ui);
    ALIA_ASSERT(body_index < alia_app_typeface_count(g_app));
    ALIA_ASSERT(heading_index < alia_app_typeface_count(g_app));

    fill_resolved_font(
        ui,
        alia_app_typeface(g_app, body_index),
        14.f * scale,
        &g_fonts.body_14);
    fill_resolved_font(
        ui,
        alia_app_typeface(g_app, heading_index),
        14.f * scale,
        &g_fonts.heading_14);
    fill_resolved_font(
        ui,
        alia_app_typeface(g_app, heading_index),
        18.f * scale,
        &g_fonts.heading_18);
}

alia_srgba8
resolve_expr(
    alia_style_environment const* env,
    alia::color::color_program_expr const& expr)
{
    alignas(alia_color_program_step) unsigned char storage[256];
    alia_bump_allocator bump;
    alia_bump_allocator_bind(
        &bump, storage, sizeof(storage), alia_grow_callback_none());
    alia_color_program_emitter emitter
        = alia_color_program_emitter_begin(&bump);
    alia_color_program_ref const ref = alia::color::emit(&emitter, expr);
    return alia_color_program_resolve(
        env, alia_color_program_emitter_steps(&emitter), ref);
}

} // namespace

demo_fonts const&
demo_get_fonts()
{
    return g_fonts;
}

void
demo_setup_fonts(alia_app* app)
{
    ALIA_ASSERT(app);
    g_app = app;
    refresh_resolved_fonts(
        alia_font_roboto_regular_index, alia_font_roboto_bold_index, 1.0f);
}

void
demo_set_typefaces(
    alia_app* app,
    size_t body_font_index,
    size_t heading_font_index,
    float scale)
{
    ALIA_ASSERT(app);
    g_app = app;
    refresh_resolved_fonts(body_font_index, heading_font_index, scale);
}

alia_srgba8
demo_ink(alia::ui_context ctx, demo_ink_weight weight)
{
    using namespace alia::color;
    float t = 0.f;
    switch (weight)
    {
        case DEMO_INK_MUTED:
            t = -0.25f;
            break;
        case DEMO_INK_BODY:
            t = 0.f;
            break;
        case DEMO_INK_EMPHASIS:
            t = 0.1f;
            break;
        case DEMO_INK_STRONG:
            t = 0.2f;
            break;
    }
    color_program_expr expr = sample(text);
    if (t > 0.f)
        expr = expr | strengthen(t);
    else if (t < 0.f)
        expr = expr | weaken(-t);
    return resolve_expr(alia_ctx_style(ctx)->environment, expr);
}

alia_srgb8
demo_surface(alia::ui_context ctx)
{
    return alia_palette_color_srgb(&alia_ctx_style(ctx)->environment->palette, ALIA_STYLE_COLOR_SURFACE);
}

alia_srgb8
demo_raised(alia::ui_context ctx)
{
    // One WELL-step of elevation toward text (matches default well offset).
    alia_palette elevated;
    alia_palette_elevate(&elevated, &alia_ctx_style(ctx)->environment->palette, 0.075f);
    return alia_palette_color_srgb(&elevated, ALIA_STYLE_COLOR_SURFACE);
}

alia_srgb8
demo_accent_wash(alia::ui_context ctx)
{
    using namespace alia::color;
    alia_srgba8 const rgba = resolve_expr(
        alia_ctx_style(ctx)->environment,
        sample(primary) | fade(surface, 0.75f, 1.f));
    return alia_srgb8_make(rgba.r, rgba.g, rgba.b);
}

alia_srgba8
demo_on_accent_wash(alia::ui_context ctx)
{
    using namespace alia::color;
    return resolve_expr(
        alia_ctx_style(ctx)->environment,
        sample(primary) | match_lightness(text) | scale_chroma(0.6f));
}

void
demo_text(
    alia::ui_context ctx,
    char const* text,
    alia_resolved_font const* font,
    alia_srgba8 color,
    alia::layout_flag_set flags)
{
    alia_text_style const style = {
        .font = font,
        .color = color,
    };
    alia_text(ctx, alia::raw_code(flags), alia_text_literal(text), &style);
}
