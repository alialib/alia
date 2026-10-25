#include <cstdio>
#include <iomanip>
#include <iostream>

#include <alia/shell/app.h>

#include <alia/abi/base/color.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/geometry/api.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/styling/api.h>
#include <alia/abi/ui/styling/catalog.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/system/tracer.h>
#include <alia/base/color.hpp>
#include <alia/impl/context.hpp>
#include <alia/kernel/actions/basic.hpp>
#include <alia/kernel/actions/operators.hpp>
#include <alia/kernel/macros.hpp>
#include <alia/kernel/signals/adaptors.hpp>
#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/lambdas.hpp>
#include <alia/kernel/signals/state.hpp>
#include <alia/ui/layout/api.hpp>
#include <alia/ui/library.hpp>
#include <alia/ui/system/object.h>
#include <alia/ui/text.hpp>

using namespace alia;
using namespace alia::operators;

static alia_srgb8 const primary_colors[] = {
    hex_color("94c1fd"),
    hex_color("#6f42c1"),
    hex_color("#a52e45"),
};
static int primary_index = 0;
static bool light_theme = false;
static bool theme_dirty = false;

alia_ui_system* the_system;
static alia_ui_tracer the_tracer;

static void
apply_gallery_theme()
{
    alia_theme_accent accent;
    alia_theme_accent_from_color(&accent, primary_colors[primary_index]);
    alia_palette_conditions conditions
        = alia_palette_conditions_default(!light_theme);
    alia_palette_generate_from_accent(
        &the_system->palette, &accent, &conditions, nullptr, nullptr);
    alia_ui_paint_changed(the_system);
    theme_dirty = false;
}

#include "common/demo_text.hpp"
#include "prototyping/allocation_probe.h"
#include "prototyping/panel.h"

#include "alia_fonts.h"

alia_app* the_app = nullptr;

char const* lorem_ipsum
    = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin sed "
      "dictum massa. Maecenas et euismod lorem, ut dapibus eros.";

// Inset `content` vertically without the symmetric leaf margin.
template<class Content>
void
with_vertical_insets(
    ui_context ctx, float top, float bottom, Content&& content)
{
    alia_layout_edge_offsets_begin(
        ctx, alia_edge_offsets{0.f, 0.f, top, bottom}, 0);
    std::forward<Content>(content)();
    alia_layout_edge_offsets_end(ctx);
}

void
do_heading(ui_context ctx, char const* text)
{
    // Sit close to the section content. The separator above carries the break.
    with_vertical_insets(ctx, 4.f, 2.f, [&] {
        demo_text(
            ctx,
            text,
            &demo_get_fonts().heading_18,
            demo_text_color(ALIA_PALETTE_RAMP_LEVEL_STRONGER_2),
            FLUSH);
    });
}

void
do_subheading(ui_context ctx, char const* text)
{
    with_vertical_insets(ctx, 14.f, 2.f, [&] {
        demo_text(
            ctx,
            text,
            &demo_get_fonts().heading_14,
            demo_text_color(ALIA_PALETTE_RAMP_LEVEL_STRONGER_1),
            FLUSH);
    });
}

void
do_section_separator(ui_context ctx)
{
    with_vertical_insets(ctx, 12.f, 6.f, [&] { separator(ctx, FLUSH); });
}

void
apply_gallery_typeface(int typography_index)
{
    if (typography_index == 0)
    {
        demo_set_typefaces(
            the_app,
            alia_font_roboto_regular_index,
            alia_font_roboto_bold_index,
            1.00f);
    }
    else
    {
        demo_set_typefaces(
            the_app,
            alia_font_source_sans_regular_index,
            alia_font_source_sans_bold_index,
            1.08f);
    }
}

template<class Spacing, class Magnification, class Typography>
void
do_controls(
    ui_context ctx,
    Spacing const& spacing,
    Magnification const& magnification,
    Typography const& typography)
{
    do_heading(ctx, "SETTINGS");

    do_subheading(ctx, "Typography");
    radio_button(ctx, typography, 0, "Roboto");
    radio_button(ctx, typography, 1, "Source Sans 3");

    do_subheading(ctx, "Appearance");
    toggle_switch(
        ctx,
        lambda_binding(
            [] { return light_theme; },
            [](bool value) {
                light_theme = value;
                theme_dirty = true;
            }),
        "Light");

    do_subheading(ctx, "Spacing");
    slider(ctx, spacing, 0.0, 24.0, 1.0, FILL);

    do_subheading(ctx, "Magnification");
    slider(ctx, magnification, 0.25, 3.0, 0.001, FILL);
}

void
do_toggle_switch_demo(ui_context ctx)
{
    do_heading(ctx, "TOGGLE SWITCHES");

    toggle_switch(ctx, use_state(ctx, false), "Setting One");
    toggle_switch(ctx, use_state(ctx, false), "Setting Two");
    toggle_switch(ctx, disable_writes(use_state(ctx, false)), "Disabled Off");
    toggle_switch(ctx, disable_writes(use_state(ctx, true)), "Disabled On");
}

void
do_radio_button_demo(ui_context ctx)
{
    do_heading(ctx, "RADIO BUTTONS");

    auto radio_index = use_state(ctx, 0);
    radio_button(ctx, radio_index, 0, "Option One");
    radio_button(ctx, radio_index, 1, "Option Two");
    radio_button(ctx, radio_index, 2, "Option Three");
}

void
do_checkbox_demo(ui_context ctx)
{
    do_heading(ctx, "CHECKBOXES");

    checkbox(ctx, use_state(ctx, false), "Initially Unchecked");
    checkbox(ctx, use_state(ctx, true), "Initially Checked");
    checkbox(ctx, disable_writes(use_state(ctx, false)), "Disabled/Unchecked");
    checkbox(ctx, disable_writes(use_state(ctx, true)), "Disabled/Checked");
}

void
do_node_expander_demo(ui_context ctx)
{
    do_heading(ctx, "NODE EXPANDER");

    node_expander(ctx, use_state(ctx, false), "Expandable");
    node_expander(ctx, disable_writes(use_state(ctx, true)), "Disabled");
}

void
do_slider_demo(ui_context ctx)
{
    do_heading(ctx, "SLIDERS");

    slider(ctx, use_state(ctx, 5.f), 0.0, 10.0, 1.0, FILL);
}

void
do_collapsible_demo(ui_context ctx)
{
    do_heading(ctx, "COLLAPSIBLE");

    auto collapsed = use_state(ctx, false);
    grid(ctx, [&](auto grid_handle) {
        alia_box box;
        grid_row(ctx, grid_handle, provide_box(box), [&]() {
            auto id = node_expander(ctx, collapsed);

            text(
                ctx,
                "Lorem Ipsum",
                CENTER_Y | pad({-alia_spacing_active(ctx), 0, 0, 0}));

            alia_element_box_region(
                ctx,
                id,
                &box,
                ALIA_Z_PLANE_CONTENT,
                ALIA_HIT_TEST_MOUSE,
                ALIA_CURSOR_DEFAULT);
        });
        grid_row(ctx, grid_handle, [&]() {
            spacer(ctx);
            collapsible(ctx, collapsed, GROW, [&]() {
                flow(ctx, [&]() {
                    demo_text(
                        ctx,
                        lorem_ipsum,
                        &demo_get_fonts().body_14,
                        demo_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE),
                        FLUSH);
                });
            });
        });
    });
}

void
do_button_demo(ui_context ctx)
{
    do_heading(ctx, "BUTTONS");

    auto clicks = use_state(ctx, 0);
    auto n = use_state(ctx, 0);
    auto m = use_state(ctx, 3);

    {
        char label[64];
        std::snprintf(
            label, sizeof(label), "Clicked %d times", read_signal(clicks));
        demo_text(
            ctx,
            label,
            &demo_get_fonts().body_14,
            demo_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE));
    }

    row(ctx, ALIGN_LEFT, [&]() {
        button(ctx, "Click me", ++clicks);
        button(ctx, "Reset", clicks <<= 0);
        button(ctx, "Disabled", actions::unready());
    });

    {
        char label[64];
        std::snprintf(
            label,
            sizeof(label),
            "n = %d, m = %d",
            read_signal(n),
            read_signal(m));
        demo_text(
            ctx,
            label,
            &demo_get_fonts().body_14,
            demo_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE));
    }

    row(ctx, ALIGN_LEFT, [&]() {
        button(ctx, "n += 1", n += 1);
        button(ctx, "m <<= n", m <<= n);
        button(ctx, "n <<= empty", n <<= empty<int>());
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
do_dropdown_demo(ui_context ctx)
{
    do_heading(ctx, "DROPDOWN");

    auto open = use_state(ctx, false);
    auto selected = use_state(ctx, 0);
    char const* const items[] = {"First", "Second", "Third"};

    {
        char label[64];
        std::snprintf(
            label,
            sizeof(label),
            "Selected: %s",
            items[read_signal(selected)]);
        demo_text(
            ctx,
            label,
            &demo_get_fonts().body_14,
            demo_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE));
    }

    alia_box trigger_box{};
    column(ctx, provide_box(trigger_box), ALIGN_LEFT, [&]() {
        button(
            ctx,
            read_signal(open) ? "Close" : items[read_signal(selected)],
            open <<= !open,
            width(120));
    });

    ALIA_IF_ (ctx, read_signal(open))
    {
        popup(ctx, trigger_box, [&]() {
            concrete_panel(
                ctx,
                alia_z(ctx, ALIA_Z_PLANE_BACKGROUND),
                alia_ctx_palette(ctx)->foundation.background.stronger_1,
                FILL,
                [&]() {
                    column(ctx, pad(6), [&]() {
                        for (int i = 0; i != 3; ++i)
                        {
                            button(
                                ctx,
                                items[i],
                                (selected <<= i, open <<= false));
                        }
                    });
                });
        });
    }
    ALIA_END
}

void
do_text_sample(ui_context ctx)
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
do_content(ui_context ctx, layout_options layout = {})
{
    column(ctx, layout, [&]() {
        do_toggle_switch_demo(ctx);
        do_section_separator(ctx);
        do_button_demo(ctx);
        do_section_separator(ctx);
        do_dropdown_demo(ctx);
        do_section_separator(ctx);
        do_node_expander_demo(ctx);
        do_section_separator(ctx);
        do_radio_button_demo(ctx);
        do_section_separator(ctx);
        do_checkbox_demo(ctx);
        do_section_separator(ctx);
        do_slider_demo(ctx);
        do_section_separator(ctx);
        do_collapsible_demo(ctx);
        do_section_separator(ctx);
        do_text_sample(ctx);
    });
}

template<class Content>
void
with_spacing(ui_context ctx, float spacing, Content&& content)
{
    alia_spacing_push(ctx, spacing * alia_ctx_geometry(ctx)->scale);
    content();
    alia_spacing_pop(ctx);
}

void
the_demo(ui_context ctx)
{
    if (theme_dirty)
        apply_gallery_theme();

    auto spacing = use_state(ctx, 6.f);
    auto magnification = use_state(ctx, 1.f);
    auto typography = use_state(ctx, 0);
    apply_gallery_typeface(read_signal(typography));

    alia_font_push(ctx, &demo_get_fonts().body_14);
    with_spacing(ctx, 0, [&] {
        row(ctx, [&]() {
            column(ctx, width(300), [&] {
                concrete_panel(
                    ctx,
                    0,
                    alia_ctx_palette(ctx)->foundation.background.stronger_1,
                    GROW | FILL,
                    [&]() {
                        with_spacing(ctx, 6, [&] {
                            column(ctx, pad(20), [&]() {
                                do_controls(
                                    ctx, spacing, magnification, typography);
                            });
                        });
                    });
            });
            with_spacing(ctx, read_signal(spacing), [&] {
                concrete_panel(
                    ctx,
                    0,
                    alia_ctx_palette(ctx)->foundation.background.base,
                    GROW,
                    [&]() {
                        column(ctx, GROW, [&]() {
                            scroll_view(ctx, GROW, [&]() {
                                alia_layout_edge_offsets_begin(
                                    ctx, {20, 20, 20, 20}, ALIA_FILL);
                                alia_geometry_push_scale(
                                    ctx, read_signal(magnification));
                                do_content(ctx);
                                alia_geometry_pop_scale(ctx);
                                alia_layout_edge_offsets_end(ctx);
                            });
                        });
                    });
            });
        });
    });
    alia_font_pop(ctx);
}

static void
the_demo_controller(void* user_data, alia_ui_context* ctx)
{
    (void) user_data;
    the_demo(ui_context{ctx});
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

    the_system = alia_app_ui(&app);

    alia_ui_tracer_init(&the_tracer);
    alia_ui_tracer_attach(&the_tracer, the_system);

    alia_app_setup_stock_text(&app);
    demo_setup_fonts(&app);

    apply_gallery_theme();
    alia_style_generate_defaults(the_system, nullptr);

    alia_app_run_loop(&config, &app);
#ifndef __EMSCRIPTEN__
    alia_app_destroy(&app);
#endif
    return 0;
}
