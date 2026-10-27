#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/palette.h>
#include <cmath>
#include <numbers>

// This implemented a compile-time check that enum index math matches the named
// `alia_palette` layout.
consteval bool
alia_palette_flat_index_ok()
{
    size_t const slot = ALIA_PALETTE_SLOT_SIZE;
    // foundation: base + ramp * stride + level
    if (ALIA_PALETTE_INDEX_FOUNDATION_BASE
            + 0u * ALIA_PALETTE_FOUNDATION_RAMP_STRIDE
            + (size_t) ALIA_PALETTE_RAMP_LEVEL_BASE
        != offsetof(alia_color_table, foundation.background.base) / slot)
        return false;
    if (ALIA_PALETTE_INDEX_FOUNDATION_BASE
            + 1u * ALIA_PALETTE_FOUNDATION_RAMP_STRIDE
            + (size_t) ALIA_PALETTE_RAMP_LEVEL_WEAKER_2
        != offsetof(alia_color_table, foundation.structural.weaker_2) / slot)
        return false;
    if (ALIA_PALETTE_INDEX_FOUNDATION_BASE
            + 2u * ALIA_PALETTE_FOUNDATION_RAMP_STRIDE
            + (size_t) ALIA_PALETTE_RAMP_LEVEL_BASE
        != offsetof(alia_color_table, foundation.text.base) / slot)
        return false;
    // semantic swatch: base + swatch * stride + part
    if (ALIA_PALETTE_INDEX_SWATCH_BASE
            + (size_t) ALIA_PALETTE_SWATCH_PRIMARY * ALIA_PALETTE_SWATCH_STRIDE
            + (size_t) ALIA_PALETTE_SWATCH_PART_OUTLINE
        != offsetof(alia_color_table, primary.outline) / slot)
        return false;
    if (ALIA_PALETTE_INDEX_SWATCH_BASE
            + (size_t) ALIA_PALETTE_SWATCH_INFO * ALIA_PALETTE_SWATCH_STRIDE
            + (size_t) ALIA_PALETTE_SWATCH_PART_SOLID
        != offsetof(alia_color_table, info.solid) / slot)
        return false;
    // literal: base + literal * stride + part
    if (ALIA_PALETTE_INDEX_LITERAL_BASE
            + (size_t) ALIA_PALETTE_LITERAL_RED * ALIA_PALETTE_SWATCH_STRIDE
            + (size_t) ALIA_PALETTE_SWATCH_PART_OUTLINE
        != offsetof(alia_color_table, colors.red.outline) / slot)
        return false;
    if (ALIA_PALETTE_INDEX_LITERAL_BASE
            + (size_t) ALIA_PALETTE_LITERAL_PINK * ALIA_PALETTE_SWATCH_STRIDE
            + (size_t) ALIA_PALETTE_SWATCH_PART_TEXT
        != offsetof(alia_color_table, colors.pink.text) / slot)
        return false;
    return true;
}
static_assert(
    alia_palette_flat_index_ok(),
    "palette flat index math must match struct layout");
static_assert(
    sizeof(alia_color_table) == ALIA_PALETTE_SLOT_COUNT * sizeof(alia_srgb8),
    "color table must match flat[ALIA_PALETTE_SLOT_COUNT]");
static_assert(
    offsetof(alia_palette, colors) == 0,
    "color table must be the first member of the palette");

static inline float
alia_deg_to_rad(float degrees)
{
    return degrees * std::numbers::pi_v<float> / 180.0f;
}

static inline float
alia_rad_to_deg(float radians)
{
    return radians * 180.0f / std::numbers::pi_v<float>;
}

// Get the hue on the accent's 15-degree lattice nearest to `hue_degrees`.
static float
harmonize_hue(float hue_degrees, float accent_degrees)
{
    float const step = 15.f;
    float const delta = hue_degrees - accent_degrees;
    float const remainder = delta - step * std::round(delta / step);
    return hue_degrees - remainder;
}

static inline bool
alia_is_dark_mode(alia_palette_conditions const* conditions)
{
    return conditions->is_dark_mode;
}

extern "C" {

alia_palette_conditions
alia_palette_conditions_default(bool is_dark_mode)
{
    return alia_palette_conditions{
        .is_dark_mode = is_dark_mode,
        .elevation = 0,
        .contrast_target = 0.f,
    };
}

alia_theme_colors_recipe
alia_theme_colors_recipe_default(void)
{
    return alia_theme_colors_recipe{
        .bg_l = {.light = 0.96f, .dark = 0.24f},
        .bg_elevation_step = 0.03f,
        .bg_c = {.light = 0.005f, .dark = 0.015f},

        .text_l = {.light = 0.15f, .dark = 0.95f},
        .text_c = 0.02f,

        .accent_l = {.light = 0.50f, .dark = 0.78f},
        .accent_c_default = 0.20f,

        .alert_l = {.light = 0.48f, .dark = 0.68f},
        .alert_c = 0.20f,

        .glow_l = {.light = 0.30f, .dark = 0.25f},
        .highlight_l = {.light = 0.05f, .dark = 0.12f},
    };
}

alia_palette_expansion
alia_palette_expansion_default(void)
{
    return alia_palette_expansion{
        .foundation_step_l = 0.075f,

        .swatch_subtle_l = {.light = 0.90f, .dark = 0.25f},
        .swatch_text_l = {.light = 0.40f, .dark = 0.85f},
        .on_solid_l_for_light_solid = 0.15f,
        .on_solid_l_for_dark_solid = 0.95f,
        .swatch_outline_c_scale = 0.8f,
        .swatch_text_c_scale = 0.6f,
        .swatch_subtle_c = 0.04f,

        .literal_l = {.light = 0.55f, .dark = 0.65f},
        .literal_c = 0.16f,
        .literal_policy = ALIA_PALETTE_LITERAL_FIXED_SPECTRUM,
    };
}

void
alia_theme_accent_from_hue(
    alia_theme_accent* out, float hue, alia_theme_colors_recipe const* recipe)
{
    alia_theme_colors_recipe r
        = recipe ? *recipe : alia_theme_colors_recipe_default();
    // Color generation replaces this lightness with `accent_l` for the mode.
    out->primary = {0.f, r.accent_c_default, hue};
}

void
alia_theme_accent_from_color(alia_theme_accent* out, alia_srgb8 color)
{
    out->primary = alia_oklch_from_srgb8(color);
}

static void
fill_one(alia_srgb8* out, alia_oklch color)
{
    *out = alia_srgb8_from_unclamped_oklch(color);
}

static float
on_solid_l_for_contrast(
    alia_oklch const& base,
    alia_palette_expansion const& expansion,
    alia_palette_conditions const& conditions)
{
    float const target
        = conditions.contrast_target > 0.f ? conditions.contrast_target : 4.5f;
    float const fallback
        = base.l > 0.65f ? expansion.on_solid_l_for_light_solid
                         : expansion.on_solid_l_for_dark_solid;

    alia_srgb8 const solid = alia_srgb8_from_unclamped_oklch(base);
    float const solid_lum = alia_relative_luminance_srgb8(solid);

    float best_l = fallback;
    float best_ratio = 0.f;
    for (int i = 0; i <= 16; ++i)
    {
        float candidate_l = i / 16.0f;
        alia_srgb8 candidate_srgb
            = alia_srgb8_from_unclamped_oklch({candidate_l, 0.02f, base.h});
        float ratio = alia_relative_luminance_ratio(
            solid_lum, alia_relative_luminance_srgb8(candidate_srgb));
        if (ratio >= target)
            return candidate_l;
        if (ratio > best_ratio)
        {
            best_ratio = ratio;
            best_l = candidate_l;
        }
    }
    return best_l;
}

static void
generate_swatch_from_oklch(
    alia_swatch* swatch,
    alia_oklch base,
    alia_palette_conditions const* conditions,
    alia_palette_expansion const* expansion)
{
    bool const dark = alia_is_dark_mode(conditions);

    fill_one(&swatch->solid, base);

    float on_solid_l = on_solid_l_for_contrast(base, *expansion, *conditions);
    fill_one(&swatch->on_solid, {on_solid_l, 0.02f, base.h});

    float subtle_l = dark ? expansion->swatch_subtle_l.dark
                          : expansion->swatch_subtle_l.light;
    fill_one(&swatch->subtle, {subtle_l, expansion->swatch_subtle_c, base.h});

    fill_one(
        &swatch->outline,
        {base.l, base.c * expansion->swatch_outline_c_scale, base.h});

    float text_l = dark ? expansion->swatch_text_l.dark
                        : expansion->swatch_text_l.light;
    alia_oklch text_oklch
        = {text_l, base.c * expansion->swatch_text_c_scale, base.h};
    fill_one(&swatch->text, text_oklch);
    fill_one(&swatch->on_subtle, text_oklch);
}

static void
generate_ramp(
    alia_foundation_ramp* ramp,
    alia_oklch base,
    alia_palette_conditions const* conditions,
    alia_palette_expansion const* expansion)
{
    float step = expansion->foundation_step_l;
    float dir = alia_is_dark_mode(conditions) ? 1.0f : -1.0f;

    fill_one(&ramp->weaker_4, {base.l - (step * 4 * dir), base.c, base.h});
    fill_one(&ramp->weaker_3, {base.l - (step * 3 * dir), base.c, base.h});
    fill_one(&ramp->weaker_2, {base.l - (step * 2 * dir), base.c, base.h});
    fill_one(&ramp->weaker_1, {base.l - (step * 1 * dir), base.c, base.h});
    fill_one(&ramp->base, base);
    fill_one(&ramp->stronger_1, {base.l + (step * 1 * dir), base.c, base.h});
    fill_one(&ramp->stronger_2, {base.l + (step * 2 * dir), base.c, base.h});
    fill_one(&ramp->stronger_3, {base.l + (step * 3 * dir), base.c, base.h});
    fill_one(&ramp->stronger_4, {base.l + (step * 4 * dir), base.c, base.h});
}

void
alia_theme_colors_generate(
    alia_theme_colors* out,
    alia_theme_accent const* accent,
    alia_palette_conditions const* conditions,
    alia_theme_colors_recipe const* recipe)
{
    alia_theme_colors_recipe r
        = recipe ? *recipe : alia_theme_colors_recipe_default();
    bool const dark = alia_is_dark_mode(conditions);
    int const elevation = conditions->elevation;
    float const primary_h = accent->primary.h;
    float const mode_sign = dark ? 1.f : -1.f;

    float const bg_l = (dark ? r.bg_l.dark : r.bg_l.light)
                     + (elevation * r.bg_elevation_step * mode_sign);
    out->bg = {bg_l, dark ? r.bg_c.dark : r.bg_c.light, primary_h};

    // Structural and secondary sit off the page. The mode flips the direction
    // so both read as raised surfaces.
    float const structural_l_offset = 0.6f;
    float const structural_c_offset = 0.005f;
    out->structural = {
        out->bg.l + structural_l_offset * mode_sign,
        out->bg.c + structural_c_offset,
        out->bg.h,
    };

    float const text_l = dark ? r.text_l.dark : r.text_l.light;
    out->text = {text_l, r.text_c, primary_h};

    // Hue and chroma stay with the accent. Lightness follows the mode.
    float const primary_l = dark ? r.accent_l.dark : r.accent_l.light;
    out->primary = {primary_l, accent->primary.c, accent->primary.h};

    float const secondary_l_offset = 0.35f;
    float const secondary_c_offset = 0.02f;
    out->secondary = {
        out->bg.l + secondary_l_offset * mode_sign,
        out->bg.c + secondary_c_offset,
        out->bg.h,
    };

    float const alert_l = dark ? r.alert_l.dark : r.alert_l.light;

    // Fixed hues. Warning carries a small lightness bump so the gold stays
    // inside the sRGB gamut, matching the literal yellow entry.
    struct alert_entry
    {
        alia_oklch* slot;
        float degrees;
        float l_offset;
    };
    alert_entry const alerts[] = {
        {&out->danger, 25.f, 0.f},
        {&out->warning, 75.f, 0.08f},
        {&out->success, 145.f, 0.f},
        {&out->info, 250.f, 0.f},
    };
    for (alert_entry const& alert : alerts)
    {
        *alert.slot
            = {alert_l + alert.l_offset,
               r.alert_c,
               alia_deg_to_rad(alert.degrees)};
    }
}

static void
generate_literals(
    alia_literal_palette* colors,
    alia_theme_colors const* theme_colors,
    alia_palette_conditions const* conditions,
    alia_palette_expansion const* expansion)
{
    bool const dark = alia_is_dark_mode(conditions);
    float literal_l
        = dark ? expansion->literal_l.dark : expansion->literal_l.light;
    float literal_c = expansion->literal_c;
    float const accent_degrees = alia_rad_to_deg(theme_colors->primary.h);
    bool const harmonize = expansion->literal_policy
                        == ALIA_PALETTE_LITERAL_HARMONIZE_TO_PRIMARY;

    struct literal_entry
    {
        alia_swatch* swatch;
        float hue_degrees;
        float l_offset;
    };

    literal_entry entries[] = {
        {&colors->red, 15.0f, 0.0f},
        {&colors->orange, 35.0f, 0.0f},
        {&colors->amber, 55.0f, 0.02f},
        {&colors->yellow, 80.0f, 0.08f},
        {&colors->lime, 110.0f, 0.04f},
        {&colors->green, 140.0f, 0.0f},
        {&colors->teal, 165.0f, 0.0f},
        {&colors->cyan, 190.0f, 0.0f},
        {&colors->blue, 220.0f, 0.0f},
        {&colors->indigo, 250.0f, 0.0f},
        {&colors->purple, 285.0f, 0.0f},
        {&colors->pink, 320.0f, 0.0f},
    };

    for (literal_entry const& entry : entries)
    {
        float hue_degrees = entry.hue_degrees;
        if (harmonize)
            hue_degrees = harmonize_hue(hue_degrees, accent_degrees);
        float const h = alia_deg_to_rad(hue_degrees);
        alia_oklch literal = {literal_l + entry.l_offset, literal_c, h};
        generate_swatch_from_oklch(
            entry.swatch, literal, conditions, expansion);
    }
}

// Get the alpha of a paint at lightness `fg_l` that moves the page by `step`
// toward that paint. `step` is positive. The sign follows `fg_l - bg_l`.
// Compositing at alpha `a` mixes linear luminance, and OKLab lightness tracks
// the cube root of that, so the cube ratio is the alpha that lands on `bg_l +
// step`.
static uint8_t
overlay_alpha_for(float bg_l, float fg_l, float step)
{
    float const toward_fg = fg_l >= bg_l ? step : -step;
    float const target = bg_l + toward_fg;
    float const bg3 = bg_l * bg_l * bg_l;
    float const denom = fg_l * fg_l * fg_l - bg3;
    float a = denom != 0.f ? (target * target * target - bg3) / denom : 0.f;
    if (a < 0.f)
        a = 0.f;
    if (a > 1.f)
        a = 1.f;
    return (uint8_t) (a * 255.f + 0.5f);
}

void
alia_palette_generate(
    alia_palette* out,
    alia_theme_colors const* colors,
    alia_palette_expansion const* expansion,
    alia_palette_conditions const* conditions,
    alia_theme_colors_recipe const* recipe)
{
    ALIA_ASSERT(colors);
    ALIA_ASSERT(conditions);
    alia_palette_expansion resolved
        = expansion ? *expansion : alia_palette_expansion_default();
    alia_theme_colors_recipe const resolved_recipe
        = recipe ? *recipe : alia_theme_colors_recipe_default();
    alia_color_table* table = &out->colors;

    generate_ramp(
        &table->foundation.background, colors->bg, conditions, &resolved);
    generate_ramp(
        &table->foundation.structural,
        colors->structural,
        conditions,
        &resolved);
    generate_ramp(
        &table->foundation.text, colors->text, conditions, &resolved);

    generate_swatch_from_oklch(
        &table->focus, colors->primary, conditions, &resolved);
    generate_swatch_from_oklch(
        &table->selection, colors->secondary, conditions, &resolved);
    generate_swatch_from_oklch(
        &table->primary, colors->primary, conditions, &resolved);
    generate_swatch_from_oklch(
        &table->secondary, colors->secondary, conditions, &resolved);
    generate_swatch_from_oklch(
        &table->success, colors->success, conditions, &resolved);
    generate_swatch_from_oklch(
        &table->warning, colors->warning, conditions, &resolved);
    generate_swatch_from_oklch(
        &table->danger, colors->danger, conditions, &resolved);
    generate_swatch_from_oklch(
        &table->info, colors->info, conditions, &resolved);

    generate_literals(&table->colors, colors, conditions, &resolved);

    out->page = colors->bg;
    bool const dark = alia_is_dark_mode(conditions);
    out->stronger_sign = dark ? 1 : -1;
    float const glow_l
        = dark ? resolved_recipe.glow_l.dark : resolved_recipe.glow_l.light;
    float const highlight_l = dark ? resolved_recipe.highlight_l.dark
                                   : resolved_recipe.highlight_l.light;
    out->glow_alpha = overlay_alpha_for(colors->bg.l, colors->text.l, glow_l);
    out->highlight_alpha
        = overlay_alpha_for(colors->bg.l, colors->primary.l, highlight_l);
    out->conditions = *conditions;
}

static float
clamp01_unit(float x)
{
    return x < 0.f ? 0.f : (x > 1.f ? 1.f : x);
}

static alia_srgba8
adjust_oklab(alia_srgba8 base, int8_t l_delta, uint8_t c_scale)
{
    alia_oklab lab = alia_oklab_from_rgb(
        alia_rgb_from_srgb8(alia_srgb8_make(base.r, base.g, base.b)));
    lab.l = clamp01_unit(lab.l + (float) l_delta / 255.f);
    float const scale = (float) c_scale / 255.f;
    lab.a *= scale;
    lab.b *= scale;
    alia_srgb8 const rgb = alia_srgb8_from_oklab(lab);
    return alia_srgba8_make(rgb.r, rgb.g, rgb.b, base.a);
}

// Composite `src` over `dst` in linear light. Both colors are straight alpha.
static alia_srgba8
blend_over(alia_srgba8 dst_s, alia_srgba8 src_s)
{
    alia_rgba const dst = alia_rgba_from_srgba8(dst_s);
    alia_rgba const src = alia_rgba_from_srgba8(src_s);
    float const out_a = src.a + dst.a * (1.f - src.a);
    if (out_a <= 0.f)
        return alia_srgba8_make(0, 0, 0, 0);
    float const inv = 1.f / out_a;
    float const dst_weight = dst.a * (1.f - src.a);
    return alia_srgba8_from_rgba(
        alia_rgba{
            (src.r * src.a + dst.r * dst_weight) * inv,
            (src.g * src.a + dst.g * dst_weight) * inv,
            (src.b * src.a + dst.b * dst_weight) * inv,
            out_a,
        });
}

static int8_t
clamp_i8(int value)
{
    if (value > 127)
        return 127;
    if (value < -128)
        return -128;
    return (int8_t) value;
}

// Close `l_frac` of the lightness gap to the page and `c_frac` of the chroma
// gap. Hue is the page's hue.
static alia_srgba8
fade_toward_page(
    alia_palette const* p, alia_srgba8 base, uint8_t l_frac, uint8_t c_frac)
{
    alia_oklch const page = p->page;
    alia_oklch const color
        = alia_oklch_from_srgb8(alia_srgb8_make(base.r, base.g, base.b));
    float const l_t = (float) l_frac / 255.f;
    float const c_t = (float) c_frac / 255.f;
    alia_oklch const faded = {
        color.l + l_t * (page.l - color.l),
        color.c + c_t * (page.c - color.c),
        page.h,
    };
    alia_srgb8 const rgb = alia_srgb8_from_unclamped_oklch(faded);
    return alia_srgba8_make(rgb.r, rgb.g, rgb.b, base.a);
}

alia_srgba8
alia_color_op_resolve(
    alia_palette const* p, alia_palette_color base, alia_color_op op)
{
    alia_srgba8 const base_rgba
        = alia_palette_color_resolve(alia_palette_colors(p), base);
    switch (op.kind)
    {
        case ALIA_COLOR_OP_ADJUST:
            return adjust_oklab(base_rgba, op.adjust.l, op.adjust.c_scale);
        case ALIA_COLOR_OP_STRENGTHEN:
            return adjust_oklab(
                base_rgba,
                clamp_i8((int) op.adjust.l * p->stronger_sign),
                op.adjust.c_scale);
        case ALIA_COLOR_OP_FADE:
            return fade_toward_page(p, base_rgba, op.fade.l, op.fade.c);
        case ALIA_COLOR_OP_REPLACE:
            return alia_palette_color_resolve(
                alia_palette_colors(p), op.color);
        case ALIA_COLOR_OP_BLEND:
            return blend_over(
                base_rgba,
                alia_palette_color_resolve(alia_palette_colors(p), op.color));
        case ALIA_COLOR_OP_NONE:
        default:
            return base_rgba;
    }
}

alia_srgba8
alia_responsive_color_resolve(
    alia_palette const* p, alia_responsive_color color, uint8_t status)
{
    if (status & ALIA_INTERACTION_STATUS_DISABLED)
        return alia_color_op_resolve(p, color.base, color.disabled);
    if (status & ALIA_INTERACTION_STATUS_ACTIVE)
        return alia_color_op_resolve(p, color.base, color.active);
    if (status & ALIA_INTERACTION_STATUS_HOVERED)
        return alia_color_op_resolve(p, color.base, color.hovered);
    return alia_palette_color_resolve(alia_palette_colors(p), color.base);
}

static_assert(sizeof(alia_color_op) <= 4, "color op should stay a few bytes");

void
alia_palette_generate_from_accent(
    alia_palette* out,
    alia_theme_accent const* accent,
    alia_palette_conditions const* conditions,
    alia_theme_colors_recipe const* recipe,
    alia_palette_expansion const* expansion)
{
    alia_theme_colors colors;
    alia_theme_colors_generate(&colors, accent, conditions, recipe);
    alia_palette_generate(out, &colors, expansion, conditions, recipe);
}

} // extern "C"
