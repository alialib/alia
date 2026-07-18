#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <utility>

#include <alia/shell/app.h>

#include <alia/abi/base/arena.h>
#include <alia/abi/base/color.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/drawing.h>
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
#include <alia/abi/ui/style.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/system/host_window.h>
#include <alia/abi/ui/system/input_processing.h>
#include <alia/base/color.hpp>
#include <alia/context.h>
#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>
#include <alia/kernel/flow/dispatch.h>
#include <alia/kernel/macros.hpp>
#include <alia/ui/drawing.h>
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

// Local widget style overrides for the content pane.
static alia_switch_style local_switch_style;
static alia_slider_style local_slider_style;
static alia_radio_style local_radio_style;
static alia_checkbox_style local_checkbox_style;
static alia_node_expander_style local_node_expander_style;
static bool local_styles_initialized = false;
static float demo_spacing = 6.f;
static float demo_scale = 1.0f;
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
    float old_spacing = ctx.style->spacing;
    ctx.style->spacing = spacing * ctx.geometry->scale;
    content();
    ctx.style->spacing = old_spacing;
}

template<class Content>
void
with_ui_scale(context& ctx, float scale, Content&& content)
{
    float old_scale = ctx.geometry->scale;
    float old_spacing = ctx.style->spacing;
    ctx.geometry->scale *= scale;
    ctx.style->spacing *= scale;
    content();
    ctx.geometry->scale = old_scale;
    ctx.style->spacing = old_spacing;
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

struct pass_aborted
{
};

void
abort_pass(context& ctx)
{
    ctx.events->aborted = true;
    throw pass_aborted();
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
do_radio_with_text(
    context& ctx,
    alia_bool_signal* value,
    char const* text,
    alia_radio_style const* style)
{
    alia_box row_box;
    row(ctx, ALIGN_LEFT, &row_box, [&]() {
        alia_element_id id = alia_do_radio(&ctx, value, ALIA_CENTER_Y, style);
        demo_text(
            ctx,
            text,
            &demo_get_fonts().body_14,
            demo_text_color(
                (value->flags & ALIA_SIGNAL_WRITABLE)
                    ? ALIA_PALETTE_RAMP_LEVEL_BASE
                    : ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
            CENTER_Y);
        alia_element_box_region(
            &ctx, id, &row_box, ALIA_CURSOR_DEFAULT, ALIA_HIT_TEST_MOUSE);
    });
}

void
do_checkbox_with_text(
    context& ctx,
    alia_bool_signal* value,
    char const* text,
    alia_checkbox_style const* style)
{
    alia_box row_box;
    row(ctx, ALIGN_LEFT, &row_box, [&]() {
        alia_element_id id
            = alia_do_checkbox(&ctx, value, ALIA_CENTER_Y, style);
        demo_text(
            ctx,
            text,
            &demo_get_fonts().body_14,
            demo_text_color(
                (value->flags & ALIA_SIGNAL_WRITABLE)
                    ? ALIA_PALETTE_RAMP_LEVEL_BASE
                    : ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
            CENTER_Y);
        alia_element_box_region(
            &ctx, id, &row_box, ALIA_CURSOR_DEFAULT, ALIA_HIT_TEST_MOUSE);
    });
}

void
do_switch_with_text(
    context& ctx,
    alia_bool_signal* value,
    char const* text,
    alia_switch_style const* style)
{
    alia_box row_box;
    row(ctx, ALIGN_LEFT, &row_box, [&]() {
        alia_element_id id = alia_do_switch(&ctx, value, ALIA_CENTER_Y, style);
        demo_text(
            ctx,
            text,
            &demo_get_fonts().body_14,
            demo_text_color(
                (value->flags & ALIA_SIGNAL_WRITABLE)
                    ? ALIA_PALETTE_RAMP_LEVEL_BASE
                    : ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
            CENTER_Y);
        alia_element_box_region(
            &ctx, id, &row_box, ALIA_CURSOR_DEFAULT, ALIA_HIT_TEST_MOUSE);
    });
}

void
do_node_expander_with_text(
    context& ctx,
    alia_bool_signal* value,
    char const* text,
    alia_node_expander_style const* style)
{
    alia_box row_box;
    row(ctx, ALIGN_LEFT, &row_box, [&]() {
        alia_element_id id
            = alia_do_node_expander(&ctx, value, ALIA_CENTER_Y, style);
        demo_text(
            ctx,
            text,
            &demo_get_fonts().body_14,
            demo_text_color(
                (value->flags & ALIA_SIGNAL_WRITABLE)
                    ? ALIA_PALETTE_RAMP_LEVEL_BASE
                    : ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
            CENTER_Y);
        alia_element_box_region(
            &ctx, id, &row_box, ALIA_CURSOR_DEFAULT, ALIA_HIT_TEST_MOUSE);
    });
}

void
do_controls(context& ctx)
{
    do_heading(ctx, "GEOMETRY");

    do_subheading(ctx, "Spacing");
    alia_do_slider_f(&ctx, &demo_spacing, 0.f, 24.f, 1.f, 0, false, nullptr);

    do_subheading(ctx, "Scale");
    alia_do_slider_f(&ctx, &demo_scale, 0.1f, 3.0f, 0.001f, 0, false, nullptr);

    do_subheading(ctx, "Node Expander");
    alia_do_slider_f(
        &ctx,
        &demo_node_expander_triangle_side,
        14.f,
        36.f,
        0.5f,
        0,
        false,
        nullptr);
}

void
do_switch_demo(context& ctx, alia_switch_style const* style)
{
    do_heading(ctx, "SWITCHES");

    {
        static bool setting_one = false;
        alia_bool_signal switch_signal{
            .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
            .value = setting_one,
        };
        do_switch_with_text(ctx, &switch_signal, "Setting One", style);
        if (switch_signal.flags & ALIA_SIGNAL_WRITTEN)
        {
            setting_one = switch_signal.value;
            abort_pass(ctx);
        }
    }

    {
        static bool setting_two = false;
        alia_bool_signal switch_signal{
            .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
            .value = setting_two,
        };
        do_switch_with_text(ctx, &switch_signal, "Setting Two", style);
        if (switch_signal.flags & ALIA_SIGNAL_WRITTEN)
        {
            setting_two = switch_signal.value;
            abort_pass(ctx);
        }
    }

    {
        static bool setting_three = false;
        alia_bool_signal switch_signal{
            .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
            .value = setting_three,
        };
        do_switch_with_text(ctx, &switch_signal, "Setting Three", style);
        if (switch_signal.flags & ALIA_SIGNAL_WRITTEN)
        {
            setting_three = switch_signal.value;
            abort_pass(ctx);
        }
    }
}

void
do_radio_demo(context& ctx, alia_radio_style const* style)
{
    static int radio_index = 0;

    do_heading(ctx, "RADIO BUTTONS");

    {
        alia_bool_signal radio_signal{
            .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
            .value = radio_index == 0,
        };
        do_radio_with_text(ctx, &radio_signal, "Option One", style);
        if (radio_signal.flags & ALIA_SIGNAL_WRITTEN)
        {
            radio_index = 0;
            abort_pass(ctx);
        }
    }

    {
        alia_bool_signal radio_signal{
            .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
            .value = radio_index == 1,
        };
        do_radio_with_text(ctx, &radio_signal, "Option Two", style);
        if (radio_signal.flags & ALIA_SIGNAL_WRITTEN)
        {
            radio_index = 1;
            abort_pass(ctx);
        }
    }

    {
        alia_bool_signal radio_signal{
            .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
            .value = radio_index == 2,
        };
        do_radio_with_text(ctx, &radio_signal, "Option Three", style);
        if (radio_signal.flags & ALIA_SIGNAL_WRITTEN)
        {
            radio_index = 2;
            abort_pass(ctx);
        }
    }
}

void
do_checkbox_demo(context& ctx, alia_checkbox_style const* style)
{
    do_heading(ctx, "CHECKBOXES");

    {
        static bool setting_one = false;
        alia_bool_signal checkbox_signal{
            .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
            .value = setting_one,
        };
        do_checkbox_with_text(
            ctx, &checkbox_signal, "Initially Unchecked", style);
        if (checkbox_signal.flags & ALIA_SIGNAL_WRITTEN)
        {
            setting_one = checkbox_signal.value;
            abort_pass(ctx);
        }
    }

    {
        static bool setting_two = true;
        alia_bool_signal checkbox_signal{
            .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
            .value = setting_two,
        };
        do_checkbox_with_text(
            ctx, &checkbox_signal, "Initially Checked", style);
        if (checkbox_signal.flags & ALIA_SIGNAL_WRITTEN)
        {
            setting_two = checkbox_signal.value;
            abort_pass(ctx);
        }
    }

    {
        static bool setting_disabled = false;
        alia_bool_signal checkbox_signal{
            .flags = ALIA_SIGNAL_READABLE,
            .value = setting_disabled,
        };
        do_checkbox_with_text(
            ctx, &checkbox_signal, "Disabled/Unchecked", style);
        (void) setting_disabled;
    }

    {
        static bool setting_disabled = true;
        alia_bool_signal checkbox_signal{
            .flags = ALIA_SIGNAL_READABLE,
            .value = setting_disabled,
        };
        do_checkbox_with_text(
            ctx, &checkbox_signal, "Disabled/Checked", style);
        (void) setting_disabled;
    }
}

void
do_node_expander_demo(context& ctx, alia_node_expander_style const* style)
{
    do_heading(ctx, "NODE EXPANDER");

    // Apply interactive tuning sliders.
    local_node_expander_style.triangle_side = demo_node_expander_triangle_side;

    alia_node_expander_style const* effective_style
        = style != nullptr ? style : &local_node_expander_style;

    {
        static bool expanded = false;
        alia_bool_signal expanded_signal{
            .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
            .value = expanded,
        };
        do_node_expander_with_text(
            ctx, &expanded_signal, "Expandable", effective_style);
        if (expanded_signal.flags & ALIA_SIGNAL_WRITTEN)
        {
            expanded = expanded_signal.value;
            abort_pass(ctx);
        }
    }

    {
        static bool disabled_expanded = true;
        alia_bool_signal disabled_signal{
            .flags = ALIA_SIGNAL_READABLE,
            .value = disabled_expanded,
        };
        do_node_expander_with_text(
            ctx, &disabled_signal, "Disabled", effective_style);
        (void) disabled_expanded;
    }
}

void
do_slider_demo(context& ctx, alia_slider_style const* style)
{
    do_heading(ctx, "SLIDERS");

    static float slider_value = 5.f;
    alia_do_slider_f(&ctx, &slider_value, 0.f, 10.f, 1.f, 0, false, style);
}

void
do_collapsible_demo(context& ctx)
{
    do_heading(ctx, "COLLAPSIBLE");

    static bool collapsed = false;
    alia_bool_signal collapsed_signal{
        .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
        .value = collapsed,
    };
    do_node_expander_with_text(ctx, &collapsed_signal, "Collapsible", nullptr);
    if (collapsed_signal.flags & ALIA_SIGNAL_WRITTEN)
    {
        collapsed = collapsed_signal.value;
        abort_pass(ctx);
    }

    alia::collapsible(ctx, &collapsed_signal, [&]() {
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
do_content(context& ctx)
{
    column(ctx, [&]() {
        do_switch_demo(ctx, nullptr); //&local_switch_style);
        do_heading(ctx, "");
        do_node_expander_demo(ctx, nullptr); //&local_node_expander_style);
        do_heading(ctx, "");
        do_radio_demo(ctx, nullptr); //&local_radio_style);
        do_heading(ctx, "");
        do_checkbox_demo(ctx, nullptr); //&local_checkbox_style);
        do_heading(ctx, "");
        do_slider_demo(ctx, nullptr); //&local_slider_style);
        do_heading(ctx, "");
        do_collapsible_demo(ctx);
        do_heading(ctx, "");
        do_heading(ctx, "GAPS");
        {
            static float x_gap = 5.f, y_gap = 5.f;
            alia_do_slider_f(
                &ctx, &x_gap, 0.f, 200.f, 0.1f, 0, false, nullptr);
            alia_do_slider_f(
                &ctx, &y_gap, 0.f, 200.f, 0.1f, 0, false, nullptr);
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
            alia_do_slider_f(&ctx, &gap, 0.f, 200.f, 0.1f, 0, false, nullptr);
            alia_do_slider_f(
                &ctx, &line_gap, 0.f, 200.f, 0.1f, 0, false, nullptr);
            alia_do_slider_f(
                &ctx,
                &minimum_line_height,
                0.f,
                200.f,
                0.1f,
                0,
                false,
                nullptr);
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
    try
    {
        alia_key_info k;
        if (alia_input_detect_global_key_press(&ctx, &k))
        {
            if (k.logical == ALIA_KEY_F11 && k.mods == ALIA_KMOD_NONE)
            {
                if (ctx.system->host_window.toggle_fullscreen)
                {
                    ctx.system->host_window.toggle_fullscreen(
                        ctx.system->host_window.user);
                }
                alia_input_acknowledge_key_event(&ctx);
            }
            else if (k.logical == ALIA_KEY_EQUAL && k.mods == ALIA_KMOD_NONE)
            {
                demo_scale *= 1.1f;
                alia_input_acknowledge_key_event(&ctx);
            }
            else if (k.logical == ALIA_KEY_MINUS && k.mods == ALIA_KMOD_NONE)
            {
                demo_scale /= 1.1f;
                alia_input_acknowledge_key_event(&ctx);
            }
        }

        // Initialize local style structs from library defaults once.
        if (!local_styles_initialized)
        {
            local_switch_style = *alia_default_switch_style();
            local_slider_style = *alia_default_slider_style();
            local_radio_style = *alia_default_radio_style();
            local_checkbox_style = *alia_default_checkbox_style();
            local_node_expander_style = *alia_default_node_expander_style();
            local_styles_initialized = true;
        }

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
                with_ui_scale(ctx, demo_scale, [&] {
                    with_spacing(ctx, demo_spacing, [&] {
                        concrete_panel(
                            ctx,
                            0,
                            ctx.palette->foundation.background.base,
                            GROW,
                            [&]() {
                                column(ctx, GROW, [&]() {
                                    alia_ui_scroll_view_begin(
                                        &ctx, ALIA_GROW, 0x3, 0, nullptr);
                                    edge_offsets(
                                        ctx,
                                        {.left = 40,
                                         .right = 40,
                                         .top = 40,
                                         .bottom = 40},
                                        [&]() { do_content(ctx); });
                                    alia_ui_scroll_view_end(&ctx);
                                });
                            });
                    });
                });
            });
        });
    }
    catch (pass_aborted&)
    {
    }
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
    static std::chrono::time_point<std::chrono::high_resolution_clock>
        last_frame_time = std::chrono::high_resolution_clock::now();
    auto const start_time = std::chrono::high_resolution_clock::now();

    AllocProbeResult result
        = probe_allocations([&]() { alia_app_shell_frame(the_system); });

    auto const end_time = std::chrono::high_resolution_clock::now();
    // auto const refresh_time = std::chrono::duration_cast<
    //     std::chrono::duration<int64_t, std::micro>>(
    //     refresh_finished_time - start_time);
    // auto const layout_time = std::chrono::duration_cast<
    //     std::chrono::duration<int64_t, std::micro>>(
    //     layout_finished_time - refresh_finished_time);
    // auto const render_time = std::chrono::duration_cast<
    //     std::chrono::duration<int64_t, std::micro>>(
    //     end_time - layout_finished_time);
    auto const frame_time = std::chrono::duration_cast<
        std::chrono::duration<int64_t, std::micro>>(end_time - start_time);

    // auto const external_frame_time = std::chrono::duration_cast<
    //     std::chrono::duration<int64_t, std::micro>>(
    //     start_time - last_frame_time);
    std::cout << "frame_time: " << std::setw(6) << frame_time.count() << "us"
              << std::endl;

    // std::cout
    //     << "frame_time: " // << std::setw(6) << external_frame_time <<
    //     ": "
    //     << std::setw(6) << frame_time << ": " << std::setw(6) <<
    //     refresh_time
    //     << " / " << std::setw(6) << layout_time << " / " << std::setw(6)
    //     << render_time << std::endl;

    // std::cout << "allocation count: " << result.count << std::endl;

    last_frame_time = start_time;
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
