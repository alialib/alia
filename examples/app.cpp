// #define WIN32_LEAN_AND_MEAN
// #include <windows.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <utility>

#include "alia_fonts.h"

#include <alia/renderers/gl/renderer.hpp>
#include <vector>

#include <alia/abi/base/arena.h>
#include <alia/abi/base/color.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/events.h>
#include <alia/abi/ui/drawing.h>
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
#include <alia/abi/ui/system/input_processing.h>
#include <alia/base/color.hpp>
#include <alia/context.h>
#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>
#include <alia/kernel/flow/dispatch.h>
#include <alia/kernel/macros.hpp>
#include <alia/platforms/glfw/window.hpp>
#include <alia/ui/drawing.h>
#include <alia/ui/layout/components.hpp>
#include <alia/ui/layout/flags.hpp>
#include <alia/ui/library.hpp>
#include <alia/ui/system/object.h>

#include <chrono>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

using namespace alia;
using namespace alia::operators;

constexpr rgba GRAY = {0.5f, 0.5f, 0.5f, 1.0f};

static alia_srgb8 const primary_colors[] = {
    hex_color("94c1fd"), // hex_color("#154DCF"),
    hex_color("#6f42c1"),
    hex_color("#a52e45"),
};
static int primary_index = 0;

alia_ui_system* the_system;
GLFWwindow* the_window;
gl_renderer the_renderer;
alia_arena the_display_list_arena;
alia_msdf_text_engine* the_msdf_text_engine;
alia_style the_style = {.spacing = 10.0f};
float the_time = 0.0f;

// Local palette and styles for the content pane (driven by controls).
static alia_palette local_palette;
static alia_switch_style local_switch_style;
static alia_slider_style local_slider_style;
static alia_radio_style local_radio_style;
static alia_checkbox_style local_checkbox_style;
static alia_node_expander_style local_node_expander_style;
static bool local_styles_initialized = false;
static float demo_hue = 0.55f;
static bool demo_is_dark = true;
static float demo_foundation_step_l = 0.075f;
static float demo_spacing = 6.f;
static float demo_scale = 1.0f;
static float demo_node_expander_triangle_side = 24.f;

#include "prototyping/allocation_probe.h"
#include "prototyping/layout_mods.h"
#include "prototyping/msdf.h"
#include "prototyping/panel.h"
#include "prototyping/rect.h"

template<class Content>
void
button(
    context& ctx,
    alia_z_index z_index,
    alia_rgba color,
    layout_flag_set flags,
    Content&& content)
{
    placement_hook(ctx, flags, [&](auto const& placement) {
        if (get_event_type(ctx) == ALIA_EVENT_DRAW)
        {
            alia_draw_rounded_box(&ctx, z_index, placement.box, color, 0.0f);
        }

        std::forward<Content>(content)();
    });
}

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
      "metus eget diam. Aenean sit amet posuere metus. In hac habitasse "
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

#include "prototyping/demos.h"

void
do_heading(context& ctx, char const* text)
{
    do_text(
        ctx,
        2,
        alia_srgba8_from_srgb8(ctx.palette->foundation.text.stronger_2),
        alia_px(&ctx, 14),
        text,
        NO_FLAGS,
        1);
}

void
do_subheading(context& ctx, char const* text)
{
    do_text(
        ctx,
        2,
        alia_srgba8_from_srgb8(ctx.palette->foundation.text.stronger_1),
        alia_px(&ctx, 12),
        text,
        NO_FLAGS,
        1);
}

void
do_radio_with_text(
    context& ctx,
    alia_bool_signal* value,
    char const* text,
    alia_radio_style const* style)
{
    // TODO: The hook gets the full placement, before the alignment is applied.
    // (But also the hook is only supposed to have a single child.)
    placement_hook(ctx, [&](auto const& placement) {
        row(ctx, ALIGN_LEFT, [&]() {
            alia_element_id id
                = alia_do_radio(&ctx, value, ALIA_CENTER_Y, style);
            do_text(
                ctx,
                2,
                alia_srgba8_from_srgb8(
                    value->flags & ALIA_SIGNAL_WRITABLE
                        ? ctx.palette->foundation.text.base
                        : ctx.palette->foundation.text.weaker_2),
                alia_px(&ctx, 12),
                text,
                CENTER_Y);
            alia_element_box_region(
                &ctx, id, &placement.box, ALIA_CURSOR_DEFAULT);
        });
    });
}

void
do_checkbox_with_text(
    context& ctx,
    alia_bool_signal* value,
    char const* text,
    alia_checkbox_style const* style)
{
    placement_hook(ctx, [&](auto const& placement) {
        row(ctx, ALIGN_LEFT, [&]() {
            alia_element_id id
                = alia_do_checkbox(&ctx, value, ALIA_CENTER_Y, style);
            do_text(
                ctx,
                2,
                alia_srgba8_from_srgb8(
                    value->flags & ALIA_SIGNAL_WRITABLE
                        ? ctx.palette->foundation.text.base
                        : ctx.palette->foundation.text.weaker_2),
                alia_px(&ctx, 12),
                text,
                CENTER_Y);
            alia_element_box_region(
                &ctx, id, &placement.box, ALIA_CURSOR_DEFAULT);
        });
    });
}

void
do_switch_with_text(
    context& ctx,
    alia_bool_signal* value,
    char const* text,
    alia_switch_style const* style)
{
    // TODO: The hook gets the full placement, before the alignment is applied.
    // (But also the hook is only supposed to have a single child.)
    placement_hook(ctx, [&](auto const& placement) {
        row(ctx, ALIGN_LEFT, [&]() {
            alia_element_id id
                = alia_do_switch(&ctx, value, ALIA_CENTER_Y, style);
            do_text(
                ctx,
                2,
                alia_srgba8_from_srgb8(
                    value->flags & ALIA_SIGNAL_WRITABLE
                        ? ctx.palette->foundation.text.base
                        : ctx.palette->foundation.text.weaker_2),
                alia_px(&ctx, 12),
                text,
                CENTER_Y);
            alia_element_box_region(
                &ctx, id, &placement.box, ALIA_CURSOR_DEFAULT);
        });
    });
}

void
do_node_expander_with_text(
    context& ctx,
    alia_bool_signal* value,
    char const* text,
    alia_node_expander_style const* style)
{
    // TODO: The hook gets the full placement, before the alignment is applied.
    // (But also the hook is only supposed to have a single child.)
    placement_hook(ctx, [&](auto const& placement) {
        row(ctx, ALIGN_LEFT, [&]() {
            alia_element_id id
                = alia_do_node_expander(&ctx, value, ALIA_CENTER_Y, style);
            do_text(
                ctx,
                2,
                alia_srgba8_from_srgb8(
                    value->flags & ALIA_SIGNAL_WRITABLE
                        ? ctx.palette->foundation.text.base
                        : ctx.palette->foundation.text.weaker_2),
                alia_px(&ctx, 12),
                text,
                CENTER_Y);
            alia_element_box_region(
                &ctx, id, &placement.box, ALIA_CURSOR_DEFAULT);
        });
    });
}

void
do_controls(context& ctx)
{
    do_heading(ctx, "PALETTE");

    {
        alia_bool_signal switch_signal{
            .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
            .value = demo_is_dark,
        };
        do_switch_with_text(ctx, &switch_signal, "Dark Mode", nullptr);
        if (switch_signal.flags & ALIA_SIGNAL_WRITTEN)
        {
            demo_is_dark = switch_signal.value;
            abort_pass(ctx);
        }
    }

    do_subheading(ctx, "Foundation Lightness Step");
    alia_do_slider_f(
        &ctx, &demo_foundation_step_l, 0.01f, 0.2f, 0.001f, 0, false, nullptr);

    do_subheading(ctx, "Hue");
    alia_do_slider_f(&ctx, &demo_hue, 0.f, 1.f, 0.01f, 0, false, nullptr);

    do_heading(ctx, "GEOMETRY");

    do_subheading(ctx, "Spacing");
    alia_do_slider_f(&ctx, &demo_spacing, 0.f, 24.f, 1.f, 0, false, nullptr);

    do_subheading(ctx, "Scale");
    alia_do_slider_f(&ctx, &demo_scale, 0.1f, 3.0f, 0.001f, 0, false, nullptr);

    do_subheading(ctx, "Node Expander");
    alia_do_slider_f(
        &ctx,
        &demo_node_expander_triangle_side,
        12.f,
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
            do_text(
                ctx,
                2,
                alia_srgba8_from_srgb8(ctx.palette->foundation.text.base),
                alia_px(&ctx, 12),
                lorem_ipsum);
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
        do_heading(ctx, "TEXT");
        flow(ctx, FILL, [&]() {
            do_text(
                ctx,
                2,
                alia_srgba8_from_srgb8(ctx.palette->foundation.text.base),
                alia_px(&ctx, 12),
                lorem_ipsum);
            do_text(
                ctx,
                2,
                alia_srgba8_from_srgb8(ctx.palette->foundation.text.base),
                alia_px(&ctx, 12),
                lorem_ipsum,
                NO_FLAGS,
                1);
            do_text(
                ctx,
                2,
                alia_srgba8_from_srgb8(ctx.palette->foundation.text.base),
                alia_px(&ctx, 12),
                lorem_ipsum);
        });
    });
}

void
the_demo(context& ctx)
{
    try
    {
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

        // Refresh local palette from control state (hue, light/dark).
        static bool current_demo_is_dark = false;
        static float current_demo_foundation_step_l = 0;
        static float current_demo_hue = 0;
        if (current_demo_is_dark != demo_is_dark
            || current_demo_foundation_step_l != demo_foundation_step_l
            || current_demo_hue != demo_hue)
        {
            alia_oklch lch = {
                .l = 0.55f,
                .c = 0.2f,
                .h = demo_hue * 2.f * 3.14159f,
            };
            alia_srgb8 primary = alia_srgb8_from_unclamped_oklch(lch);
            alia_palette_seeds pseeds
                = alia_seeds_from_elevation(primary, 0, demo_is_dark);
            alia_theme_params params = {
                .foundation_step_l = demo_foundation_step_l,
                .is_dark_mode = demo_is_dark,
            };
            alia_palette_expand(&local_palette, &pseeds, &params);

            current_demo_is_dark = demo_is_dark;
            current_demo_foundation_step_l = demo_foundation_step_l;
            current_demo_hue = demo_hue;
        }

        with_spacing(ctx, 0, [&] {
            row(ctx, [&]() {
                concrete_panel(
                    ctx,
                    0,
                    ctx.palette->foundation.background.stronger_2,
                    FILL,
                    [&]() {
                        column(ctx, [&]() {
                            inset(
                                ctx,
                                {.left = 40,
                                 .right = 40,
                                 .top = 40,
                                 .bottom = 40},
                                [&]() {
                                    with_spacing(ctx, 6, [&] {
                                        column(
                                            ctx, [&]() { do_controls(ctx); });
                                    });
                                });
                        });
                    });
                with_palette(ctx, &local_palette, [&] {
                    with_ui_scale(ctx, demo_scale, [&] {
                        with_spacing(ctx, demo_spacing, [&] {
                            concrete_panel(
                                ctx,
                                0,
                                ctx.palette->foundation.background.base,
                                FILL,
                                [&]() {
                                    column(ctx, GROW, [&]() {
                                        alia_ui_scroll_view_begin(
                                            &ctx, ALIA_GROW, 0x3, 0, nullptr);
                                        inset(
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
        });
    }
    catch (pass_aborted&)
    {
    }
}

alia_kmods_t
to_alia_kmods_t(int mods)
{
    alia_kmods_t result = 0;
    if (mods & GLFW_MOD_SHIFT)
        result |= ALIA_KMOD_SHIFT;
    if (mods & GLFW_MOD_CONTROL)
        result |= ALIA_KMOD_CTRL;
    if (mods & GLFW_MOD_ALT)
        result |= ALIA_KMOD_ALT;
    // TODO: Finish this.
    return result;
}

void
mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    float internal_x
        = (static_cast<float>(x) * framebuffer_width
           / window_width); // / the_ui_scale.x;
    float internal_y
        = (static_cast<float>(y) * framebuffer_height
           / window_height); // / the_ui_scale.y;

    switch (action)
    {
        case GLFW_PRESS: {
            alia_ui_process_mouse_press(
                the_system,
                {internal_x, internal_y},
                button,
                to_alia_kmods_t(mods));
            break;
        }
        case GLFW_RELEASE: {
            alia_ui_process_mouse_release(
                the_system,
                {internal_x, internal_y},
                button,
                to_alia_kmods_t(mods));
            break;
        }
    }
}
void
cursor_position_callback(GLFWwindow* window, double x, double y)
{
    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    float internal_x
        = (static_cast<float>(x) * framebuffer_width / window_width);
    float internal_y
        = (static_cast<float>(y) * framebuffer_height / window_height);

    alia_ui_process_mouse_motion(the_system, {internal_x, internal_y});
}

void
scroll_callback(GLFWwindow* window, double x, double y)
{
    alia_ui_process_scroll(
        the_system, {static_cast<float>(x), static_cast<float>(y)});
}

void
update()
{
    static std::chrono::time_point<std::chrono::high_resolution_clock>
        last_frame_time = std::chrono::high_resolution_clock::now();
    auto const start_time = std::chrono::high_resolution_clock::now();

    AllocProbeResult result = probe_allocations([&]() {
        alia_ui_system_update(the_system);

        update_glfw_window_info(the_system, the_window);

        // glfwMakeContextCurrent(the_window);

        alia_rgb c = alia_rgb_from_srgb8(
            the_system->palette.foundation.background.base);
        glClearColor(c.r, c.g, c.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
            printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

        alia_draw_bucket_table bucket_table = {
            .buckets = {},
            .keys = {},
        };
        alia_draw_context draw_context = {
            .buckets = &bucket_table,
            .arena = {},
        };
        alia_bump_allocator_init(&draw_context.arena, &the_display_list_arena);

        auto draw_event = alia_make_draw_event({.context = &draw_context});
        dispatch_event(*the_system, draw_event);

        while ((err = glGetError()) != GL_NO_ERROR)
            printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

        std::sort(bucket_table.keys.begin(), bucket_table.keys.end());
        for (auto const key : bucket_table.keys)
        {
            alia_clip_id clip_id = (key >> 16) & 0xffff;
            alia_draw_material_id material_id = key & 0xffff;
            alia_z_index z_index = key >> 32;
            alia_draw_bucket* bucket = &bucket_table.buckets[key];
            alia_draw_material* material
                = &the_system->draw.materials[material_id];
            material->vtable.draw_bucket(material->user, bucket);
        }

        while ((err = glGetError()) != GL_NO_ERROR)
            printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    });

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

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glfwSwapBuffers(the_window);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);
}

void
framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    alia_ui_surface_set_size(the_system, {width, height});
    update();
}

void
content_scale_callback(GLFWwindow* window, float xscale, float yscale)
{
    alia_ui_surface_set_dpi(the_system, ((xscale + yscale) / 2.0f) * 96.f);
    // TODO: Support 2D DPI?
    update();
}

void
check_and_update_resolution()
{
#ifdef __EMSCRIPTEN__
    // 1. Get the current size of the HTML element (in CSS pixels)
    //    This is the size the "window" takes up on the webpage.
    double css_w, css_h;
    emscripten_get_element_css_size("#canvas", &css_w, &css_h);

    // 2. Get the Device Pixel Ratio (e.g., 2.0 for Retina/High-DPI)
    double dpr = emscripten_get_device_pixel_ratio();

    // 3. Calculate the required Buffer Size (Physical Pixels)
    int target_w = (int) (css_w * dpr);
    int target_h = (int) (css_h * dpr);

    // 4. Check what GLFW thinks the size is
    int current_w, current_h;
    glfwGetWindowSize(the_window, &current_w, &current_h);

    // 5. If they mismatch, Resize!
    if (current_w != target_w || current_h != target_h)
    {
        // This resizes the WebGL Backbuffer
        glfwSetWindowSize(the_window, target_w, target_h);

        // Update your Alia System / Renderer Viewport
        // (Assuming you do this in your render code, but if you cache it,
        // update it here)
        the_system->surface_size = {target_w, target_h};

        // Helpful log to prove it's working
        std::cout << "Resized to: " << target_w << "x" << target_h
                  << " (DPR: " << dpr << ")\n";
    }
#endif
}

void
main_loop_step()
{
    // Check if we need to close (mostly for desktop)
    if (glfwWindowShouldClose(the_window))
    {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop(); // Stop the loop
#endif
        return;
    }

    glfwPollEvents();

    check_and_update_resolution();

    update();

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);
}

int
main()
{
    static bool light_theme = false;

    // TODO: Guarantee alignment.
    void* ui_system_storage = malloc(alia_ui_system_object_spec().size);
    the_system = alia_ui_system_init(ui_system_storage, the_demo, {0, 0});

    static bool theme_initialized = false;
    if (!theme_initialized)
    {
        alia_palette_seeds pseeds = alia_seeds_from_elevation(
            primary_colors[primary_index], 0, !light_theme);
        // pseeds.bg_base = alia_srgb8{0x32, 0x33, 0x39};
        alia_theme_params params = {
            .foundation_step_l = 0.075f,
            .is_dark_mode = !light_theme,
        };
        alia_palette_expand(&the_system->palette, &pseeds, &params);
        theme_initialized = true;
    }

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

#ifndef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
    // Emscripten handles this via linker flags (-s USE_WEBGL2=1)
    // But explicitly asking for ES 3.0 doesn't hurt:
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    the_window
        = glfwCreateWindow(1200, 1200, "Alia Renderer", nullptr, nullptr);
    if (!the_window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwSetMouseButtonCallback(the_window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(the_window, framebuffer_size_callback);
    glfwSetCursorPosCallback(the_window, cursor_position_callback);
    glfwSetScrollCallback(the_window, scroll_callback);

    float xscale, yscale;
    glfwGetWindowContentScale(the_window, &xscale, &yscale);
    // TODO: Support 2D DPI?
    alia_ui_surface_set_dpi(the_system, ((xscale + yscale) / 2.0f) * 96.f);

    glfwSetWindowContentScaleCallback(the_window, content_scale_callback);

#ifndef __EMSCRIPTEN__
    glfwMakeContextCurrent(the_window);
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }
#endif

    {
        int x, y;
        glfwGetFramebufferSize(the_window, &x, &y);
        alia_ui_surface_set_size(the_system, {x, y});
    }

#ifndef __EMSCRIPTEN__
    glEnable(GL_FRAMEBUFFER_SRGB);
#endif

    init_gl_renderer(the_system, &the_renderer);
    alia_material_register(
        the_system,
        ALIA_PRIMITIVE_MATERIAL_ID,
        alia_material_vtable{
            .draw_bucket = render_primitive_command_list,
        },
        &the_renderer);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    std::vector<std::uint8_t> atlas_rgb(
        static_cast<std::size_t>(alia_atlas_decompressed_size));
    alia_msdf_atlas_rle_decompress(
        alia_atlas_rle_r,
        alia_atlas_rle_r_size,
        alia_atlas_rle_g,
        alia_atlas_rle_g_size,
        alia_atlas_rle_b,
        alia_atlas_rle_b_size,
        atlas_rgb.data(),
        atlas_rgb.size());
    gl_renderer_upload_msdf_atlas(
        &the_renderer, atlas_rgb.data(), alia_atlas_width, alia_atlas_height);
    the_msdf_text_engine = alia_msdf_create_text_engine(
        alia_font_descriptions, alia_font_count);
    the_system->msdf_text_engine = the_msdf_text_engine;

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    initialize_lazy_commit_arena(&the_display_list_arena);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop_step, 0, 1);
#else
    while (!glfwWindowShouldClose(the_window))
        main_loop_step();
#endif

    glfwTerminate();
    return 0;
}
