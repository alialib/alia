#include "demo_text.hpp"

#include <alia/abi/prelude.h>
#include <alia/base/flags.hpp>

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
refresh_resolved_fonts(size_t body_index, size_t heading_index)
{
    ALIA_ASSERT(g_app);
    alia_ui_system* ui = alia_app_ui(g_app);
    ALIA_ASSERT(ui);
    ALIA_ASSERT(body_index < alia_app_typeface_count(g_app));
    ALIA_ASSERT(heading_index < alia_app_typeface_count(g_app));

    fill_resolved_font(
        ui, alia_app_typeface(g_app, body_index), 14.f, &g_fonts.body_14);
    fill_resolved_font(
        ui,
        alia_app_typeface(g_app, heading_index),
        14.f,
        &g_fonts.heading_14);
    fill_resolved_font(
        ui,
        alia_app_typeface(g_app, heading_index),
        18.f,
        &g_fonts.heading_18);
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
        alia_font_roboto_regular_index, alia_font_roboto_bold_index);
}

void
demo_set_typefaces(
    alia_app* app, size_t body_font_index, size_t heading_font_index)
{
    ALIA_ASSERT(app);
    g_app = app;
    refresh_resolved_fonts(body_font_index, heading_font_index);
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
