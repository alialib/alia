#include <cstdio>
#include <iomanip>
#include <iostream>

#include <alia/shell/app.h>

#include <alia/abi/base/color.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/styling.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/system/tracer.h>
#include <alia/base/color.hpp>
#include <alia/context.h>
#include <alia/kernel/actions/basic.hpp>
#include <alia/kernel/actions/operators.hpp>
#include <alia/kernel/signals/adaptors.hpp>
#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/lambdas.hpp>
#include <alia/ui/layout/api.hpp>
#include <alia/ui/library.hpp>
#include <alia/ui/system/object.h>

using namespace alia;
using namespace alia::operators;

static alia_srgb8 const primary_colors[] = {
    hex_color("94c1fd"),
    hex_color("#6f42c1"),
    hex_color("#a52e45"),
};
static int primary_index = 0;

alia_ui_system* the_system;
static alia_ui_tracer the_tracer;

static float demo_spacing = 6.f;

#include "common/demo_text.hpp"
#include "prototyping/allocation_probe.h"
#include "prototyping/panel.h"

#include "alia_fonts.h"

alia_app* the_app = nullptr;

char const* lorem_ipsum
    = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin sed "
      "dictum massa. Maecenas et euismod lorem, ut dapibus eros.";

void
do_heading(context& ctx, char const* text)
{
    demo_text(
        ctx,
        text,
        &demo_get_fonts().heading_18,
        demo_text_color(ALIA_PALETTE_RAMP_LEVEL_STRONGER_2));
}

void
do_subheading(context& ctx, char const* text)
{
    demo_text(
        ctx,
        text,
        &demo_get_fonts().heading_14,
        demo_text_color(ALIA_PALETTE_RAMP_LEVEL_STRONGER_1));
}

void
do_controls(context& ctx)
{
    do_heading(ctx, "SETTINGS");

    do_subheading(ctx, "Typography");
    {
        static int typography_index = 0;
        radio_button(
            ctx, make_radio_signal(ref(typography_index), value(0)), "Roboto");
        radio_button(
            ctx,
            make_radio_signal(ref(typography_index), value(1)),
            "Source Sans 3");
        if (typography_index == 0)
        {
            demo_set_typefaces(
                the_app,
                alia_font_roboto_regular_index,
                alia_font_roboto_bold_index);
        }
        else
        {
            demo_set_typefaces(
                the_app,
                alia_font_source_sans_regular_index,
                alia_font_source_sans_bold_index);
        }
    }

    do_subheading(ctx, "Spacing");
    slider(ctx, ref(demo_spacing), 0.0, 24.0, 1.0);

    do_subheading(ctx, "Magnification");
    {
        alia_ui_system* const ui = ctx.system;
        slider(
            ctx,
            lambda_binding(
                [ui] { return double(alia_ui_get_magnification(ui)); },
                [ui](double value) {
                    alia_ui_set_magnification(ui, float(value));
                }),
            0.25,
            3.0,
            0.001);
    }
}

void
do_toggle_switch_demo(context& ctx)
{
    do_heading(ctx, "TOGGLE SWITCHES");

    static bool setting_one = false;
    toggle_switch(ctx, ref(setting_one), "Setting One");

    static bool setting_two = false;
    toggle_switch(ctx, ref(setting_two), "Setting Two");

    static bool setting_three = false;
    toggle_switch(ctx, ref(setting_three), "Setting Three");
}

void
do_radio_button_demo(context& ctx)
{
    static int radio_index = 0;

    do_heading(ctx, "RADIO BUTTONS");

    radio_button(
        ctx, make_radio_signal(ref(radio_index), value(0)), "Option One");
    radio_button(
        ctx, make_radio_signal(ref(radio_index), value(1)), "Option Two");
    radio_button(
        ctx, make_radio_signal(ref(radio_index), value(2)), "Option Three");
}

void
do_checkbox_demo(context& ctx)
{
    do_heading(ctx, "CHECKBOXES");

    static bool setting_one = false;
    checkbox(ctx, ref(setting_one), "Initially Unchecked");

    static bool setting_two = true;
    checkbox(ctx, ref(setting_two), "Initially Checked");

    static bool setting_disabled_unchecked = false;
    checkbox(
        ctx,
        disable_writes(ref(setting_disabled_unchecked)),
        "Disabled/Unchecked");

    static bool setting_disabled_checked = true;
    checkbox(
        ctx,
        disable_writes(ref(setting_disabled_checked)),
        "Disabled/Checked");
}

void
do_node_expander_demo(context& ctx)
{
    do_heading(ctx, "NODE EXPANDER");

    static bool expanded = false;
    node_expander(ctx, ref(expanded), "Expandable");

    static bool disabled_expanded = true;
    node_expander(ctx, disable_writes(ref(disabled_expanded)), "Disabled");
}

void
do_slider_demo(context& ctx)
{
    do_heading(ctx, "SLIDERS");

    static float slider_value = 5.f;
    slider(ctx, ref(slider_value), 0.0, 10.0, 1.0);
}

void
do_collapsible_demo(context& ctx)
{
    do_heading(ctx, "COLLAPSIBLE");

    static bool collapsed = false;
    node_expander(ctx, ref(collapsed), "Collapsible");

    alia::collapsible(ctx, ref(collapsed), [&]() {
        flow(ctx, FILL, [&]() {
            demo_text(
                ctx,
                lorem_ipsum,
                &demo_get_fonts().body_14,
                demo_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE));
        });
    });
}

void
do_button_demo(context& ctx)
{
    do_heading(ctx, "BUTTONS");

    static int clicks = 0;
    static int n = 0;
    static int m = 3;

    {
        char label[64];
        std::snprintf(label, sizeof(label), "Clicked %d times", clicks);
        demo_text(
            ctx,
            label,
            &demo_get_fonts().body_14,
            demo_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE));
    }

    row(ctx, ALIGN_LEFT, [&]() {
        button(ctx, "Click me", ++ref(clicks));
        button(ctx, "Reset", ref(clicks) <<= 0);
        button(ctx, "Disabled", actions::unready());
    });

    {
        char label[64];
        std::snprintf(label, sizeof(label), "n = %d, m = %d", n, m);
        demo_text(
            ctx,
            label,
            &demo_get_fonts().body_14,
            demo_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE));
    }

    row(ctx, ALIGN_LEFT, [&]() {
        button(ctx, "n <<= m", ref(n) <<= ref(m));
        button(ctx, "m <<= n", ref(m) <<= ref(n));
        button(ctx, "n <<= empty", ref(n) <<= empty<int>());
    });

    do_subheading(ctx, "Variants");
    row(ctx, ALIGN_LEFT, [&]() {
        button(ctx, "Primary", actions::noop());
        button(ctx, "Danger", actions::noop(), {swatch::danger});
        button(
            ctx,
            "Outline",
            actions::noop(),
            {swatch::primary, button_chrome::outline});
        button(
            ctx,
            "Outline Danger",
            actions::noop(),
            {swatch::danger, button_chrome::outline});
    });
}

void
do_text_sample(context& ctx)
{
    do_heading(ctx, "TEXT");
    flow(ctx, FILL, [&]() {
        demo_text(
            ctx,
            lorem_ipsum,
            &demo_get_fonts().body_14,
            demo_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE));
        demo_text(
            ctx,
            lorem_ipsum,
            &demo_get_fonts().heading_14,
            demo_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE));
        demo_text(
            ctx,
            lorem_ipsum,
            &demo_get_fonts().body_14,
            demo_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE));
    });
}

void
do_content(context& ctx, layout_options layout = {})
{
    column(ctx, layout, [&]() {
        do_toggle_switch_demo(ctx);
        separator(ctx);
        do_button_demo(ctx);
        separator(ctx);
        do_node_expander_demo(ctx);
        separator(ctx);
        do_radio_button_demo(ctx);
        separator(ctx);
        do_checkbox_demo(ctx);
        separator(ctx);
        do_slider_demo(ctx);
        separator(ctx);
        do_collapsible_demo(ctx);
        separator(ctx);
        do_text_sample(ctx);
    });
}

template<class Content>
void
with_spacing(context& ctx, float spacing, Content&& content)
{
    alia_layout_style* layout_style = alia_layout_style_active(&ctx);
    float old_spacing = layout_style->spacing;
    layout_style->spacing = spacing * ctx.geometry->scale;
    content();
    layout_style->spacing = old_spacing;
}

void
the_demo(context& ctx)
{
    with_spacing(ctx, 0, [&] {
        row(ctx, [&]() {
            concrete_panel(
                ctx,
                0,
                ctx.palette->foundation.background.stronger_2,
                FILL,
                [&]() {
                    with_spacing(ctx, 6, [&] {
                        column(ctx, pad(40), [&]() { do_controls(ctx); });
                    });
                });
            with_spacing(ctx, demo_spacing, [&] {
                concrete_panel(
                    ctx,
                    0,
                    ctx.palette->foundation.background.base,
                    GROW,
                    [&]() {
                        column(ctx, GROW, [&]() {
                            scroll_view(ctx, GROW, [&]() {
                                do_content(ctx, pad(40));
                            });
                        });
                    });
            });
        });
    });
}

static void
the_demo_controller(void* user_data, alia_context* ctx)
{
    (void) user_data;
    the_demo(*ctx);
}

void
update()
{
    AllocProbeResult result
        = probe_allocations([&]() { alia_app_shell_frame(the_system); });
    (void) result;
}

static void
app_frame(void* /*user_data*/)
{
    update();
}

int
main()
{
    static alia_app app;
    the_app = &app;
    alia_app_config const config = {
        .inner = {the_demo_controller, nullptr},
        .shell = {
            .draw_foundation_underlay = true,
            .surface_padding = {},
            .enable_keyboard_zoom = true,
            .enable_smooth_zoom = true,
        },
        .frame = {app_frame, nullptr},
        .continuous = false,
        .title = "Alia Widget Gallery",
        .window_state = alia_window_state_make(1600, 1600),
        .canvas_selector = "#canvas",
    };
    if (alia_app_init(&config, &app) != 0)
        return 1;

    static bool light_theme = false;
    the_system = alia_app_ui(&app);

    alia_ui_tracer_init(&the_tracer);
    alia_ui_tracer_attach(&the_tracer, the_system);

    alia_app_setup_stock_text(&app);
    demo_setup_fonts(&app);

    static bool theme_initialized = false;
    if (!theme_initialized)
    {
        alia_theme_accent accent;
        alia_theme_accent_from_color(&accent, primary_colors[primary_index]);
        alia_theme_context theme_ctx
            = alia_theme_context_default(!light_theme);
        alia_palette_from_accent(
            &the_system->palette,
            &accent,
            &theme_ctx,
            nullptr,
            nullptr,
            ALIA_LITERAL_FIXED_SPECTRUM);
        alia_style_generate_defaults(the_system, nullptr);
        theme_initialized = true;
    }

    alia_app_run_loop(&config, &app);
#ifndef __EMSCRIPTEN__
    alia_app_destroy(&app);
#endif
    return 0;
}
