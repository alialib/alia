#include <alia/abi/ui/palette.h>
#include <cmath>
#include <numbers>

// Compile-time check that flat index math matches struct layout (no padding
// surprises).
consteval bool
alia_palette_flat_index_ok()
{
    size_t const slot = ALIA_PALETTE_SLOT_SIZE;
    // Foundation: base + ramp*stride + level
    if (ALIA_PALETTE_INDEX_FOUNDATION_BASE
            + 0u * ALIA_PALETTE_FOUNDATION_RAMP_STRIDE
            + (size_t) ALIA_PALETTE_RAMP_LEVEL_BASE
        != offsetof(alia_palette, foundation.background.base) / slot)
        return false;
    if (ALIA_PALETTE_INDEX_FOUNDATION_BASE
            + 1u * ALIA_PALETTE_FOUNDATION_RAMP_STRIDE
            + (size_t) ALIA_PALETTE_RAMP_LEVEL_WEAKER_2
        != offsetof(alia_palette, foundation.structural.weaker_2) / slot)
        return false;
    if (ALIA_PALETTE_INDEX_FOUNDATION_BASE
            + 2u * ALIA_PALETTE_FOUNDATION_RAMP_STRIDE
            + (size_t) ALIA_PALETTE_RAMP_LEVEL_BASE
        != offsetof(alia_palette, foundation.text.base) / slot)
        return false;
    // Semantic swatch: base + swatch*stride + part
    if (ALIA_PALETTE_INDEX_SWATCH_BASE
            + (size_t) ALIA_PALETTE_SWATCH_PRIMARY * ALIA_PALETTE_SWATCH_STRIDE
            + (size_t) ALIA_PALETTE_SWATCH_PART_OUTLINE
        != offsetof(alia_palette, primary.outline) / slot)
        return false;
    if (ALIA_PALETTE_INDEX_SWATCH_BASE
            + (size_t) ALIA_PALETTE_SWATCH_INFO * ALIA_PALETTE_SWATCH_STRIDE
            + (size_t) ALIA_PALETTE_SWATCH_PART_SOLID
        != offsetof(alia_palette, info.solid) / slot)
        return false;
    // Literal: base + literal*stride + part
    if (ALIA_PALETTE_INDEX_LITERAL_BASE
            + (size_t) ALIA_PALETTE_LITERAL_RED * ALIA_PALETTE_SWATCH_STRIDE
            + (size_t) ALIA_PALETTE_SWATCH_PART_OUTLINE
        != offsetof(alia_palette, colors.red.outline) / slot)
        return false;
    if (ALIA_PALETTE_INDEX_LITERAL_BASE
            + (size_t) ALIA_PALETTE_LITERAL_PINK * ALIA_PALETTE_SWATCH_STRIDE
            + (size_t) ALIA_PALETTE_SWATCH_PART_TEXT
        != offsetof(alia_palette, colors.pink.text) / slot)
        return false;
    return true;
}
static_assert(
    alia_palette_flat_index_ok(),
    "palette flat index math must match struct layout");
static_assert(
    sizeof(alia_palette) == ALIA_PALETTE_SLOT_COUNT * sizeof(alia_srgb8),
    "palette union must match flat[ALIA_PALETTE_SLOT_COUNT]");

static inline float
alia_deg_to_rad(float degrees)
{
    return degrees * std::numbers::pi_v<float> / 180.0f;
}

static inline bool
alia_is_dark_mode(alia_theme_context const* ctx)
{
    return ctx->surface.is_dark_mode;
}

extern "C" {

alia_theme_context
alia_theme_context_default(bool is_dark_mode)
{
    return alia_theme_context{
        .surface = {
            .is_dark_mode = is_dark_mode,
            .elevation = 0,
        },
        .contrast_target = 0.f,
    };
}

alia_seed_params
alia_seed_params_default(void)
{
    return alia_seed_params{
        .primary_l_default = 0.55f,
        .primary_c_default = 0.20f,

        .elevation_bg_l_base_dark = 0.12f,
        .elevation_bg_l_per_step_dark = 0.03f,
        .elevation_bg_l_base_light = 0.90f,
        .elevation_bg_l_per_step_light = -0.02f,
        .elevation_bg_c = 0.015f,
        .elevation_text_l_dark = 0.95f,
        .elevation_text_l_light = 0.15f,
        .elevation_text_c = 0.02f,

        .secondary_l_offset = 0.35f,
        .secondary_c_offset = 0.02f,

        .alert_l_dark = 0.65f,
        .alert_l_light = 0.55f,
        .alert_c = 0.16f,
        .alert_hue_danger_deg = 25.0f,
        .alert_hue_warning_deg = 75.0f,
        .alert_hue_success_deg = 145.0f,
        .alert_hue_info_deg = 250.0f,
        .alert_warning_l_offset = 0.15f,
    };
}

alia_palette_params
alia_palette_params_default(void)
{
    return alia_palette_params{
        .foundation_step_l = 0.075f,
        .structural_l_offset = 0.6f,
        .structural_c_offset = 0.005f,

        .swatch_subtle_l_dark = 0.25f,
        .swatch_subtle_l_light = 0.90f,
        .swatch_text_l_dark = 0.85f,
        .swatch_text_l_light = 0.40f,
        .swatch_on_solid_l_dark = 0.15f,
        .swatch_on_solid_l_light = 0.95f,
        .swatch_outline_c_scale = 0.8f,
        .swatch_text_c_scale = 0.6f,
        .swatch_subtle_c = 0.04f,

        .literal_l_dark = 0.65f,
        .literal_l_light = 0.55f,
        .literal_c = 0.16f,
    };
}

void
alia_theme_accent_from_hue(
    alia_theme_accent* out, float hue, alia_seed_params const* params)
{
    alia_seed_params p = params ? *params : alia_seed_params_default();
    out->primary = {p.primary_l_default, p.primary_c_default, hue};
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
    alia_palette_params const& params,
    alia_theme_context const& ctx)
{
    float const target = ctx.contrast_target > 0.f ? ctx.contrast_target : 4.5f;
    float const fallback = base.l > 0.65f ? params.swatch_on_solid_l_dark
                                          : params.swatch_on_solid_l_light;

    alia_srgb8 const solid = alia_srgb8_from_unclamped_oklch(base);
    float const solid_lum = alia_relative_luminance_srgb8(solid);

    float best_l = fallback;
    float best_ratio = 0.f;
    for (int i = 0; i <= 16; ++i)
    {
        float candidate_l = i / 16.0f;
        alia_srgb8 candidate_srgb = alia_srgb8_from_unclamped_oklch(
            {candidate_l, 0.02f, base.h});
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
    alia_theme_context const* ctx,
    alia_palette_params const* params)
{
    bool const dark = alia_is_dark_mode(ctx);

    fill_one(&swatch->solid, base);

    float on_solid_l
        = on_solid_l_for_contrast(base, *params, *ctx);
    fill_one(&swatch->on_solid, {on_solid_l, 0.02f, base.h});

    float subtle_l
        = dark ? params->swatch_subtle_l_dark : params->swatch_subtle_l_light;
    fill_one(
        &swatch->subtle,
        {subtle_l, params->swatch_subtle_c, base.h});

    fill_one(
        &swatch->outline,
        {base.l, base.c * params->swatch_outline_c_scale, base.h});

    float text_l = dark ? params->swatch_text_l_dark : params->swatch_text_l_light;
    alia_oklch text_oklch
        = {text_l, base.c * params->swatch_text_c_scale, base.h};
    fill_one(&swatch->text, text_oklch);
    fill_one(&swatch->on_subtle, text_oklch);
}

static void
generate_ramp(
    alia_foundation_ramp* ramp,
    alia_oklch base,
    alia_theme_context const* ctx,
    alia_palette_params const* params)
{
    float step = params->foundation_step_l;
    float dir = alia_is_dark_mode(ctx) ? 1.0f : -1.0f;

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
alia_palette_seeds_from_accent(
    alia_palette_seeds* out,
    alia_theme_accent const* accent,
    alia_theme_context const* ctx,
    alia_seed_params const* params)
{
    alia_seed_params sp = params ? *params : alia_seed_params_default();
    bool const dark = alia_is_dark_mode(ctx);
    int const elevation = ctx->surface.elevation;
    float const primary_h = accent->primary.h;

    float bg_l = dark ? sp.elevation_bg_l_base_dark
                          + (elevation * sp.elevation_bg_l_per_step_dark)
                      : sp.elevation_bg_l_base_light
                          + (elevation * sp.elevation_bg_l_per_step_light);
    out->bg = {bg_l, sp.elevation_bg_c, primary_h};

    float text_l = dark ? sp.elevation_text_l_dark : sp.elevation_text_l_light;
    out->text = {text_l, sp.elevation_text_c, primary_h};

    out->primary = accent->primary;

    float sec_l = dark ? out->bg.l + sp.secondary_l_offset
                       : out->bg.l - sp.secondary_l_offset;
    out->secondary = {sec_l, out->bg.c + sp.secondary_c_offset, out->bg.h};

    float alert_l = dark ? sp.alert_l_dark : sp.alert_l_light;

    out->danger = {
        alert_l, sp.alert_c, alia_deg_to_rad(sp.alert_hue_danger_deg)};
    out->success = {
        alert_l, sp.alert_c, alia_deg_to_rad(sp.alert_hue_success_deg)};
    out->info = {alert_l, sp.alert_c, alia_deg_to_rad(sp.alert_hue_info_deg)};
    out->warning = {
        alert_l + sp.alert_warning_l_offset,
        sp.alert_c,
        alia_deg_to_rad(sp.alert_hue_warning_deg)};
}

static void
generate_literals(
    alia_literal_palette* colors,
    alia_palette_seeds const* seeds,
    alia_theme_context const* ctx,
    alia_palette_params const* params,
    enum alia_literal_policy policy)
{
    bool const dark = alia_is_dark_mode(ctx);
    float literal_l = dark ? params->literal_l_dark : params->literal_l_light;
    float literal_c = params->literal_c;

    float hue_offset_rad = 0.f;
    if (policy == ALIA_LITERAL_HARMONIZE_TO_PRIMARY)
        hue_offset_rad = seeds->primary.h;

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
        float h = alia_deg_to_rad(entry.hue_degrees) + hue_offset_rad;
        alia_oklch seed = {literal_l + entry.l_offset, literal_c, h};
        generate_swatch_from_oklch(entry.swatch, seed, ctx, params);
    }
}

void
alia_palette_from_seeds(
    alia_palette* out,
    alia_palette_seeds const* seeds,
    alia_theme_context const* ctx,
    alia_palette_params const* params,
    enum alia_literal_policy literals)
{
    alia_palette_params pp = params ? *params : alia_palette_params_default();

    generate_ramp(&out->foundation.background, seeds->bg, ctx, &pp);

    float struct_l_shift = alia_is_dark_mode(ctx) ? pp.structural_l_offset
                                                  : -pp.structural_l_offset;
    alia_oklch struct_oklch = {
        seeds->bg.l + struct_l_shift,
        seeds->bg.c + pp.structural_c_offset,
        seeds->bg.h,
    };
    generate_ramp(&out->foundation.structural, struct_oklch, ctx, &pp);

    generate_ramp(&out->foundation.text, seeds->text, ctx, &pp);

    generate_swatch_from_oklch(&out->focus, seeds->primary, ctx, &pp);
    generate_swatch_from_oklch(&out->selection, seeds->secondary, ctx, &pp);
    generate_swatch_from_oklch(&out->primary, seeds->primary, ctx, &pp);
    generate_swatch_from_oklch(&out->secondary, seeds->secondary, ctx, &pp);
    generate_swatch_from_oklch(&out->success, seeds->success, ctx, &pp);
    generate_swatch_from_oklch(&out->warning, seeds->warning, ctx, &pp);
    generate_swatch_from_oklch(&out->danger, seeds->danger, ctx, &pp);
    generate_swatch_from_oklch(&out->info, seeds->info, ctx, &pp);

    generate_literals(&out->colors, seeds, ctx, &pp, literals);
}

void
alia_palette_from_accent(
    alia_palette* out,
    alia_theme_accent const* accent,
    alia_theme_context const* ctx,
    alia_seed_params const* seed_params,
    alia_palette_params const* palette_params,
    enum alia_literal_policy literals)
{
    alia_palette_seeds seeds;
    alia_palette_seeds_from_accent(&seeds, accent, ctx, seed_params);
    alia_palette_from_seeds(out, &seeds, ctx, palette_params, literals);
}

} // extern "C"
