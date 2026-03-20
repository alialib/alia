#include <alia/abi/ui/palette.h>
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

static inline float
alia_deg_to_rad(float degrees)
{
    return degrees * std::numbers::pi_v<float> / 180.0f;
}

extern "C" {

static void
fill_states(
    alia_interaction_colors* states,
    alia_oklch base,
    const alia_theme_params* p)
{
    float dir = p->is_dark_mode ? 1.0f : -1.0f;

    states->idle = alia_srgb8_from_unclamped_oklch(base);
    states->hover = alia_srgb8_from_unclamped_oklch(
        {base.l + (p->hover_l_shift * dir),
         base.c,
         base.h + p->interaction_hue_shift});
    states->active = alia_srgb8_from_unclamped_oklch(
        {base.l + (p->active_l_shift * dir),
         base.c,
         base.h + (p->interaction_hue_shift * 2.0f)});
    states->disabled = alia_srgb8_from_unclamped_oklch(
        {p->is_dark_mode ? 0.3f : 0.8f, 0.02f, base.h});
}

static void
generate_swatch_from_oklch(
    alia_swatch* swatch, alia_oklch base, const alia_theme_params* p)
{
    // 1. Solid Background
    fill_states(&swatch->solid, base, p);

    // 2. On-Solid (High Contrast Flip)
    // If the solid background lightness is > 0.65, we need dark text.
    // Otherwise, we need light text. We keep the hue, but drop chroma to 0.02.
    float on_solid_l = (base.l > 0.65f) ? 0.15f : 0.95f;
    alia_oklch on_solid_oklch = {on_solid_l, 0.02f, base.h};
    fill_states(&swatch->on_solid, on_solid_oklch, p);

    // 3. Subtle Background
    // Lighter in light mode (0.90), darker in dark mode (0.25).
    float subtle_l = p->is_dark_mode ? 0.25f : 0.90f;
    alia_oklch subtle_oklch = {subtle_l, 0.04f, base.h};
    fill_states(&swatch->subtle, subtle_oklch, p);

    // 4. Outline
    // Matches the solid lightness, but pulls the chroma back slightly.
    alia_oklch outline_oklch = {base.l, base.c * 0.8f, base.h};
    fill_states(&swatch->outline, outline_oklch, p);

    // 5. Text and On-Subtle
    // Needs to be legible against both the neutral app background AND the
    // subtle background. Lighter in dark mode (0.85), darker in light mode
    // (0.40).
    float text_l = p->is_dark_mode ? 0.85f : 0.40f;
    alia_oklch text_oklch = {text_l, base.c * 0.6f, base.h};

    fill_states(&swatch->text, text_oklch, p);

    // Mathematically identical to text, but explicitly mapped for API
    // symmetry.
    fill_states(&swatch->on_subtle, text_oklch, p);
}

static void
generate_swatch_from_srgb8(
    alia_swatch* swatch, alia_srgb8 seed, const alia_theme_params* p)
{
    alia_oklch base = alia_oklch_from_srgb8(seed);
    generate_swatch_from_oklch(swatch, base, p);
}

static void
generate_ramp(
    alia_foundation_ramp* ramp, alia_srgb8 seed, const alia_theme_params* p)
{
    alia_oklch base = alia_oklch_from_srgb8(seed);
    float step = p->foundation_step_l;
    float dir = p->is_dark_mode ? 1.0f : -1.0f;

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
    seeds.secondary = alia_srgb8_from_unclamped_oklch(
        {sec_l, bg_oklch.c + 0.02f, bg_oklch.h});

    // 2. Relational Alerts:
    // Target a lightness that pops against the context.
    float alert_l = is_dark ? 0.65f : 0.55f;
    float alert_c = 0.16f; // High punch

    // Danger: Red hue (~25 deg)
    seeds.danger = alia_srgb8_from_unclamped_oklch(
        {alert_l, alert_c, alia_deg_to_rad(25.0f)});

    // Success: Green hue (~145 deg)
    seeds.success = alia_srgb8_from_unclamped_oklch(
        {alert_l, alert_c, alia_deg_to_rad(145.0f)});

    // Info: Blue hue (~250 deg)
    seeds.info = alia_srgb8_from_unclamped_oklch(
        {alert_l, alert_c, alia_deg_to_rad(250.0f)});

    // Warning: Yellow/Orange hue (~75 deg)
    // *OKLCH Gamut Quirk:* Yellow physically requires a higher lightness
    // to reach high chroma without turning muddy brown.
    seeds.warning = alia_srgb8_from_unclamped_oklch(
        {alert_l + 0.15f, alert_c, alia_deg_to_rad(75.0f)});

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
    alia_srgb8 bg = alia_srgb8_from_unclamped_oklch(bg_oklch);

    // 2. Calculate the text
    // High contrast against the background, carrying a slightly stronger brand
    // tint (Chroma 0.02)
    float text_l = is_dark ? 0.95f : 0.15f;
    alia_oklch text_oklch = {text_l, 0.02f, brand_h};
    alia_srgb8 text = alia_srgb8_from_unclamped_oklch(text_oklch);

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
                  .foundation_step_l = 0.075f,
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

    alia_srgb8 structural_seed = alia_srgb8_from_unclamped_oklch(struct_oklch);

    // 3. Expand Structural Ramp using the new offset seed
    generate_ramp(&out_palette->foundation.structural, structural_seed, &p);

    generate_ramp(&out_palette->foundation.text, seeds->text_base, &p);

    // TODO: Add seeds for focus and selection
    generate_swatch_from_srgb8(&out_palette->focus, seeds->primary, &p);
    generate_swatch_from_srgb8(&out_palette->selection, seeds->secondary, &p);

    // Expand Semantics
    generate_swatch_from_srgb8(&out_palette->primary, seeds->primary, &p);
    generate_swatch_from_srgb8(&out_palette->secondary, seeds->secondary, &p);
    generate_swatch_from_srgb8(&out_palette->success, seeds->success, &p);
    generate_swatch_from_srgb8(&out_palette->warning, seeds->warning, &p);
    generate_swatch_from_srgb8(&out_palette->danger, seeds->danger, &p);
    generate_swatch_from_srgb8(&out_palette->info, seeds->info, &p);

    // Literal palette: Evenly-sampled hues around the OKLCH wheel.
    //
    // We intentionally decouple these from semantics so that consumers can
    // always rely on:
    //  - `red`    ≈ canonical red
    //  - `orange` ≈ canonical orange
    //  - `amber`  ≈ orange-yellow
    //  - `yellow` ≈ canonical yellow
    //  - `lime`   ≈ yellow-green
    //  - `green`  ≈ canonical green
    //  - `teal`   ≈ green-cyan
    //  - `cyan`   ≈ canonical cyan
    //  - `blue`   ≈ canonical blue
    //  - `indigo` ≈ blue-violet
    //  - `purple` ≈ canonical purple / violet
    //  - `pink`   ≈ warm magenta
    //
    // These are generated directly in OKLCH so that the theme can still
    // control lightness and chroma in a perceptual space.
    float literal_l = p.is_dark_mode ? 0.65f : 0.55f;
    float literal_c = 0.16f;

    auto make_literal_seed
        = [&](const char* /*name*/, float hue_degrees, float l_offset) {
              float h = alia_deg_to_rad(hue_degrees);
              float l = literal_l + l_offset;
              alia_oklch seed = {l, literal_c, h};
              return seed;
          };

    // Primary warm wheel (R → O → A → Y → L)
    generate_swatch_from_oklch(
        &out_palette->colors.red, make_literal_seed("red", 15.0f, 0.0f), &p);
    generate_swatch_from_oklch(
        &out_palette->colors.orange,
        make_literal_seed("orange", 35.0f, 0.0f),
        &p);
    generate_swatch_from_oklch(
        &out_palette->colors.amber,
        make_literal_seed("amber", 55.0f, 0.02f),
        &p);
    // Yellow needs a touch more lightness to stay clean and avoid brown.
    generate_swatch_from_oklch(
        &out_palette->colors.yellow,
        make_literal_seed("yellow", 80.0f, 0.08f),
        &p);
    generate_swatch_from_oklch(
        &out_palette->colors.lime,
        make_literal_seed("lime", 110.0f, 0.04f),
        &p);

    // Cool wheel (G → T → C → B → I)
    generate_swatch_from_oklch(
        &out_palette->colors.green,
        make_literal_seed("green", 140.0f, 0.0f),
        &p);
    generate_swatch_from_oklch(
        &out_palette->colors.teal,
        make_literal_seed("teal", 165.0f, 0.0f),
        &p);
    generate_swatch_from_oklch(
        &out_palette->colors.cyan,
        make_literal_seed("cyan", 190.0f, 0.0f),
        &p);
    generate_swatch_from_oklch(
        &out_palette->colors.blue,
        make_literal_seed("blue", 220.0f, 0.0f),
        &p);
    generate_swatch_from_oklch(
        &out_palette->colors.indigo,
        make_literal_seed("indigo", 250.0f, 0.0f),
        &p);

    // Ending arc (Purple → Pink)
    generate_swatch_from_oklch(
        &out_palette->colors.purple,
        make_literal_seed("purple", 285.0f, 0.0f),
        &p);
    generate_swatch_from_oklch(
        &out_palette->colors.pink,
        make_literal_seed("pink", 320.0f, 0.0f),
        &p);
}

} // extern "C"
