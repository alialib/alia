#include <alia/abi/ui/palette.h>

extern "C" {

static void
fill_states(
    alia_color_states* states, alia_oklch base, const alia_theme_params* p)
{
    float dir = p->is_dark_mode ? 1.0f : -1.0f;

    states->idle = alia_srgb8_from_unclipped_oklch(base);
    states->hover = alia_srgb8_from_unclipped_oklch(
        {base.l + (p->hover_l_shift * dir),
         base.c,
         base.h + p->interaction_hue_shift});
    states->active = alia_srgb8_from_unclipped_oklch(
        {base.l + (p->active_l_shift * dir),
         base.c,
         base.h + (p->interaction_hue_shift * 2.0f)});
    states->disabled = alia_srgb8_from_unclipped_oklch(
        {p->is_dark_mode ? 0.3f : 0.8f, 0.02f, base.h});
}

static void
generate_swatch(
    alia_swatch* swatch, alia_srgb8 seed, const alia_theme_params* p)
{
    alia_oklch base = alia_oklch_from_srgb8(seed);
    fill_states(&swatch->solid, base, p);
    fill_states(
        &swatch->subtle, {p->is_dark_mode ? 0.25f : 0.90f, 0.04f, base.h}, p);
    fill_states(&swatch->outline, {base.l, base.c * 0.8f, base.h}, p);
    fill_states(
        &swatch->text,
        {p->is_dark_mode ? 0.85f : 0.40f, base.c * 0.6f, base.h},
        p);
}

static void
generate_ramp(
    alia_foundation_ramp* ramp, alia_srgb8 seed, const alia_theme_params* p)
{
    alia_oklch base = alia_oklch_from_srgb8(seed);
    float step = p->foundation_step_l;
    float dir = p->is_dark_mode ? -1.0f : 1.0f;

    fill_states(
        &ramp->weaker_4, {base.l - (step * 4 * dir), base.c, base.h}, p);
    fill_states(
        &ramp->weaker_3, {base.l - (step * 3 * dir), base.c, base.h}, p);
    fill_states(
        &ramp->weaker_2, {base.l - (step * 2 * dir), base.c, base.h}, p);
    fill_states(
        &ramp->weaker_1, {base.l - (step * 1 * dir), base.c, base.h}, p);
    fill_states(&ramp->base, base, p);
    fill_states(
        &ramp->stronger_1, {base.l + (step * 1 * dir), base.c, base.h}, p);
    fill_states(
        &ramp->stronger_2, {base.l + (step * 2 * dir), base.c, base.h}, p);
    fill_states(
        &ramp->stronger_3, {base.l + (step * 3 * dir), base.c, base.h}, p);
    fill_states(
        &ramp->stronger_4, {base.l + (step * 4 * dir), base.c, base.h}, p);
}

// --- Public API ---

alia_palette_seeds
alia_seeds_from_core(
    alia_srgb8 brand, alia_srgb8 bg, alia_srgb8 text, bool is_dark)
{
    alia_palette_seeds seeds;

    seeds.bg_base = bg;
    seeds.text_base = text;
    seeds.primary = brand;

    // Convert the provided background to OKLCH so we can extract its DNA
    alia_oklch bg_oklch = alia_oklch_from_srgb8(bg);

    // 1. Relational Secondary:
    // Inherit the background's hue. Bump chroma slightly so it isn't
    // completely dead. Shift lightness toward the text so it functions as a
    // muted mid-tone.
    float sec_l = is_dark ? bg_oklch.l + 0.35f : bg_oklch.l - 0.35f;
    seeds.secondary = alia_srgb8_from_unclipped_oklch(
        {sec_l, bg_oklch.c + 0.02f, bg_oklch.h});

    // 2. Relational Alerts:
    // Target a lightness that pops against the context.
    float alert_l = is_dark ? 0.65f : 0.55f;
    float alert_c = 0.16f; // High punch

    // Danger: Red hue (~25 deg)
    seeds.danger = alia_srgb8_from_unclipped_oklch({alert_l, alert_c, 25.0f});

    // Success: Green hue (~145 deg)
    seeds.success
        = alia_srgb8_from_unclipped_oklch({alert_l, alert_c, 145.0f});

    // Info: Blue hue (~250 deg)
    seeds.info = alia_srgb8_from_unclipped_oklch({alert_l, alert_c, 250.0f});

    // Warning: Yellow/Orange hue (~75 deg)
    // *OKLCH Gamut Quirk:* Yellow physically requires a higher lightness
    // to reach high chroma without turning muddy brown.
    seeds.warning
        = alia_srgb8_from_unclipped_oklch({alert_l + 0.15f, alert_c, 75.0f});

    return seeds;
}

alia_palette_seeds
alia_seeds_from_elevation(alia_srgb8 brand, int elevation, bool is_dark)
{
    // Extract the brand's DNA to subtly tint the entire UI shell
    alia_oklch brand_oklch = alia_oklch_from_srgb8(brand);
    float brand_h = brand_oklch.h;

    // 1. Calculate the background
    // Dark mode: start deep (0.12), lighten as elevation increases (+0.03 per
    // level). Light mode: start light (0.98), darken very slightly (-0.01 per
    // level) for depth.
    float bg_l
        = is_dark ? 0.12f + (elevation * 0.03f) : 0.98f - (elevation * 0.01f);

    // Apply a tiny hint of the brand hue (Chroma 0.015) so it isn't dead grey
    alia_oklch bg_oklch = {bg_l, 0.015f, brand_h};
    alia_srgb8 bg = alia_srgb8_from_unclipped_oklch(bg_oklch);

    // 2. Calculate the text
    // High contrast against the background, carrying a slightly stronger brand
    // tint (Chroma 0.02)
    float text_l = is_dark ? 0.95f : 0.15f;
    alia_oklch text_oklch = {text_l, 0.02f, brand_h};
    alia_srgb8 text = alia_srgb8_from_unclipped_oklch(text_oklch);

    // 3. Delegate to the core triad function
    // alia_seeds_from_core will now use this tinted background to generate a
    // perfectly harmonized secondary slate and appropriate alert lightnesses.
    return alia_seeds_from_core(brand, bg, text, is_dark);
}

void
alia_palette_expand(
    alia_palette* out_palette,
    const alia_palette_seeds* seeds,
    const alia_theme_params* params)
{
    alia_theme_params p
        = params
            ? *params
            : alia_theme_params{
                  .foundation_step_l = 0.05f,
                  .hover_l_shift = 0.05f,
                  .active_l_shift = 0.10f,
                  .interaction_hue_shift = 0.0f,
                  .is_dark_mode = true // Should ideally be passed in, but
                                       // defaulting to true for safety
              };

    // Expand Foundations
    generate_ramp(&out_palette->foundation.background, seeds->bg_base, &p);

    // Shift lightness away from the background (lighter in dark mode, darker
    // in light mode)
    float struct_l_shift = p.is_dark_mode ? 0.6f : -0.6f;

    // Bump chroma slightly so borders/dividers hold their shape
    float struct_c_shift = 0.005f;

    alia_oklch bg_oklch = alia_oklch_from_srgb8(seeds->bg_base);
    alia_oklch struct_oklch = {
        bg_oklch.l + struct_l_shift, bg_oklch.c + struct_c_shift, bg_oklch.h};

    alia_srgb8 structural_seed = alia_srgb8_from_unclipped_oklch(struct_oklch);

    // 3. Expand Structural Ramp using the new offset seed
    generate_ramp(&out_palette->foundation.structural, structural_seed, &p);

    generate_ramp(&out_palette->foundation.text, seeds->text_base, &p);

    // Expand Semantics
    generate_swatch(&out_palette->primary, seeds->primary, &p);
    generate_swatch(&out_palette->secondary, seeds->secondary, &p);
    generate_swatch(&out_palette->success, seeds->success, &p);
    generate_swatch(&out_palette->warning, seeds->warning, &p);
    generate_swatch(&out_palette->danger, seeds->danger, &p);
    generate_swatch(&out_palette->info, seeds->info, &p);

    // Setup basic literal palette mappings based on semantics for V1
    generate_swatch(&out_palette->colors.red, seeds->danger, &p);
    generate_swatch(&out_palette->colors.orange, seeds->warning, &p);
    generate_swatch(&out_palette->colors.green, seeds->success, &p);
    generate_swatch(&out_palette->colors.blue, seeds->info, &p);

    // Overrides
    alia_oklch focus_base = alia_oklch_from_srgb8(seeds->primary);
    fill_states(
        &out_palette->focus_ring, {0.70f, focus_base.c, focus_base.h}, &p);
}

} // extern "C"
