#include <cstdio>
#include <iomanip>
#include <iostream>
#include <numbers>

#include <alia/shell/app.h>

#include <alia/abi/base/color.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/geometry/api.h>
#include <alia/abi/ui/styling/context_api.h>
#include <alia/abi/ui/styling/environment.h>
#include <alia/abi/ui/styling/descriptors.h>
#include <alia/abi/ui/styling/palette_generation.h>
#include <alia/abi/ui/styling/style_catalog.h>
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
#include <alia/ui/styling/descriptors_internal.h>
#include <alia/ui/system/object.h>
#include <alia/ui/text.hpp>

using namespace alia;
using namespace alia::operators;

// Gallery palette sources - Generated uses `palette_generate` with a live
// recipe. Wine / Neon / Ink import Coolors-style swatch rows.
enum gallery_scheme
{
    GALLERY_SCHEME_GENERATED = 0,
    GALLERY_SCHEME_WINE,
    GALLERY_SCHEME_NEON,
    GALLERY_SCHEME_INK,
    GALLERY_SCHEME_COUNT
};

static int scheme_index = GALLERY_SCHEME_GENERATED;
static bool light_theme = false;
static bool theme_dirty = false;

// Panel B style floats - Survives palette regeneration. Reset copies
// light/dark defaults for the current mode.
static float gallery_floats[ALIA_STYLE_VAR_COUNT]{};
static bool gallery_floats_ready = false;

// Panel A generation recipe - Used only by Generated mode. Mode-symmetric; the
// light/dark toggle only changes the generate flag.
static alia_palette_recipe gallery_recipe{};
static bool gallery_recipe_ready = false;

alia_ui_system* the_system;
static alia_ui_tracer the_tracer;

static float
gallery_hue_degrees_of(alia_srgb8 color)
{
    float deg
        = alia_oklch_from_srgb8(color).h * 180.f / std::numbers::pi_v<float>;
    while (deg < 0.f)
        deg += 360.f;
    while (deg >= 360.f)
        deg -= 360.f;
    return deg;
}

static void
gallery_load_float_defaults(bool dark)
{
    alia_style_environment scratch{};
    alia::style_vars_apply_core_defaults(&scratch, dark);
    for (int i = 0; i < ALIA_STYLE_VAR_COUNT; ++i)
        gallery_floats[i] = scratch.vars[i];
    gallery_floats_ready = true;
}

static void
gallery_ensure_floats(bool dark)
{
    if (!gallery_floats_ready)
        gallery_load_float_defaults(dark);
}

static void
gallery_ensure_recipe()
{
    if (!gallery_recipe_ready)
    {
        gallery_recipe = alia_palette_recipe_default();
        gallery_recipe_ready = true;
    }
}

static void
gallery_apply_floats_to_palette()
{
    for (int i = 0; i < ALIA_STYLE_VAR_COUNT; ++i)
        the_system->styling.environment.vars[i] = gallery_floats[i];
}

static void
gallery_paint_floats_changed()
{
    gallery_apply_floats_to_palette();
    alia_ui_paint_changed(the_system);
}

// Build a palette from a Coolors-style swatch row via `palette_from_swatches`.
static void
gallery_fill_from_swatches(
    alia_palette* out, bool dark, alia_srgb8 const* colors, int count)
{
    alia_palette_from_swatches(out, colors, count, dark, nullptr);
}

static void
apply_gallery_theme()
{
    bool const dark = !light_theme;
    gallery_ensure_floats(dark);
    gallery_ensure_recipe();
    switch (scheme_index)
    {
        case GALLERY_SCHEME_NEON: {
            // Coolors-style punchy row with magenta, violet, cyan, and
            // neutrals.
            alia_srgb8 const swatches[] = {
                hex_color("#ff2e63"),
                hex_color("#8338ec"),
                hex_color("#08d9d6"),
                hex_color("#252a34"),
                hex_color("#eaeaea"),
            };
            gallery_fill_from_swatches(
                &the_system->styling.environment.palette, dark, swatches, 5);
            break;
        }
        case GALLERY_SCHEME_INK: {
            // Near-monochrome cool gray ramp with a soft blue accent.
            alia_srgb8 const swatches[] = {
                hex_color("#778da9"),
                hex_color("#415a77"),
                hex_color("#1b263b"),
                hex_color("#0d1b2a"),
                hex_color("#e0e1dd"),
            };
            gallery_fill_from_swatches(
                &the_system->styling.environment.palette, dark, swatches, 5);
            break;
        }
        case GALLERY_SCHEME_WINE: {
            // Warm swatch row mapped to primary / secondary / tertiary plus
            // neutrals.
            alia_srgb8 const swatches[] = {
                hex_color("#a52e45"),
                hex_color("#c97b63"),
                hex_color("#e8b86d"),
                hex_color("#2b1d1f"),
                hex_color("#f5ebe0"),
            };
            gallery_fill_from_swatches(
                &the_system->styling.environment.palette, dark, swatches, 5);
            break;
        }
        case GALLERY_SCHEME_GENERATED:
        default: {
            alia_palette_generate(
                &the_system->styling.environment.palette,
                &gallery_recipe,
                dark);
            break;
        }
    }
    // Preserve Panel B knobs across regeneration.
    gallery_apply_floats_to_palette();
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
            ctx, text, &demo_get_fonts().heading_18, DEMO_INK_STRONG, FLUSH);
    });
}

void
do_subheading(ui_context ctx, char const* text)
{
    with_vertical_insets(ctx, 14.f, 2.f, [&] {
        demo_text(
            ctx, text, &demo_get_fonts().heading_14, DEMO_INK_EMPHASIS, FLUSH);
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

// Emit a short label and a slider bound to `value`.
template<class Binding>
void
do_labeled_slider(
    ui_context ctx,
    char const* label,
    Binding const& value,
    double minimum,
    double maximum,
    double step)
{
    demo_text(ctx, label, &demo_get_fonts().body_14, DEMO_INK_MUTED, FLUSH);
    slider(ctx, value, minimum, maximum, step, FILL);
}

// Bind a gallery style float and refresh paint without regenerating roles.
static auto
gallery_float_binding(uint16_t index)
{
    return lambda_binding(
        [index] { return double(gallery_floats[index]); },
        [index](double value) {
            gallery_floats[index] = float(value);
            gallery_paint_floats_changed();
        });
}

// Bind a generation-recipe float by descriptor index and mark the theme dirty.
static auto
gallery_recipe_binding(uint16_t index)
{
    return lambda_binding(
        [index] {
            float const* base
                = reinterpret_cast<float const*>(&gallery_recipe);
            return double(base[index]);
        },
        [index](double value) {
            float* base = reinterpret_cast<float*>(&gallery_recipe);
            base[index] = float(value);
            theme_dirty = true;
        });
}

// Emit labeled sliders for each float descriptor.
template<class MakeBinding>
void
do_float_desc_sliders(
    ui_context ctx,
    alia_style_float_desc const* descs,
    size_t count,
    MakeBinding&& make_binding)
{
    for (size_t i = 0; i < count; ++i)
    {
        alia_style_float_desc const& d = descs[i];
        do_labeled_slider(
            ctx,
            d.meta.name,
            make_binding(d.index),
            double(d.min),
            double(d.max),
            double(d.step));
    }
}

static void
gallery_snap_hue(float degrees)
{
    gallery_ensure_recipe();
    alia_palette_recipe_seed_hue(&gallery_recipe, degrees);
    theme_dirty = true;
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

    do_subheading(ctx, "Palette");
    auto scheme = lambda_binding(
        [] { return scheme_index; },
        [](int value) {
            scheme_index = value;
            theme_dirty = true;
        });
    radio_button(ctx, scheme, GALLERY_SCHEME_GENERATED, "Generated");
    radio_button(ctx, scheme, GALLERY_SCHEME_WINE, "Wine (swatch)");
    radio_button(ctx, scheme, GALLERY_SCHEME_NEON, "Neon (swatch)");
    radio_button(ctx, scheme, GALLERY_SCHEME_INK, "Ink (swatch)");

    alia_if (scheme_index == GALLERY_SCHEME_GENERATED)
    {
        do_subheading(ctx, "Generation");
        {
            size_t recipe_count = 0;
            alia_style_float_desc const* recipe_descs
                = alia_style_recipe_descs(&recipe_count);
            do_float_desc_sliders(
                ctx, recipe_descs, recipe_count, [](uint16_t index) {
                    return gallery_recipe_binding(index);
                });
        }
        // Prefer row over flow here: outline chrome + label still fragment
        // incorrectly inside a wrapping flow. Hue is frozen via factory
        // outline recipes so chips keep their named color as the theme moves.
        row(ctx, ALIGN_LEFT, [&]() {
            float const sky = gallery_hue_degrees_of(hex_color("94c1fd"));
            float const violet = gallery_hue_degrees_of(hex_color("#6f42c1"));
            float const teal = gallery_hue_degrees_of(hex_color("#0d9488"));
            with_button_variant(
                ctx,
                {.chrome = button_chrome::outline,
                 .hue = channel_value(sky),
                 .chroma = channel_from_slot(ALIA_STYLE_COLOR_PRIMARY)},
                [&] {
                    button(ctx, "Sky", callback([sky] { gallery_snap_hue(sky); }));
                });
            with_button_variant(
                ctx,
                {.chrome = button_chrome::outline,
                 .hue = channel_value(violet),
                 .chroma = channel_from_slot(ALIA_STYLE_COLOR_PRIMARY)},
                [&] {
                    button(ctx, "Violet", callback([violet] {
                               gallery_snap_hue(violet);
                           }));
                });
            with_button_variant(
                ctx,
                {.chrome = button_chrome::outline,
                 .hue = channel_value(teal),
                 .chroma = channel_from_slot(ALIA_STYLE_COLOR_PRIMARY)},
                [&] {
                    button(
                        ctx, "Teal", callback([teal] { gallery_snap_hue(teal); }));
                });
        });
    }
    alia_end

    do_subheading(ctx, "Style Floats");
    {
        size_t var_count = 0;
        alia_style_float_desc const* var_descs
            = alia_style_var_descs(the_system, &var_count);
        do_float_desc_sliders(ctx, var_descs, var_count, [](uint16_t index) {
            return gallery_float_binding(index);
        });
    }
    button(
        ctx,
        "Reset style floats",
        callback([] {
            gallery_load_float_defaults(!light_theme);
            gallery_paint_floats_changed();
        }),
        {
             .chrome = button_chrome::outline,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL)});

    do_subheading(ctx, "Spacing");
    slider(ctx, spacing, 0.0, 24.0, 1.0, FILL);

    do_subheading(ctx, "Magnification");
    slider(ctx, magnification, 0.125, 5.0, 0.001, FILL);
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
    radio_button(ctx, disable_writes(radio_index), 0, "Disabled One");
    radio_button(ctx, disable_writes(radio_index), 1, "Disabled Two");
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
                        DEMO_INK_BODY,
                        FLUSH);
                });
            });
        });
    });
}

// Append `rest_suffix` onto every paint/label rest channel of `style`,
// re-emitting into `emitter`. Continuations are copied as-is. Call with the
// pre-patch `style->steps` as `src` before retargeting.
inline void
patch_button_style_rest(
    alia_button_style* style,
    alia_color_program_step const* src,
    alia_color_program_emitter* emitter,
    color::color_program_expr const& rest_suffix)
{
    using namespace color;
    style->paint.fill = emit_responsive_append_rest(
        emitter, src, style->paint.fill, rest_suffix);
    style->paint.fill_end = emit_responsive_append_rest(
        emitter, src, style->paint.fill_end, rest_suffix);
    style->paint.border = emit_responsive_append_rest(
        emitter, src, style->paint.border, rest_suffix);
    style->paint.border_end = emit_responsive_append_rest(
        emitter, src, style->paint.border_end, rest_suffix);
    style->label
        = emit_responsive_append_rest(emitter, src, style->label, rest_suffix);
    style->steps = alia_color_program_emitter_steps(emitter);
}

// Run `content` while the active button style is a copy of the current one
// with `rest_suffix` appended to each rest program.
template<class Content>
void
with_patched_button_style(
    ui_context ctx,
    color::color_program_expr const& rest_suffix,
    Content&& content)
{
    alia_button_style* style = alia_button_style_active(ctx);
    alia_button_style const saved = *style;
    alia_color_program_step const* src = saved.steps;
    alia_color_program_emitter emitter = alia_color_program_emitter_begin(
        alia_ctx_scratch(alia_ctx_kernel(ctx)));
    *style = saved;
    patch_button_style_rest(style, src, &emitter, rest_suffix);
    std::forward<Content>(content)();
    *alia_button_style_active(ctx) = saved;
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
        demo_text(ctx, label, &demo_get_fonts().body_14, DEMO_INK_BODY);
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
        demo_text(ctx, label, &demo_get_fonts().body_14, DEMO_INK_BODY);
    }

    row(ctx, ALIGN_LEFT, [&]() {
        button(ctx, "n += 1", n += 1);
        button(ctx, "m <<= n", m <<= n);
        button(ctx, "n <<= empty", n <<= empty<int>());
    });

    do_subheading(ctx, "Roles");
    row(ctx, ALIGN_LEFT, [&]() {
        button(ctx, "Primary", actions::noop());
        button(
            ctx, "Secondary", actions::noop(), {
             .hue = channel_from_slot(ALIA_STYLE_COLOR_SECONDARY),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_SECONDARY)});
        button(ctx, "Tertiary", actions::noop(), {
             .hue = channel_from_slot(ALIA_STYLE_COLOR_TERTIARY),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_TERTIARY)});
        button(ctx, "Neutral", actions::noop(), {
             .hue = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL)});
        button(ctx, "Danger", actions::noop(), {
             .hue = channel_from_slot(ALIA_STYLE_COLOR_DANGER),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_DANGER)});
    });

    do_subheading(ctx, "Tone");
    row(ctx, ALIGN_LEFT, [&]() {
        button(
            ctx,
            "Match",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::match});
        button(
            ctx,
            "Inverse",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::inverse});
        button(
            ctx,
            "Dark",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::dark});
        button(
            ctx,
            "Light",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::light});
    });
    row(ctx, ALIGN_LEFT, [&]() {
        button(
            ctx,
            "Neutral Dark",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::dark,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL)});
        button(
            ctx,
            "Neutral Light",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::light,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL)});
        button(
            ctx,
            "Danger Dark",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::dark,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_DANGER),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_DANGER)});
        button(
            ctx,
            "Tertiary Dark",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::dark,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_TERTIARY),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_TERTIARY)});
        button(
            ctx,
            "Tertiary Light",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::light,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_TERTIARY),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_TERTIARY)});
    });

    do_subheading(ctx, "Outline");
    row(ctx, ALIGN_LEFT, [&]() {
        button(
            ctx,
            "Primary",
            actions::noop(),
            {.chrome = button_chrome::outline});
        button(
            ctx,
            "Secondary",
            actions::noop(),
            {
             .chrome = button_chrome::outline,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_SECONDARY),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_SECONDARY)});
        button(
            ctx,
            "Neutral",
            actions::noop(),
            {
             .chrome = button_chrome::outline,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL)});
        button(
            ctx,
            "Danger",
            actions::noop(),
            {
             .chrome = button_chrome::outline,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_DANGER),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_DANGER)});
    });

    do_subheading(ctx, "Gradient");
    row(ctx, ALIGN_LEFT, [&]() {
        button(
            ctx,
            "Primary",
            actions::noop(),
            {.chrome = button_chrome::gradient});
        button(
            ctx,
            "Danger",
            actions::noop(),
            {
             .chrome = button_chrome::gradient,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_DANGER),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_DANGER)});
        button(
            ctx,
            "Tertiary Dark",
            actions::noop(),
            {
             .chrome = button_chrome::gradient,
             .tone = color::tone::dark,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_TERTIARY),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_TERTIARY)});
    });

    do_subheading(ctx, "Style patch");
    demo_text(
        ctx,
        "Decorator path: copy the active style and append match_hue(danger).",
        &demo_get_fonts().body_14,
        DEMO_INK_MUTED,
        FLUSH);
    row(ctx, ALIGN_LEFT, [&]() {
        button(ctx, "Ambient", actions::noop());
        button(
            ctx,
            "Outline",
            actions::noop(),
            {.chrome = button_chrome::outline});
    });
    with_patched_button_style(
        ctx,
        color::color_program_expr{} | color::match_hue(color::danger),
        [&] {
            // Stay on the ambient style here; a nested variant would
            // regenerate and drop the patch.
            button(ctx, "Patched ambient", actions::noop());
        });
    // Patch on top of an already-selected variant (opaque base).
    with_button_variant(
        ctx,
        {
             .chrome = button_chrome::filled,
             .tone = color::tone::inverse,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_SECONDARY),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_SECONDARY)},
        [&] {
            with_patched_button_style(
                ctx,
                color::color_program_expr{} | color::match_hue(color::danger),
                [&] { button(ctx, "Inverse + danger hue", actions::noop()); });
        });
    with_button_variant(
        ctx, {.chrome = button_chrome::outline}, [&] {
            with_patched_button_style(
                ctx,
                color::color_program_expr{} | color::match_hue(color::danger),
                [&] { button(ctx, "Outline + danger hue", actions::noop()); });
        });
}

// Temporarily elevate the active palette toward TEXT for `content`.
template<class Content>
void
with_elevated_palette(ui_context ctx, float delta_l, Content&& content)
{
    alia_palette* palette = &alia_ctx_style(ctx)->environment->palette;
    alia_palette const base = *palette;
    alia_palette_elevate(palette, &base, delta_l);
    std::forward<Content>(content)();
    *palette = base;
}

void
do_elevation_demo(ui_context ctx)
{
    do_heading(ctx, "ELEVATION");
    demo_text(
        ctx,
        "Same controls on SURFACE raised toward TEXT. Match chrome should "
        "stay quiet on each level.",
        &demo_get_fonts().body_14,
        DEMO_INK_MUTED,
        FLUSH);

    auto controls = [&](char const* label) {
        demo_text(
            ctx, label, &demo_get_fonts().body_14, DEMO_INK_EMPHASIS, FLUSH);
        row(ctx, ALIGN_LEFT, [&]() {
            button(ctx, "Match", actions::noop());
            button(
                ctx,
                "Inverse",
                actions::noop(),
                {
             .chrome = button_chrome::filled,
             .tone = color::tone::inverse});
            checkbox(ctx, use_state(ctx, true), "On");
        });
    };

    concrete_panel(ctx, 0, demo_surface(ctx), FILL, [&]() {
        column(ctx, pad(12), [&]() {
            controls("Level 0");
            with_elevated_palette(ctx, 0.075f, [&] {
                concrete_panel(ctx, 0, demo_surface(ctx), FILL, [&]() {
                    column(ctx, pad(12), [&]() {
                        controls("Level +1");
                        with_elevated_palette(ctx, 0.075f, [&] {
                            concrete_panel(
                                ctx, 0, demo_surface(ctx), FILL, [&]() {
                                    column(ctx, pad(12), [&]() {
                                        controls("Level +2");
                                    });
                                });
                        });
                    });
                });
            });
        });
    });
}

void
do_status_chip_demo(ui_context ctx)
{
    do_heading(ctx, "STATUS");
    demo_text(
        ctx,
        "Hard chips use inverse/dark tones; soft chips stay near the page "
        "(match).",
        &demo_get_fonts().body_14,
        DEMO_INK_MUTED,
        FLUSH);

    do_subheading(ctx, "Hard");
    row(ctx, ALIGN_LEFT, [&]() {
        button(
            ctx,
            "Info",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::inverse});
        button(
            ctx,
            "Success",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::inverse,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_SECONDARY),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_SECONDARY)});
        button(
            ctx,
            "Warning",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::dark,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_TERTIARY),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_TERTIARY)});
        button(
            ctx,
            "Danger",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::inverse,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_DANGER),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_DANGER)});
        button(
            ctx,
            "Neutral",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::dark,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL)});
    });

    do_subheading(ctx, "Soft");
    row(ctx, ALIGN_LEFT, [&]() {
        button(
            ctx,
            "Info",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::match});
        button(
            ctx,
            "Success",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::match,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_SECONDARY),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_SECONDARY)});
        button(
            ctx,
            "Warning",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::match,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_TERTIARY),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_TERTIARY)});
        button(
            ctx,
            "Danger",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::match,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_DANGER),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_DANGER)});
        button(
            ctx,
            "Neutral",
            actions::noop(),
            {
             .chrome = button_chrome::filled,
             .tone = color::tone::match,
             .hue = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL),
             .chroma = channel_from_slot(ALIA_STYLE_COLOR_NEUTRAL)});
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
        demo_text(ctx, label, &demo_get_fonts().body_14, DEMO_INK_BODY);
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
                demo_raised(ctx),
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
        demo_text(ctx, lorem_ipsum, &demo_get_fonts().body_14, DEMO_INK_BODY);
        demo_text(
            ctx, lorem_ipsum, &demo_get_fonts().heading_14, DEMO_INK_BODY);
        demo_text(ctx, lorem_ipsum, &demo_get_fonts().body_14, DEMO_INK_BODY);
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
        do_status_chip_demo(ctx);
        do_section_separator(ctx);
        do_elevation_demo(ctx);
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
            column(ctx, width(340), [&] {
                concrete_panel(ctx, 0, demo_raised(ctx), GROW | FILL, [&]() {
                    scroll_view(ctx, GROW | FILL, [&]() {
                        with_spacing(ctx, 6, [&] {
                            column(ctx, pad(20), [&]() {
                                do_controls(
                                    ctx, spacing, magnification, typography);
                            });
                        });
                    });
                });
            });
            with_spacing(ctx, read_signal(spacing), [&] {
                concrete_panel(ctx, 0, demo_surface(ctx), GROW, [&]() {
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

    gallery_ensure_recipe();
    alia_palette_recipe_seed_color(&gallery_recipe, hex_color("94c1fd"));
    apply_gallery_theme();
    alia_style_generate_defaults(the_system, nullptr);

    alia_app_run_loop(&config, &app);
#ifndef __EMSCRIPTEN__
    alia_app_destroy(&app);
#endif
    return 0;
}
