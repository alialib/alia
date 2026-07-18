#include "demo_text.hpp"

#include <alia/abi/prelude.h>
#include <alia/flags.hpp>

namespace {

demo_fonts g_fonts{};

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
    alia_ui_system* ui = alia_app_ui(app);
    ALIA_ASSERT(ui);
    ALIA_ASSERT(alia_app_typeface_count(app) > 0);

    alia_typeface_id const body = alia_app_typeface(app, 0);
    alia_typeface_id const heading = alia_app_typeface_count(app) > 1
                                       ? alia_app_typeface(app, 1)
                                       : body;

    fill_resolved_font(ui, body, 14.f, &g_fonts.body_14);
    fill_resolved_font(ui, heading, 14.f, &g_fonts.heading_14);
    fill_resolved_font(ui, heading, 18.f, &g_fonts.heading_18);
}

void
demo_text(
    alia::context& ctx,
    char const* text,
    alia_resolved_font const* font,
    alia_palette_color color,
    alia::layout_flag_set flags)
{
    alia_text_style const style = {.font = font, .color = color};
    alia_text(
        &ctx, alia::raw_code(flags), alia_text_literal(text), &style);
}
