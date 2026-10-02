#include <cstdio>
#include <iomanip>
#include <iostream>

#include <alia/shell/app.h>

#include <alia/abi/base/color.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/drawing/primitives.h>
#include <alia/abi/ui/drawing/targets.h>
#include <alia/abi/ui/events.h>
#include <alia/abi/ui/layout/system.h>
#include <alia/abi/ui/layout/utilities.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/styling.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/system/tracer.h>
#include <alia/base/color.hpp>
#include <alia/context.h>
#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>
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
#include "prototyping/flow_panel.h"
#include "prototyping/panel.h"
#include "prototyping/rect.h"

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
}

void
do_content(context& ctx, layout_options layout = {})
{
    column(ctx, layout, [&]() {
        do_heading(ctx, "GAPS");
        {
            static float x_gap = 5.f, y_gap = 5.f;
            slider(ctx, ref(x_gap), 0.0, 200.0, 0.1);
            slider(ctx, ref(y_gap), 0.0, 200.0, 0.1);
            column(ctx, alia::gap(y_gap), pad(8.f), [&]() {
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
                alia::gap(gap) | alia::line_gap(line_gap)
                    | alia::minimum_line_height(minimum_line_height),
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
                        alia::line_gap(40.f) | alia::minimum_line_height(40.f),
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
                                = ALIA_PALETTE_COLOR(primary.on_subtle, 0xff);
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

    // if (alia_trace_frame const* frame = alia_ui_tracer_latest(&the_tracer))
    // {
    //     int64_t const wall_us
    //         = alia_trace_ticks_to_ns(frame->wall_end - frame->wall_start)
    //         / 1000;
    //     std::cout << "frame " << frame->index << ": " << std::setw(6)
    //               << wall_us << "us\n";
    // }
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
        .title = "Alia Layout Lab",
        .window_state = alia_window_state_make(1200, 1200),
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
