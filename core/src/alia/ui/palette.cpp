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
static_assert(
    sizeof(alia_palette) == ALIA_PALETTE_SLOT_COUNT * sizeof(alia_srgb8),
    "palette union must match flat[ALIA_PALETTE_SLOT_COUNT]");

static inline float
alia_deg_to_rad(float degrees)
{
    return degrees * std::numbers::pi_v<float> / 180.0f;
}

extern "C" {

static void
fill_one(alia_srgb8* out, alia_oklch base)
{
    *out = alia_srgb8_from_unclamped_oklch(base);
}

static void
generate_swatch_from_oklch(
    alia_swatch* swatch, alia_oklch base, const alia_theme_params* p)
{
    (void)p;
    // 1. Solid Background
    fill_one(&swatch->solid, base);

    // 2. On-Solid (High Contrast Flip)
    float on_solid_l = (base.l > 0.65f) ? 0.15f : 0.95f;
    alia_oklch on_solid_oklch = {on_solid_l, 0.02f, base.h};
    fill_one(&swatch->on_solid, on_solid_oklch);

    // 3. Subtle Background
    float subtle_l = p->is_dark_mode ? 0.25f : 0.90f;
    alia_oklch subtle_oklch = {subtle_l, 0.04f, base.h};
    fill_one(&swatch->subtle, subtle_oklch);

    // 4. Outline
    alia_oklch outline_oklch = {base.l, base.c * 0.8f, base.h};
    fill_one(&swatch->outline, outline_oklch);

    // 5. Text and On-Subtle
    float text_l = p->is_dark_mode ? 0.85f : 0.40f;
    alia_oklch text_oklch = {text_l, base.c * 0.6f, base.h};

    fill_one(&swatch->text, text_oklch);
    fill_one(&swatch->on_subtle, text_oklch);
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

// --- Public API ---

alia_palette_seeds
alia_seeds_from_core(
    alia_srgb8 brand, alia_srgb8 bg, alia_srgb8 text, bool is_dark)
{
    alia_palette_seeds seeds;

    seeds.bg_base = bg;
    seeds.text_base = text;
    seeds.primary = brand;

    alia_oklch bg_oklch = alia_oklch_from_srgb8(bg);

    float sec_l = is_dark ? bg_oklch.l + 0.35f : bg_oklch.l - 0.35f;
    seeds.secondary = alia_srgb8_from_unclamped_oklch(
        {sec_l, bg_oklch.c + 0.02f, bg_oklch.h});

    float alert_l = is_dark ? 0.65f : 0.55f;
    float alert_c = 0.16f;

    seeds.danger = alia_srgb8_from_unclamped_oklch(
        {alert_l, alert_c, alia_deg_to_rad(25.0f)});

    seeds.success = alia_srgb8_from_unclamped_oklch(
        {alert_l, alert_c, alia_deg_to_rad(145.0f)});

    seeds.info = alia_srgb8_from_unclamped_oklch(
        {alert_l, alert_c, alia_deg_to_rad(250.0f)});

    seeds.warning = alia_srgb8_from_unclamped_oklch(
        {alert_l + 0.15f, alert_c, alia_deg_to_rad(75.0f)});

    return seeds;
}

alia_palette_seeds
alia_seeds_from_elevation(alia_srgb8 brand, int elevation, bool is_dark)
{
    alia_oklch brand_oklch = alia_oklch_from_srgb8(brand);
    float brand_h = brand_oklch.h;

    float bg_l
        = is_dark ? 0.12f + (elevation * 0.03f) : 0.98f - (elevation * 0.01f);

    alia_oklch bg_oklch = {bg_l, 0.015f, brand_h};
    alia_srgb8 bg = alia_srgb8_from_unclamped_oklch(bg_oklch);

    float text_l = is_dark ? 0.95f : 0.15f;
    alia_oklch text_oklch = {text_l, 0.02f, brand_h};
    alia_srgb8 text = alia_srgb8_from_unclamped_oklch(text_oklch);

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
                  .is_dark_mode = true,
              };

    generate_ramp(&out_palette->foundation.background, seeds->bg_base, &p);

    float struct_l_shift = p.is_dark_mode ? 0.6f : -0.6f;
    float struct_c_shift = 0.005f;

    alia_oklch bg_oklch = alia_oklch_from_srgb8(seeds->bg_base);
    alia_oklch struct_oklch = {
        bg_oklch.l + struct_l_shift, bg_oklch.c + struct_c_shift, bg_oklch.h};

    alia_srgb8 structural_seed = alia_srgb8_from_unclamped_oklch(struct_oklch);

    generate_ramp(
        &out_palette->foundation.structural, structural_seed, &p);

    generate_ramp(&out_palette->foundation.text, seeds->text_base, &p);

    generate_swatch_from_srgb8(&out_palette->focus, seeds->primary, &p);
    generate_swatch_from_srgb8(
        &out_palette->selection, seeds->secondary, &p);

    generate_swatch_from_srgb8(&out_palette->primary, seeds->primary, &p);
    generate_swatch_from_srgb8(
        &out_palette->secondary, seeds->secondary, &p);
    generate_swatch_from_srgb8(&out_palette->success, seeds->success, &p);
    generate_swatch_from_srgb8(&out_palette->warning, seeds->warning, &p);
    generate_swatch_from_srgb8(&out_palette->danger, seeds->danger, &p);
    generate_swatch_from_srgb8(&out_palette->info, seeds->info, &p);

    float literal_l = p.is_dark_mode ? 0.65f : 0.55f;
    float literal_c = 0.16f;

    auto make_literal_seed
        = [&](const char* /*name*/, float hue_degrees, float l_offset) {
              float h = alia_deg_to_rad(hue_degrees);
              float l = literal_l + l_offset;
              alia_oklch seed = {l, literal_c, h};
              return seed;
          };

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
    generate_swatch_from_oklch(
        &out_palette->colors.yellow,
        make_literal_seed("yellow", 80.0f, 0.08f),
        &p);
    generate_swatch_from_oklch(
        &out_palette->colors.lime,
        make_literal_seed("lime", 110.0f, 0.04f),
        &p);

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
