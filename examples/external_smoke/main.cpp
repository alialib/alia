// External-nest smoke: builds Alia via FetchContent (SOURCE_DIR) with
// ALIA_ENABLE_EXAMPLES=OFF, links a custom atlas (stock icons + typography +
// Source Sans), and shows a checkbox (icon registry) plus labeled text.

#include <alia/shell/app.h>

#include <alia/abi/base/color.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/styling.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/text.h>
#include <alia/base/color.hpp>
#include <alia/context.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/signals/basic.hpp>
#include <alia/ui/layout/api.hpp>
#include <alia/ui/library.hpp>
#include <alia/ui/system/object.h>

#include "alia_fonts.h"

using namespace alia;
using namespace alia::operators;

namespace {

alia_ui_system* the_system = nullptr;

alia_resolved_font body_font{};
alia_resolved_font heading_font{};

void
resolve_font(
    alia_ui_system* ui,
    alia_typeface_id typeface,
    float size,
    alia_resolved_font* out)
{
    alia_resolved_typeface const resolved
        = alia_typeface_resolve(ui, typeface);
    out->typeface = resolved;
    out->size = size;
    resolved.engine->vtable->get_font_metrics(
        resolved.engine, resolved.engine_handle, size, &out->metrics);
}

void
setup_demo_fonts(alia_app* app)
{
    alia_ui_system* ui = alia_app_ui(app);
    resolve_font(
        ui,
        alia_app_typeface(app, alia_font_source_sans_regular_index),
        16.f,
        &body_font);
    resolve_font(
        ui,
        alia_app_typeface(app, alia_font_source_sans_bold_index),
        20.f,
        &heading_font);
}

void
emit_text(
    context& ctx,
    char const* text,
    alia_resolved_font const* font,
    enum alia_palette_ramp_level level)
{
    alia_text_style const style = {
        .font = font,
        .color = alia_palette_color_make(
            alia_palette_index_foundation_ramp(
                ALIA_PALETTE_FOUNDATION_RAMP_TEXT, level),
            0xff),
    };
    alia_text(&ctx, 0, alia_text_literal(text), &style);
}

void
the_ui(context& ctx)
{
    column(ctx, GROW | pad(24), [&] {
        emit_text(
            ctx,
            "External nest smoke",
            &heading_font,
            ALIA_PALETTE_RAMP_LEVEL_STRONGER_2);

        emit_text(
            ctx,
            "Alia was pulled in with examples off. This atlas adds Source "
            "Sans after stock UI icons and Roboto. The checkbox checkmark "
            "should still draw from the icon registry.",
            &body_font,
            ALIA_PALETTE_RAMP_LEVEL_BASE);

        static bool checked = true;
        checkbox(ctx, ref(checked), "Stock check icon still works");
    });
}

void
the_controller(void* /*user_data*/, alia_context* ctx)
{
    the_ui(*ctx);
}

void
app_frame(void* /*user_data*/)
{
    alia_app_shell_frame(the_system);
}

} // namespace

int
main()
{
    static alia_app app;
    alia_app_config const config = {
        .inner = {the_controller, nullptr},
        .shell =
            {
                .draw_foundation_underlay = true,
                .surface_padding = {},
                .enable_keyboard_zoom = true,
                .enable_smooth_zoom = true,
            },
        .frame = {app_frame, nullptr},
        .continuous = false,
        .title = "Alia External Smoke",
        .window_state = alia_window_state_make(720, 480),
        .canvas_selector = "#canvas",
    };
    if (alia_app_init(&config, &app) != 0)
        return 1;

    the_system = alia_app_ui(&app);

    if (!alia_app_setup_stock_text(&app))
        return 1;
    setup_demo_fonts(&app);

    alia_theme_accent accent;
    alia_theme_accent_from_color(&accent, hex_color("94c1fd"));
    alia_theme_context theme_ctx = alia_theme_context_default(true);
    alia_palette_from_accent(
        &the_system->palette,
        &accent,
        &theme_ctx,
        nullptr,
        nullptr,
        ALIA_LITERAL_FIXED_SPECTRUM);
    alia_style_generate_defaults(the_system, nullptr);

    alia_app_run_loop(&config, &app);
#ifndef __EMSCRIPTEN__
    alia_app_destroy(&app);
#endif
    return 0;
}
