#include <alia/abi/base/arena.h>
#include <alia/abi/ui/library.h>
#include <alia/abi/ui/styling/color_program.h>
#include <alia/abi/ui/styling/environment.h>
#include <alia/abi/ui/styling/palette_generation.h>
#include <alia/impl/ui/styling/color_program.hpp>
#include <alia/ui/styling/descriptors_internal.h>


#include <doctest/doctest.h>

#include <cstring>

using namespace alia::color;

static alia_style_environment
make_env(alia_palette const& palette, bool dark = true)
{
    alia_style_environment env{};
    env.palette = palette;
    alia::style_vars_apply_core_defaults(&env, dark);
    env.seeds = alia_style_seeds_default();
    return env;
}

static alia_srgba8
resolve_env(
    alia_palette const& palette,
    alia_color_program_step const* steps,
    alia_color_program_ref ref,
    bool dark = true)
{
    alia_style_environment env = make_env(palette, dark);
    return alia_color_program_resolve(&env, steps, ref);
}

TEST_CASE("button chrome uses color programs")
{
    alignas(alia_color_program_step) unsigned char storage[4096];
    alia_bump_allocator bump;
    alia_bump_allocator_bind(
        &bump, storage, sizeof(storage), alia_grow_callback_none());
    alia_color_program_emitter emitter
        = alia_color_program_emitter_begin(&bump);

    alia_button_style style;
    alia_button_style_generate(&style, NULL, &emitter);
    CHECK(style.steps != nullptr);

    // match fill: LOAD_COLOR + match_lightness + delta_float + scale_float
    CHECK(style.paint.fill.rest.count == 21);
    CHECK(
        style.steps[style.paint.fill.rest.offset].op
        == ALIA_COLOR_OP_LOAD_COLOR);
    CHECK(
        style.steps[style.paint.fill.rest.offset].u8
        == ALIA_STYLE_COLOR_PRIMARY);
    CHECK(
        style.steps[style.paint.fill.rest.offset + 2].op == ALIA_COLOR_OP_GET);
    CHECK(
        style.steps[style.paint.fill.rest.offset + 2].u8
        == (uint8_t) ALIA_COLOR_CHANNEL_L);
    CHECK(
        style.steps[style.paint.fill.rest.offset + 6].op
        == ALIA_COLOR_OP_LOAD_VAR);
    CHECK(
        style.steps[style.paint.fill.rest.offset + 6].u8
        == ALIA_STYLE_VAR_MATCH_STEP);
    CHECK(
        style.steps[style.paint.fill.rest.offset + 20].op
        == ALIA_COLOR_OP_SET);
    CHECK(
        style.steps[style.paint.fill.rest.offset + 20].u8
        == (uint8_t) ALIA_COLOR_CHANNEL_C);

    // strengthen continuations expand to GET/LOAD_VAR/LOAD/arith/SET
    CHECK(style.paint.fill.hovered.count == 12);
    CHECK(
        style.steps[style.paint.fill.hovered.offset].op == ALIA_COLOR_OP_GET);
    CHECK(
        style.steps[style.paint.fill.hovered.offset + 1].op
        == ALIA_COLOR_OP_LOAD_VAR);
    CHECK(
        style.steps[style.paint.fill.hovered.offset + 1].u8
        == ALIA_STYLE_VAR_HOVER_STRENGTHEN);
    CHECK(
        style.steps[style.paint.fill.hovered.offset + 11].op
        == ALIA_COLOR_OP_SET);
    CHECK(style.paint.fill.active.count == 12);
    CHECK(
        style.steps[style.paint.fill.active.offset + 1].op
        == ALIA_COLOR_OP_LOAD_VAR);
    CHECK(
        style.steps[style.paint.fill.active.offset + 1].u8
        == ALIA_STYLE_VAR_ACTIVE_STRENGTHEN);
    CHECK(style.paint.fill.disabled.count == 1);
    CHECK(
        style.steps[style.paint.fill.disabled.offset].op
        == ALIA_COLOR_OP_LOAD_COLOR);

    // fill + white seed + swap + contrast
    CHECK(style.label.rest.count == 31);
    CHECK(
        style.steps[style.label.rest.offset + 30].op
        == ALIA_COLOR_OP_CONTRAST);
    CHECK(!style.paint.gradient);
    CHECK(style.paint.border_width == 0.f);

    alia_color_program_emitter outline_emitter
        = alia_color_program_emitter_begin(&bump);
    alia_button_recipe outline_recipe = alia_button_recipe_default();
    outline_recipe.chrome = ALIA_BUTTON_CHROME_OUTLINE;
    outline_recipe.hue = alia_style_channel_from_slot(ALIA_STYLE_COLOR_DANGER);
    outline_recipe.chroma
        = alia_style_channel_from_slot(ALIA_STYLE_COLOR_DANGER);
    alia_style_seeds const seeds = alia_style_seeds_default();
    alia_button_style_apply_recipe(
        &style, &outline_recipe, &seeds, &outline_emitter);
    CHECK(
        style.steps[style.paint.fill.rest.offset].u8
        == ALIA_STYLE_COLOR_DANGER);
    CHECK(style.paint.fill.rest.count == 3);
    CHECK(
        style.steps[style.paint.fill.rest.offset + 1].op
        == ALIA_COLOR_OP_IMMEDIATE);
    CHECK(
        style.steps[style.paint.fill.rest.offset + 1].f
        == doctest::Approx(0.f));
    CHECK(
        style.steps[style.paint.fill.rest.offset + 2].op == ALIA_COLOR_OP_SET);
    CHECK(
        style.steps[style.paint.fill.rest.offset + 2].u8
        == (uint8_t) ALIA_COLOR_CHANNEL_A);
    CHECK(style.paint.fill.hovered.count == 2);
    CHECK(
        style.steps[style.paint.fill.hovered.offset].op
        == ALIA_COLOR_OP_IMMEDIATE);
    CHECK(
        style.steps[style.paint.fill.hovered.offset].f
        == doctest::Approx(0x20 / 255.f));
    CHECK(
        style.steps[style.paint.fill.active.offset].f
        == doctest::Approx(0x40 / 255.f));
    CHECK(style.paint.border_width == 1.5f);
}

TEST_CASE("color programs push roles and contrast")
{
    alia_palette_recipe recipe = alia_palette_recipe_default();
    alia_palette_recipe_seed_hue(&recipe, 4.5f * 180.f / 3.14159265f);
    alia_palette palette;
    alia_palette_generate(&palette, &recipe, false);

    alignas(alia_color_program_step) unsigned char storage[4096];
    alia_bump_allocator bump;
    alia_bump_allocator_bind(
        &bump, storage, sizeof(storage), alia_grow_callback_none());
    alia_color_program_emitter emitter
        = alia_color_program_emitter_begin(&bump);

    alia_color_program_ref const sample_ref
        = emit(&emitter, sample(ALIA_STYLE_COLOR_PRIMARY));
    alia_color_program_ref const contrast = emit(
        &emitter,
        sample(ALIA_STYLE_COLOR_TEXT)
            | ensure_contrast(ALIA_STYLE_COLOR_PRIMARY, 1.f));

    alia_color_program_step const* steps
        = alia_color_program_emitter_steps(&emitter);
    alia_srgba8 const solid = resolve_env(palette, steps, sample_ref);
    alia_srgba8 const on = resolve_env(palette, steps, contrast);
    CHECK(solid.a == 0xff);
    CHECK(on.a == 0xff);
    CHECK((solid.r != on.r || solid.g != on.g || solid.b != on.b));

    alia_color_program_ref const weak_solid = emit(
        &emitter,
        sample(ALIA_STYLE_COLOR_PRIMARY)
            | ensure_contrast(ALIA_STYLE_COLOR_TEXT, 1.f));
    alia_srgba8 const weak = resolve_env(palette, steps, weak_solid);
    CHECK(weak.a == 0xff);

    alia_color_program_ref const washed = emit(
        &emitter, sample(ALIA_STYLE_COLOR_PRIMARY) | set_alpha(0x40 / 255.f));
    alia_srgba8 const wash = resolve_env(palette, steps, washed);
    CHECK(wash.a == 0x40);
    CHECK(wash.r == solid.r);
    CHECK(wash.g == solid.g);
    CHECK(wash.b == solid.b);

    CHECK(alia_color_op_description(ALIA_COLOR_OP_LOAD_COLOR) != nullptr);
}

TEST_CASE("color program set_hue and harmonize_hue")
{
    {
        color_program_expr const set_expr
            = sample(ALIA_STYLE_COLOR_PRIMARY) | set_hue(180.f);
        CHECK(set_expr.steps[1].op == ALIA_COLOR_OP_IMMEDIATE);
        CHECK(
            set_expr.steps[1].f
            == doctest::Approx(alia_color_program_deg_to_rad(180.f)));
        CHECK(set_expr.steps[2].op == ALIA_COLOR_OP_SET);
        CHECK(set_expr.steps[2].u8 == (uint8_t) ALIA_COLOR_CHANNEL_H);

        color_program_expr const shift_expr
            = sample(ALIA_STYLE_COLOR_PRIMARY) | shift_hue(15.f);
        CHECK(shift_expr.steps[1].op == ALIA_COLOR_OP_GET);
        CHECK(shift_expr.steps[2].op == ALIA_COLOR_OP_IMMEDIATE);
        CHECK(
            shift_expr.steps[2].f
            == doctest::Approx(alia_color_program_deg_to_rad(15.f)));
        CHECK(shift_expr.steps[3].op == ALIA_COLOR_OP_ADD);
        CHECK(shift_expr.steps[4].op == ALIA_COLOR_OP_SET);

        color_program_expr const harm_expr
            = sample(ALIA_STYLE_COLOR_PRIMARY)
            | harmonize_hue(
                  ALIA_STYLE_COLOR_PRIMARY,
                  ALIA_COLOR_PROGRAM_HARMONIZE_STEP_DEG);
        CHECK(harm_expr.steps[1].op == ALIA_COLOR_OP_GET);
        CHECK(harm_expr.steps[2].op == ALIA_COLOR_OP_LOAD_COLOR);
        CHECK(harm_expr.steps[2].u8 == ALIA_STYLE_COLOR_PRIMARY);
        CHECK(harm_expr.steps[5].op == ALIA_COLOR_OP_IMMEDIATE);
        CHECK(
            harm_expr.steps[5].f
            == doctest::Approx(alia_color_program_deg_to_rad(
                ALIA_COLOR_PROGRAM_HARMONIZE_STEP_DEG)));
        CHECK(harm_expr.steps[6].op == ALIA_COLOR_OP_SNAP);
        CHECK(harm_expr.steps[7].op == ALIA_COLOR_OP_SET);
    }

    alia_palette palette;
    std::memset(&palette, 0, sizeof(palette));
    palette.colors[ALIA_STYLE_COLOR_PRIMARY]
        = alia_oklch_make(0.6f, 0.15f, alia_color_program_deg_to_rad(30.f));

    alignas(alia_color_program_step) unsigned char storage[1024];
    alia_bump_allocator bump;
    alia_bump_allocator_bind(
        &bump, storage, sizeof(storage), alia_grow_callback_none());
    alia_color_program_emitter emitter
        = alia_color_program_emitter_begin(&bump);

    alia_color_program_ref const set_blue
        = emit(&emitter, sample(ALIA_STYLE_COLOR_PRIMARY) | set_hue(220.f));
    alia_color_program_ref const harmonized = emit(
        &emitter,
        sample(ALIA_STYLE_COLOR_PRIMARY) | set_hue(40.f)
            | harmonize_hue(
                ALIA_STYLE_COLOR_PRIMARY,
                ALIA_COLOR_PROGRAM_HARMONIZE_STEP_DEG));

    alia_color_program_step const* steps
        = alia_color_program_emitter_steps(&emitter);

    alia_srgba8 const blue = resolve_env(palette, steps, set_blue);
    alia_oklch const blue_oklch
        = alia_oklch_from_srgb8(alia_srgb8_make(blue.r, blue.g, blue.b));
    CHECK(
        blue_oklch.h
        == doctest::Approx(alia_color_program_deg_to_rad(220.f))
               .epsilon(0.05));

    // 40 deg vs primary 30 deg with a 15 deg step snaps to 45 deg.
    alia_srgba8 const snapped = resolve_env(palette, steps, harmonized);
    alia_oklch const snapped_oklch = alia_oklch_from_srgb8(
        alia_srgb8_make(snapped.r, snapped.g, snapped.b));
    CHECK(
        snapped_oklch.h
        == doctest::Approx(alia_color_program_deg_to_rad(45.f)).epsilon(0.05));
}

TEST_CASE("color program lightness delta and strengthen")
{
    alia_palette palette;
    std::memset(&palette, 0, sizeof(palette));
    // Light page: surface high L, text low L, so strengthen darkens.
    palette.colors[ALIA_STYLE_COLOR_SURFACE]
        = alia_oklch_make(0.9f, 0.02f, 0.f);
    palette.colors[ALIA_STYLE_COLOR_TEXT] = alia_oklch_make(0.2f, 0.02f, 0.f);
    palette.colors[ALIA_STYLE_COLOR_PRIMARY]
        = alia_oklch_make(0.55f, 0.12f, 0.f);

    alignas(alia_color_program_step) unsigned char storage[1024];
    alia_bump_allocator bump;
    alia_bump_allocator_bind(
        &bump, storage, sizeof(storage), alia_grow_callback_none());
    alia_color_program_emitter emitter
        = alia_color_program_emitter_begin(&bump);

    alia_color_program_ref const delta_ref = emit(
        &emitter,
        sample(ALIA_STYLE_COLOR_PRIMARY)
            | add_lightness_delta(
                ALIA_STYLE_COLOR_SURFACE, ALIA_STYLE_COLOR_TEXT, 0.2f));

    alia_color_program_step const* steps
        = alia_color_program_emitter_steps(&emitter);
    CHECK(steps[delta_ref.offset].op == ALIA_COLOR_OP_LOAD_COLOR);
    CHECK(steps[delta_ref.offset + 1].op == ALIA_COLOR_OP_GET);
    CHECK(steps[delta_ref.offset + 2].op == ALIA_COLOR_OP_IMMEDIATE);
    CHECK(steps[delta_ref.offset + 2].f == doctest::Approx(0.2f));
    CHECK(steps[delta_ref.offset + 12].op == ALIA_COLOR_OP_SET);

    alia_oklch const before = palette.colors[ALIA_STYLE_COLOR_PRIMARY];
    alia_srgba8 const after_srgb = resolve_env(palette, steps, delta_ref);
    alia_oklch const after = alia_oklch_from_srgb8(
        alia_srgb8_make(after_srgb.r, after_srgb.g, after_srgb.b));
    float const expected_l = before.l + 0.2f * (0.2f - 0.9f);
    CHECK(after.l == doctest::Approx(expected_l).epsilon(0.05));
    CHECK(after.l < before.l);
}

TEST_CASE("highlight alpha follows surface and accent")
{
    alia_palette_recipe recipe = alia_palette_recipe_default();
    alia_palette_recipe_seed_hue(&recipe, 4.5f * 180.f / 3.14159265f);
    alia_palette light;
    alia_palette dark;
    alia_palette_generate(&light, &recipe, false);
    alia_palette_generate(&dark, &recipe, true);
    alia_style_environment light_env = make_env(light, false);
    alia_style_environment dark_env = make_env(dark, true);

    float const highlight_l = 0.05f;
    float const light_a = alia_style_overlay_alpha(
        &light_env, ALIA_STYLE_COLOR_PRIMARY, highlight_l);
    float const dark_a = alia_style_overlay_alpha(
        &dark_env, ALIA_STYLE_COLOR_PRIMARY, highlight_l);
    CHECK(light_a > 0.f);
    CHECK(dark_a > 0.f);

    alignas(alia_color_program_step) unsigned char storage[256];
    alia_bump_allocator bump;
    alia_bump_allocator_bind(
        &bump, storage, sizeof(storage), alia_grow_callback_none());
    alia_color_program_emitter emitter
        = alia_color_program_emitter_begin(&bump);
    alia_color_program_ref const prog
        = emit(&emitter, highlight(ALIA_STYLE_COLOR_PRIMARY, highlight_l));
    alia_color_program_step const* steps
        = alia_color_program_emitter_steps(&emitter);

    CHECK(
        alia_color_program_resolve(&light_env, steps, prog).a
        == (uint8_t) (light_a * 255.f + 0.5f));
    CHECK(
        alia_color_program_resolve(&dark_env, steps, prog).a
        == (uint8_t) (dark_a * 255.f + 0.5f));
}

TEST_CASE("style overlay alpha caps step to L headroom")
{
    alia_style_environment env{};
    env.palette.colors[ALIA_STYLE_COLOR_SURFACE]
        = alia_oklch_make(0.60f, 0.f, 0.f);
    env.palette.colors[ALIA_STYLE_COLOR_PRIMARY]
        = alia_oklch_make(0.64f, 0.1f, 0.f);

    // Narrow headroom: style policy caps step to half the gap so coverage
    // stays soft instead of clamping opaque.
    float const capped
        = alia_style_overlay_alpha(&env, ALIA_STYLE_COLOR_PRIMARY, 0.05f);
    CHECK(capped > 0.f);
    CHECK(capped < 200.f / 255.f);
    CHECK(capped < alia_oklch_overlay_alpha(0.60f, 0.64f, 0.05f));
}

TEST_CASE("elevate_palette shifts surface toward text")
{
    alia_palette_recipe recipe = alia_palette_recipe_default();
    alia_palette_recipe_seed_hue(&recipe, 4.5f * 180.f / 3.14159265f);
    alia_palette base;
    alia_palette_generate(&base, &recipe, false);

    alia_palette raised;
    alia_palette_elevate(&raised, &base, 0.075f);

    float const surface_delta = raised.colors[ALIA_STYLE_COLOR_SURFACE].l
                              - base.colors[ALIA_STYLE_COLOR_SURFACE].l;
    float const well_delta_before = base.colors[ALIA_STYLE_COLOR_WELL].l
                                  - base.colors[ALIA_STYLE_COLOR_SURFACE].l;
    float const well_delta_after = raised.colors[ALIA_STYLE_COLOR_WELL].l
                                 - raised.colors[ALIA_STYLE_COLOR_SURFACE].l;

    // Light mode: raise moves surface down toward text.
    CHECK(surface_delta == doctest::Approx(-0.075f));
    CHECK(well_delta_after == doctest::Approx(well_delta_before));
    CHECK(
        raised.colors[ALIA_STYLE_COLOR_PRIMARY].l
        == doctest::Approx(base.colors[ALIA_STYLE_COLOR_PRIMARY].l));
    CHECK(
        raised.colors[ALIA_STYLE_COLOR_TEXT].l
        == doctest::Approx(base.colors[ALIA_STYLE_COLOR_TEXT].l));
}

TEST_CASE("ensure_contrast respects require and absolute direction")
{
    alia_palette palette;
    std::memset(&palette, 0, sizeof(palette));
    // Mid-light fill that sits near light text. AUTO used to prefer the
    // lighter pole.
    palette.colors[ALIA_STYLE_COLOR_PRIMARY]
        = alia_oklch_make(0.78f, 0.12f, 0.f);
    palette.colors[ALIA_STYLE_COLOR_TEXT] = alia_oklch_make(0.92f, 0.02f, 0.f);
    palette.colors[ALIA_STYLE_COLOR_SURFACE]
        = alia_oklch_make(0.20f, 0.02f, 0.f);

    alignas(alia_color_program_step) unsigned char storage[1024];
    alia_bump_allocator bump;
    alia_bump_allocator_bind(
        &bump, storage, sizeof(storage), alia_grow_callback_none());
    alia_color_program_emitter emitter
        = alia_color_program_emitter_begin(&bump);

    alia_color_program_ref const auto_ink = emit(
        &emitter,
        sample(ALIA_STYLE_COLOR_TEXT)
            | ensure_contrast(ALIA_STYLE_COLOR_PRIMARY, 4.5f));
    alia_color_program_ref const dark_fill = emit(
        &emitter,
        sample(ALIA_STYLE_COLOR_PRIMARY)
            | ensure_contrast_against_white(
                4.5f, ALIA_COLOR_L_DARK, ALIA_COLOR_CONTRAST_REQUIRE));
    // Strong+require from TEXT must stay on the light (text) side of a
    // dark-surface theme, even when prefer would flip for ratio.
    alia_color_program_ref const strong_ink = emit(
        &emitter,
        sample(ALIA_STYLE_COLOR_TEXT)
            | ensure_contrast(
                ALIA_STYLE_COLOR_PRIMARY,
                4.5f,
                ALIA_COLOR_L_STRONG,
                ALIA_COLOR_CONTRAST_REQUIRE));

    alia_color_program_step const* steps
        = alia_color_program_emitter_steps(&emitter);

    alia_srgba8 const auto_srgb = resolve_env(palette, steps, auto_ink);
    alia_oklch const ink = alia_oklch_from_srgb8(
        alia_srgb8_make(auto_srgb.r, auto_srgb.g, auto_srgb.b));
    // Light fill: AUTO prefers the darker ink pole (luminance heuristic).
    CHECK(ink.l < 0.78f);

    alia_srgba8 const dark_srgb = resolve_env(palette, steps, dark_fill);
    alia_oklch const dark = alia_oklch_from_srgb8(
        alia_srgb8_make(dark_srgb.r, dark_srgb.g, dark_srgb.b));
    CHECK(dark.l < palette.colors[ALIA_STYLE_COLOR_PRIMARY].l);

    alia_srgba8 const strong_srgb = resolve_env(palette, steps, strong_ink);
    alia_oklch const strong = alia_oklch_from_srgb8(
        alia_srgb8_make(strong_srgb.r, strong_srgb.g, strong_srgb.b));
    CHECK(strong.l >= palette.colors[ALIA_STYLE_COLOR_TEXT].l);
}

TEST_CASE("match and inverse use palette floats along SURFACE to TEXT")
{
    alia_style_environment dark_env{};
    alia::style_vars_apply_core_defaults(&dark_env, true);
    alia_palette& dark_page = dark_env.palette;
    dark_page.colors[ALIA_STYLE_COLOR_SURFACE]
        = alia_oklch_make(0.24f, 0.01f, 0.f);
    dark_page.colors[ALIA_STYLE_COLOR_TEXT]
        = alia_oklch_make(0.95f, 0.02f, 0.f);
    dark_page.colors[ALIA_STYLE_COLOR_PRIMARY]
        = alia_oklch_make(0.78f, 0.18f, 4.5f);

    alia_style_environment light_env{};
    alia::style_vars_apply_core_defaults(&light_env, false);
    alia_palette& light_page = light_env.palette;
    light_page.colors[ALIA_STYLE_COLOR_SURFACE]
        = alia_oklch_make(0.96f, 0.01f, 0.f);
    light_page.colors[ALIA_STYLE_COLOR_TEXT]
        = alia_oklch_make(0.15f, 0.02f, 0.f);
    light_page.colors[ALIA_STYLE_COLOR_PRIMARY]
        = alia_oklch_make(0.50f, 0.18f, 4.5f);

    alignas(alia_color_program_step) unsigned char storage[1024];
    alia_bump_allocator bump;
    alia_bump_allocator_bind(
        &bump, storage, sizeof(storage), alia_grow_callback_none());
    alia_color_program_emitter emitter
        = alia_color_program_emitter_begin(&bump);

    alia_color_program_ref const match = emit(
        &emitter,
        sample(ALIA_STYLE_COLOR_PRIMARY)
            | match_lightness(ALIA_STYLE_COLOR_SURFACE)
            | add_lightness_delta_float(
                ALIA_STYLE_COLOR_SURFACE,
                ALIA_STYLE_COLOR_TEXT,
                ALIA_STYLE_VAR_MATCH_STEP));
    alia_color_program_ref const inverse = emit(
        &emitter,
        sample(ALIA_STYLE_COLOR_PRIMARY)
            | match_lightness(ALIA_STYLE_COLOR_SURFACE)
            | add_lightness_delta_float(
                ALIA_STYLE_COLOR_SURFACE,
                ALIA_STYLE_COLOR_TEXT,
                ALIA_STYLE_VAR_INVERSE_STEP));

    alia_color_program_step const* steps
        = alia_color_program_emitter_steps(&emitter);

    auto l_of = [&](alia_style_environment const& env,
                    alia_color_program_ref ref) {
        alia_srgba8 const rgba = alia_color_program_resolve(&env, steps, ref);
        return alia_oklch_from_srgb8(alia_srgb8_make(rgba.r, rgba.g, rgba.b))
            .l;
    };

    float const match_dark = l_of(dark_env, match);
    float const inv_dark = l_of(dark_env, inverse);
    CHECK(match_dark < 0.5f);
    CHECK(inv_dark > match_dark + 0.2f);

    float const match_light = l_of(light_env, match);
    float const inv_light = l_of(light_env, inverse);
    CHECK(match_light > 0.75f);
    CHECK(match_light > inv_light + 0.2f);

    // Retuning match_step on the environment moves the fill without new steps.
    dark_env.vars[ALIA_STYLE_VAR_MATCH_STEP] = 0.5f;
    CHECK(l_of(dark_env, match) > match_dark + 0.1f);

    char dump[512];
    int n = alia_color_program_snprint(dump, (int) sizeof(dump), steps, match);
    CHECK(n > 0);
    CHECK(std::strstr(dump, "LOAD_COLOR") != nullptr);
    CHECK(std::strstr(dump, "LOAD_VAR") != nullptr);
}

TEST_CASE("palette_from_swatches maps accents and neutrals")
{
    alia_srgb8 const colors[] = {
        alia_srgb8_from_hex("ff2e63"),
        alia_srgb8_from_hex("8338ec"),
        alia_srgb8_from_hex("08d9d6"),
        alia_srgb8_from_hex("252a34"),
        alia_srgb8_from_hex("eaeaea"),
    };
    alia_palette pal;
    alia_palette_from_swatches(&pal, colors, 5, true, NULL);

    CHECK(
        pal.colors[ALIA_STYLE_COLOR_SURFACE].l
        < pal.colors[ALIA_STYLE_COLOR_TEXT].l);
    CHECK(pal.colors[ALIA_STYLE_COLOR_PRIMARY].c > 0.1f);
    CHECK(pal.colors[ALIA_STYLE_COLOR_SECONDARY].c > 0.05f);
    CHECK(pal.colors[ALIA_STYLE_COLOR_TERTIARY].c > 0.05f);
    CHECK(pal.colors[ALIA_STYLE_COLOR_DANGER].c > 0.05f);
}

TEST_CASE("solid on-color is white or black vs the fill")
{
    alia_palette palette;
    std::memset(&palette, 0, sizeof(palette));
    palette.colors[ALIA_STYLE_COLOR_SURFACE]
        = alia_oklch_make(0.22f, 0.02f, 0.f);
    palette.colors[ALIA_STYLE_COLOR_TEXT] = alia_oklch_make(0.95f, 0.02f, 0.f);
    palette.colors[ALIA_STYLE_COLOR_PRIMARY]
        = alia_oklch_make(0.78f, 0.18f, 4.5f);

    alignas(alia_color_program_step) unsigned char storage[512];
    alia_bump_allocator bump;
    alia_bump_allocator_bind(
        &bump, storage, sizeof(storage), alia_grow_callback_none());
    alia_color_program_emitter emitter
        = alia_color_program_emitter_begin(&bump);

    alia_color_program_ref const label = emit(
        &emitter,
        role_solid_on_color(ALIA_STYLE_COLOR_PRIMARY, tone::match, 6.75f));

    alia_style_environment env = make_env(palette);
    alia_srgba8 const rgba = alia_color_program_resolve(
        &env, alia_color_program_emitter_steps(&emitter), label);
    alia_oklch const ink
        = alia_oklch_from_srgb8(alia_srgb8_make(rgba.r, rgba.g, rgba.b));
    CHECK(ink.l > 0.85f);
    CHECK(ink.c < 0.02f);
    palette.colors[ALIA_STYLE_COLOR_SURFACE]
        = alia_oklch_make(0.96f, 0.01f, 0.f);
    palette.colors[ALIA_STYLE_COLOR_TEXT] = alia_oklch_make(0.15f, 0.02f, 0.f);
    env.palette = palette;
    alia_srgba8 const light_rgba = alia_color_program_resolve(
        &env, alia_color_program_emitter_steps(&emitter), label);
    alia_oklch const light_ink = alia_oklch_from_srgb8(
        alia_srgb8_make(light_rgba.r, light_rgba.g, light_rgba.b));
    CHECK(light_ink.l < 0.05f);
    CHECK(light_ink.c < 0.02f);
}
