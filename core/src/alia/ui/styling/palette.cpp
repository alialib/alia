#include <alia/abi/ui/styling/descriptors.h>
#include <alia/abi/ui/styling/environment.h>
#include <alia/abi/ui/styling/palette_generation.h>

#include <cmath>
#include <numbers>

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

static inline float
clamp01(float x)
{
    if (x < 0.f)
        return 0.f;
    if (x > 1.f)
        return 1.f;
    return x;
}

static inline float
lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

static inline float
wrap_hue_degrees(float degrees)
{
    while (degrees < 0.f)
        degrees += 360.f;
    while (degrees >= 360.f)
        degrees -= 360.f;
    return degrees;
}

static inline float
shortest_hue_delta(float a, float b)
{
    float const pi = std::numbers::pi_v<float>;
    float delta = b - a;
    if (delta > pi)
        delta -= 2.f * pi;
    else if (delta < -pi)
        delta += 2.f * pi;
    return delta;
}

// Return true when chroma is low relative to typical accents.
static inline bool
is_neutralish(alia_oklch c)
{
    return c.c < 0.045f;
}

// Soft ceiling for chroma pulled from a seed color.
static constexpr float k_chroma_seed_max = 0.28f;

extern "C" {

alia_palette_recipe
alia_palette_recipe_default(void)
{
    alia_palette_recipe out{};
    float* base = reinterpret_cast<float*>(&out);
    size_t count = 0;
    alia_style_float_desc const* descs = alia_style_recipe_descs(&count);
    for (size_t i = 0; i < count; ++i)
    {
        alia_style_float_desc const& d = descs[i];
        // Recipe knobs are mode-symmetric; light/dark defaults match.
        *alia_style_float_at(base, &d) = d.default_light;
    }
    return out;
}

void
alia_palette_recipe_seed_hue(alia_palette_recipe* recipe, float hue_degrees)
{
    ALIA_ASSERT(recipe);
    recipe->hue = wrap_hue_degrees(hue_degrees);
}

void
alia_palette_recipe_seed_color(alia_palette_recipe* recipe, alia_srgb8 color)
{
    ALIA_ASSERT(recipe);
    alia_oklch const oklch = alia_oklch_from_srgb8(color);
    recipe->hue = wrap_hue_degrees(alia_rad_to_deg(oklch.h));
    // Near-gray seeds keep the existing chroma knob; otherwise hint toward
    // the swatch with a soft ceiling so neon inputs stay usable.
    if (oklch.c > 0.02f)
    {
        float c = oklch.c;
        if (c > k_chroma_seed_max)
            c = k_chroma_seed_max;
        recipe->chroma = c;
    }
}

void
alia_palette_generate(
    alia_palette* out, alia_palette_recipe const* recipe, bool dark)
{
    ALIA_ASSERT(out);
    alia_palette_recipe const r
        = recipe ? *recipe : alia_palette_recipe_default();

    float const lightness = clamp01(r.lightness);
    float const contrast = clamp01(r.contrast);
    float const h = alia_deg_to_rad(r.hue);
    float const c = r.chroma < 0.f ? 0.f : r.chroma;
    float const spacing = alia_deg_to_rad(r.hue_spacing);

    float const page_extreme = dark ? 0.20f : 0.98f;
    float const ink_extreme = dark ? 0.96f : 0.12f;
    float const surface_l = lerp(0.5f, page_extreme, lightness);
    float const text_l = lerp(surface_l, ink_extreme, contrast);

    float const surface_c = c * 0.04f;
    float const text_c = c * 0.08f;
    float const neutral_c = c * 0.06f;

    float const well_l = lerp(surface_l, text_l, 0.12f);
    float const neutral_l = lerp(surface_l, text_l, 0.35f);

    float const accent_l = lerp(surface_l, text_l, dark ? 0.72f : 0.42f);
    float const secondary_l = lerp(surface_l, text_l, dark ? 0.65f : 0.38f);
    float const tertiary_l = lerp(surface_l, text_l, dark ? 0.60f : 0.40f);
    float const danger_l = lerp(surface_l, text_l, dark ? 0.68f : 0.45f);

    out->colors[ALIA_STYLE_COLOR_SURFACE] = {surface_l, surface_c, h};
    out->colors[ALIA_STYLE_COLOR_NEUTRAL] = {neutral_l, neutral_c, h};
    out->colors[ALIA_STYLE_COLOR_TEXT] = {text_l, text_c, h};
    out->colors[ALIA_STYLE_COLOR_WELL] = {well_l, surface_c, h};
    out->colors[ALIA_STYLE_COLOR_PRIMARY] = {accent_l, c, h};
    out->colors[ALIA_STYLE_COLOR_FOCUS]
        = out->colors[ALIA_STYLE_COLOR_PRIMARY];

    out->colors[ALIA_STYLE_COLOR_SECONDARY]
        = {secondary_l, c * 0.55f, h + spacing};
    out->colors[ALIA_STYLE_COLOR_TERTIARY]
        = {tertiary_l, c * 0.70f, h - spacing};

    float danger_c = c;
    if (danger_c < 0.18f)
        danger_c = 0.18f;
    out->colors[ALIA_STYLE_COLOR_DANGER]
        = {danger_l, danger_c, alia_deg_to_rad(25.f)};
}

alia_swatch_recipe
alia_swatch_recipe_default(void)
{
    return alia_swatch_recipe{
        .assign = ALIA_SWATCH_ASSIGN_AUTO,
        .synthesize_danger = true,
    };
}

void
alia_palette_from_swatches(
    alia_palette* out,
    alia_srgb8 const* colors,
    int count,
    bool dark,
    alia_swatch_recipe const* recipe)
{
    ALIA_ASSERT(out);
    ALIA_ASSERT(colors || count == 0);
    alia_swatch_recipe const sr
        = recipe ? *recipe : alia_swatch_recipe_default();

    alia_palette_recipe pr = alia_palette_recipe_default();
    if (count <= 0)
    {
        alia_palette_recipe_seed_hue(&pr, 250.f);
        alia_palette_generate(out, &pr, dark);
        return;
    }

    constexpr int k_max = 8;
    if (count > k_max)
        count = k_max;

    alia_oklch swatch[k_max];
    for (int i = 0; i < count; ++i)
        swatch[i] = alia_oklch_from_srgb8(colors[i]);

    alia_oklch accents[k_max];
    int n_accent = 0;
    alia_oklch neutrals[k_max];
    int n_neutral = 0;

    if (sr.assign == ALIA_SWATCH_ASSIGN_ORDER)
    {
        // Take the first chromatic-ish colors as accents in order. Trailing
        // near-grays become neutrals. If everything is chromatic, all are
        // accents.
        for (int i = 0; i < count; ++i)
        {
            if (is_neutralish(swatch[i]) && n_accent > 0)
                neutrals[n_neutral++] = swatch[i];
            else
                accents[n_accent++] = swatch[i];
        }
    }
    else
    {
        for (int i = 0; i < count; ++i)
        {
            if (is_neutralish(swatch[i]))
                neutrals[n_neutral++] = swatch[i];
            else
                accents[n_accent++] = swatch[i];
        }
        // Rank accents by chroma, descending.
        for (int i = 0; i < n_accent; ++i)
        {
            for (int j = i + 1; j < n_accent; ++j)
            {
                if (accents[j].c > accents[i].c)
                {
                    alia_oklch tmp = accents[i];
                    accents[i] = accents[j];
                    accents[j] = tmp;
                }
            }
        }
    }

    if (n_accent == 0)
    {
        // If everything is neutral, promote the mid-L one as a quiet primary.
        accents[0] = neutrals[0];
        n_accent = 1;
        for (int i = 1; i < n_neutral; ++i)
        {
            float const mid = std::fabs(neutrals[i].l - 0.5f);
            float const best = std::fabs(accents[0].l - 0.5f);
            if (mid < best)
                accents[0] = neutrals[i];
        }
    }

    alia_palette_recipe_seed_color(
        &pr, alia_srgb8_from_unclamped_oklch(accents[0]));
    alia_palette_generate(out, &pr, dark);

    // Derive structural roles from neutrals when provided.
    if (n_neutral >= 2)
    {
        alia_oklch a = neutrals[0];
        alia_oklch b = neutrals[1];
        for (int i = 2; i < n_neutral; ++i)
        {
            if (neutrals[i].l < a.l)
                a = neutrals[i];
            if (neutrals[i].l > b.l)
                b = neutrals[i];
        }
        if (a.l > b.l)
        {
            alia_oklch tmp = a;
            a = b;
            b = tmp;
        }
        // a is darker, b is lighter
        if (dark)
        {
            out->colors[ALIA_STYLE_COLOR_SURFACE] = a;
            out->colors[ALIA_STYLE_COLOR_TEXT] = b;
        }
        else
        {
            out->colors[ALIA_STYLE_COLOR_SURFACE] = b;
            out->colors[ALIA_STYLE_COLOR_TEXT] = a;
        }
        // Re-anchor well/neutral after overwriting surface from swatches.
        alia_oklch const surface = out->colors[ALIA_STYLE_COLOR_SURFACE];
        float const text_l = out->colors[ALIA_STYLE_COLOR_TEXT].l;
        out->colors[ALIA_STYLE_COLOR_WELL]
            = {lerp(surface.l, text_l, 0.12f), surface.c, surface.h};
        out->colors[ALIA_STYLE_COLOR_NEUTRAL]
            = {lerp(surface.l, text_l, 0.35f), surface.c * 1.2f, surface.h};
    }
    else if (n_neutral == 1)
    {
        alia_oklch const n = neutrals[0];
        alia_oklch surface = out->colors[ALIA_STYLE_COLOR_SURFACE];
        surface.c = n.c;
        surface.h = n.h;
        if ((dark && n.l < 0.5f) || (!dark && n.l > 0.5f))
            surface.l = n.l;
        out->colors[ALIA_STYLE_COLOR_SURFACE] = surface;
        out->colors[ALIA_STYLE_COLOR_TEXT].c = n.c;
        out->colors[ALIA_STYLE_COLOR_TEXT].h = n.h;
        out->colors[ALIA_STYLE_COLOR_WELL].c = surface.c;
        out->colors[ALIA_STYLE_COLOR_WELL].h = surface.h;
        out->colors[ALIA_STYLE_COLOR_NEUTRAL].h = surface.h;
    }

    // Keep swatch L/C for accents when provided (paste fidelity).
    out->colors[ALIA_STYLE_COLOR_PRIMARY] = accents[0];
    out->colors[ALIA_STYLE_COLOR_FOCUS] = accents[0];

    float const spacing = alia_deg_to_rad(pr.hue_spacing);
    if (n_accent >= 2)
        out->colors[ALIA_STYLE_COLOR_SECONDARY] = accents[1];
    else
        out->colors[ALIA_STYLE_COLOR_SECONDARY]
            = {out->colors[ALIA_STYLE_COLOR_SECONDARY].l,
               accents[0].c * 0.55f,
               accents[0].h + spacing};

    if (n_accent >= 3)
        out->colors[ALIA_STYLE_COLOR_TERTIARY] = accents[2];
    else
        out->colors[ALIA_STYLE_COLOR_TERTIARY]
            = {out->colors[ALIA_STYLE_COLOR_TERTIARY].l,
               accents[0].c * 0.70f,
               accents[0].h - spacing};

    // Prefer a warm/red leftover for danger, otherwise synthesize one.
    bool have_danger = false;
    float const red = alia_deg_to_rad(25.f);
    for (int i = 0; i < n_accent; ++i)
    {
        float const d = std::fabs(shortest_hue_delta(accents[i].h, red));
        if (d < alia_deg_to_rad(35.f) && i >= 3)
        {
            out->colors[ALIA_STYLE_COLOR_DANGER] = accents[i];
            have_danger = true;
            break;
        }
    }
    if (!have_danger && n_accent >= 4)
    {
        out->colors[ALIA_STYLE_COLOR_DANGER] = accents[3];
        have_danger = true;
    }
    if (!have_danger && sr.synthesize_danger)
    {
        // Keep generated danger L/C; only hue is fixed to red.
        out->colors[ALIA_STYLE_COLOR_DANGER].h = red;
    }
    else if (!have_danger)
    {
        out->colors[ALIA_STYLE_COLOR_DANGER]
            = out->colors[ALIA_STYLE_COLOR_PRIMARY];
    }
}

void
alia_palette_elevate(alia_palette* out, alia_palette const* in, float delta_l)
{
    ALIA_ASSERT(out);
    ALIA_ASSERT(in);
    *out = *in;

    alia_oklch const old_surface = in->colors[ALIA_STYLE_COLOR_SURFACE];
    alia_oklch const text = in->colors[ALIA_STYLE_COLOR_TEXT];
    float const well_delta
        = in->colors[ALIA_STYLE_COLOR_WELL].l - old_surface.l;
    float const neutral_delta
        = in->colors[ALIA_STYLE_COLOR_NEUTRAL].l - old_surface.l;

    float const toward_text = text.l >= old_surface.l ? 1.f : -1.f;
    alia_oklch surface = old_surface;
    surface.l = clamp01(surface.l + toward_text * delta_l);
    out->colors[ALIA_STYLE_COLOR_SURFACE] = surface;

    alia_oklch well = in->colors[ALIA_STYLE_COLOR_WELL];
    well.l = clamp01(surface.l + well_delta);
    out->colors[ALIA_STYLE_COLOR_WELL] = well;

    alia_oklch neutral = in->colors[ALIA_STYLE_COLOR_NEUTRAL];
    neutral.l = clamp01(surface.l + neutral_delta);
    out->colors[ALIA_STYLE_COLOR_NEUTRAL] = neutral;
}

} // extern "C"
