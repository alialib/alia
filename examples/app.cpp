#include <algorithm>
#include <cstdio>
#include <functional>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <utility>

#include <alia/shell/app.h>

#include <alia/abi/base/arena.h>
#include <alia/abi/base/color.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/kernel/effect.h>
#include <alia/abi/ui/drawing/primitives.h>
#include <alia/abi/ui/drawing/targets.h>
#include <alia/abi/ui/events.h>
#include <alia/abi/ui/input/constants.h>
#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/input/keyboard.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/layout/system.h>
#include <alia/abi/ui/layout/utilities.h>
#include <alia/abi/ui/library.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/styling.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/system/input_processing.h>
#include <alia/abi/ui/system/tracer.h>
#include <alia/base/color.hpp>
#include <alia/context.h>
#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>
#include <alia/kernel/actions/basic.hpp>
#include <alia/kernel/actions/operators.hpp>
#include <alia/kernel/flow/dispatch.h>
#include <alia/kernel/macros.hpp>
#include <alia/kernel/signals/adaptors.hpp>
#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/lambdas.hpp>
#include <alia/ui/drawing/system.h>
#include <alia/ui/layout/api.hpp>
#include <alia/ui/library.hpp>
#include <alia/ui/system/internal_api.h>
#include <alia/ui/system/object.h>

using namespace alia;
using namespace alia::operators;

static alia_srgb8 const primary_colors[] = {
    hex_color("94c1fd"), // hex_color("#154DCF"),
    hex_color("#6f42c1"),
    hex_color("#a52e45"),
};
static int primary_index = 0;

alia_ui_system* the_system;
static alia_ui_tracer the_tracer;

static float demo_spacing = 6.f;
static float demo_node_expander_triangle_side = 24.f;

#include "common/demo_text.hpp"
#include "prototyping/allocation_probe.h"
#include "prototyping/flow_panel.h"
#include "prototyping/panel.h"
#include "prototyping/rect.h"

// template<class Content>
// void
// button(
//     context& ctx,
//     alia_z_index z_index,
//     alia_rgba color,
//     layout_flag_set flags,
//     Content&& content)
// {
//     alia_box button_box;
//     row(ctx, flags, &button_box, [&]() {
//         if (get_event_type(ctx) == ALIA_EVENT_DRAW)
//         {
//             alia_draw_rounded_box(&ctx, z_index, button_box, color, 0.0f);
//         }

//         std::forward<Content>(content)();
//     });
// }

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

template<class Content>
void
with_palette(context& ctx, alia_palette* palette, Content&& content)
{
    alia_palette* old_palette = ctx.palette;
    ctx.palette = palette;
    content();
    ctx.palette = old_palette;
}

char const* lorem_ipsum
    = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin sed "
      "dictum massa. Maecenas et euismod lorem, ut dapibus eros. Nam maximus, "
      "purus vitae mollis ornare, tortor justo posuere neque, at lacinia ante "
      "metus eget diam.\n\nAenean sit amet posuere metus. In hac habitasse "
      "platea dictumst. Nam sed turpis ultricies tellus auctor egestas. Ut "
      "laoreet nisi nisi, id posuere tortor tincidunt a. Pellentesque "
      "placerat vulputate massa at semper. Fusce malesuada porttitor enim "
      "dignissim viverra. In aliquam, odio nec sagittis elementum, elit enim "
      "auctor turpis, sit amet volutpat enim massa ac orci. Maecenas iaculis, "
      "ex at pulvinar volutpat, ligula nulla pellentesque tellus, vel aliquam "
      "nunc dolor eu risus.";

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
do_draw_target_demo(context& ctx)
{
    do_subheading(ctx, "Draw Target");

    static alia_draw_target_id target = ALIA_DRAW_TARGET_PRIMARY;
    if (target == ALIA_DRAW_TARGET_PRIMARY && ctx.system)
        target = alia_draw_target_create(ctx.system);

    alia_vec2f const size{160.f, 96.f};
    if (get_event_category(ctx) == ALIA_CATEGORY_REFRESH)
    {
        alia_layout_leaf_emit(&ctx, alia_layout_content_metrics_make(size), 0);
        return;
    }

    alia_box const box = alia_layout_consume_box(&ctx);
    if (get_event_category(ctx) != ALIA_CATEGORY_DRAWING)
        return;
    if (box.size.x < 1.f || box.size.y < 1.f)
        return;

    if (target == ALIA_DRAW_TARGET_PRIMARY)
    {
        alia_draw_rounded_box(
            &ctx, 0, box, alia_srgba8{220, 40, 40, 255}, 12.f);
        return;
    }

    alia_draw_target_begin(&ctx, target, box.size);
    alia_draw_rounded_box(
        &ctx, 0, {{0.f, 0.f}, box.size}, alia_srgba8{40, 40, 255, 255}, 12.f);
    alia_draw_rounded_box(
        &ctx,
        1,
        {{20.f, 20.f}, {box.size.x - 40.f, box.size.y - 40.f}},
        alia_srgba8{255, 255, 255, 220},
        8.f);
    alia_draw_target_end(&ctx);

    alia_draw_target(&ctx, 2, target, box, 0.85f);
}

void
do_controls(context& ctx)
{
    do_heading(ctx, "GEOMETRY");

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

    do_draw_target_demo(ctx);

    do_subheading(ctx, "Node Expander");
    slider(ctx, ref(demo_node_expander_triangle_side), 14.0, 36.0, 0.5);
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
    checkbox(ctx, ref(setting_two), "Initially Checked", GROW | pad(4));

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

    // Apply the interactive tuning sliders to the active style.
    alia_node_expander_style_active(&ctx)->triangle_side
        = demo_node_expander_triangle_side;

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
do_content(context& ctx)
{
    column(ctx, [&]() {
        do_toggle_switch_demo(ctx);
        do_heading(ctx, "");
        do_button_demo(ctx);
        do_heading(ctx, "");
        do_node_expander_demo(ctx);
        do_heading(ctx, "");
        do_radio_button_demo(ctx);
        do_heading(ctx, "");
        do_checkbox_demo(ctx);
        do_heading(ctx, "");
        do_slider_demo(ctx);
        do_heading(ctx, "");
        do_collapsible_demo(ctx);
        do_heading(ctx, "");
        do_heading(ctx, "GAPS");
        {
            static float x_gap = 5.f, y_gap = 5.f;
            slider(ctx, ref(x_gap), 0.0, 200.0, 0.1);
            slider(ctx, ref(y_gap), 0.0, 200.0, 0.1);
            column(ctx, alia::gap(y_gap), [&]() {
                block_flow(ctx, alia::gap(x_gap), [&]() {
                    for (int i = 0; i < 60; ++i)
                    {
                        do_rect(
                            ctx,
                            0,
                            {72, 72},
                            alia_srgb8{
                                uint8_t(0xff * float(i) / 60.f),
                                uint8_t(0xff * 0.1f),
                                uint8_t(0xff * (1.0f - float(i) / 60.f))},
                            CENTER);
                    }
                });
                do_rect(
                    ctx,
                    0,
                    {72, 72},
                    alia_srgb8{uint8_t(0xff), uint8_t(0xff), uint8_t(0xff)},
                    CENTER);
                for (int i = 0; i < 1; ++i)
                {
                    row(ctx, alia::gap(x_gap), [&]() {
                        for (int j = 0; j < 10; ++j)
                        {
                            do_rect(
                                ctx,
                                0,
                                {72, 72},
                                alia_srgb8{
                                    uint8_t(0xff * float(i) / 10.f),
                                    uint8_t(0xff * 0.1f),
                                    uint8_t(0xff * (1.0f - float(i) / 10.f))},
                                CENTER);
                        }
                    });
                }
            });
        }
        do_heading(ctx, "");
        // do_heading(ctx, "BLOCK FLOW");
        // block_flow_demo(ctx);
        // do_heading(ctx, "");
        // do_heading(ctx, "MIXED FLOW");
        // mixed_flow_demo(ctx);
        // do_heading(ctx, "");
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
        do_heading(ctx, "FLOW PANEL");
        {
            static float gap = 5.f, line_gap = 5.f, minimum_line_height = 5.f;
            slider(ctx, ref(gap), 0.0, 200.0, 0.1);
            slider(ctx, ref(line_gap), 0.0, 200.0, 0.1);
            slider(ctx, ref(minimum_line_height), 0.0, 200.0, 0.1);
            flow(
                ctx,
                alia::gap(gap),
                alia::line_gap(line_gap),
                alia::minimum_line_height(minimum_line_height),
                [&]() {
                    demo_text(
                        ctx,
                        lorem_ipsum,
                        &demo_get_fonts().body_14,
                        demo_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE));
                    demo_text(
                        ctx,
                        lorem_ipsum,
                        &demo_get_fonts().body_14,
                        demo_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE));
                    flow(
                        ctx,
                        alia::line_gap(40.f),
                        alia::minimum_line_height(40.f),
                        [&]() {
                            demo_text(
                                ctx,
                                "Nested flow (40px line gap and minimum line "
                                "height). "
                                "These lines should be more spaced than the "
                                "outer flow when the outer sliders are low.",
                                &demo_get_fonts().body_14,
                                demo_text_color(
                                    ALIA_PALETTE_RAMP_LEVEL_WEAKER_1));
                        });
                    do_flow_panel(
                        ctx,
                        0,
                        alia_edge_offsets_make_uniform(8.f),
                        ctx.palette->primary.subtle,
                        [&]() {
                            alia_palette_color const on_subtle
                                = alia_palette_color_make(
                                    alia_palette_index_swatch(
                                        ALIA_PALETTE_SWATCH_PRIMARY,
                                        ALIA_PALETTE_SWATCH_PART_ON_SUBTLE),
                                    0xff);
                            demo_text(
                                ctx,
                                "Panel text A.",
                                &demo_get_fonts().body_14,
                                on_subtle);
                            demo_text(
                                ctx,
                                "Panel text B (gap from outer flow slider).",
                                &demo_get_fonts().body_14,
                                on_subtle);
                        });
                });
        }
    });
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
                    edge_offsets(
                        ctx,
                        {.left = 40, .right = 40, .top = 40, .bottom = 40},
                        [&]() {
                            with_spacing(ctx, 6, [&] {
                                column(ctx, [&]() { do_controls(ctx); });
                            });
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
                                edge_offsets(
                                    ctx,
                                    {.left = 40,
                                     .right = 40,
                                     .top = 40,
                                     .bottom = 40},
                                    [&]() { do_content(ctx); });
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

    // temporary text dump until the dev overlay lands
    if (alia_trace_frame const* frame = alia_ui_tracer_latest(&the_tracer))
    {
        int64_t const wall_us
            = alia_trace_ticks_to_ns(frame->wall_end - frame->wall_start)
            / 1000;
        std::cout << "frame " << frame->index << ": " << std::setw(6)
                  << wall_us << "us";
        for (uint16_t i = 0; i < frame->pass_count; ++i)
        {
            alia_trace_pass const& pass = frame->passes[i];
            int64_t const us
                = alia_trace_ticks_to_ns(pass.end - pass.start) / 1000;
            std::cout << " | " << alia_trace_pass_kind_name(pass.kind);
            if (pass.kind == ALIA_TRACE_PASS_REFRESH)
            {
                std::cout << '#' << unsigned(pass.refresh.index);
                if (pass.refresh.incomplete)
                    std::cout << '*';
            }
            else if (pass.kind == ALIA_TRACE_PASS_EVENT)
            {
                std::cout
                    << "(0x" << std::hex << pass.event.type << std::dec << ')';
            }
            std::cout << ' ' << us << "us";
        }
        if (frame->dropped_passes != 0)
            std::cout << " | dropped " << frame->dropped_passes;
        std::cout << std::endl;
    }
}

static void
app_frame(void* /*user_data*/)
{
    update();
}

int
main()
{
    // Web host returns after scheduling RAF; keep storage for the page
    // lifetime.
    static alia_app app;
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
        .title = "Alia Renderer",
        .window_state = alia_window_state_make(1200, 1200),
        .canvas_selector = "#canvas",
    };
    if (alia_app_init(&config, &app) != 0)
        return 1;

    static bool light_theme = false;
    the_system = alia_app_ui(&app);

    alia_ui_tracer_init(&the_tracer);
    alia_ui_tracer_attach(&the_tracer, the_system);

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

    alia_app_setup_stock_text(&app);
    demo_setup_fonts(&app);

    alia_app_run_loop(&config, &app);
#ifndef __EMSCRIPTEN__
    alia_app_destroy(&app);
#endif
    return 0;
}
