#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/library.h>
#include <alia/abi/ui/palette.h>

#include <doctest/doctest.h>

#include <cstring>

enum
{
    SLOT_WHITE = 0,
    SLOT_BLACK = 1,
    SLOT_RED = 2,
    SLOT_GRAY = 3,
};

static alia_palette
test_palette(void)
{
    alia_palette palette;
    std::memset(&palette, 0, sizeof(palette));
    palette.flat[SLOT_WHITE] = alia_srgb8_make(255, 255, 255);
    palette.flat[SLOT_BLACK] = alia_srgb8_make(0, 0, 0);
    palette.flat[SLOT_RED] = alia_srgb8_make(200, 40, 40);
    palette.flat[SLOT_GRAY] = alia_srgb8_make(128, 128, 128);
    return palette;
}

static int
channel_spread(alia_srgba8 c)
{
    uint8_t max_c = c.r > c.g ? c.r : c.g;
    uint8_t min_c = c.r < c.g ? c.r : c.g;
    if (c.b > max_c)
        max_c = c.b;
    if (c.b < min_c)
        min_c = c.b;
    return (int) max_c - (int) min_c;
}

TEST_CASE("responsive color ops")
{
    alia_palette const palette = test_palette();
    alia_palette_color const white = alia_palette_color_make(SLOT_WHITE, 255);
    alia_palette_color const black = alia_palette_color_make(SLOT_BLACK, 255);
    alia_palette_color const red = alia_palette_color_make(SLOT_RED, 255);
    alia_palette_color const gray = alia_palette_color_make(SLOT_GRAY, 200);
    alia_palette_color const black_half
        = alia_palette_color_make(SLOT_BLACK, 128);

    // none returns the base, including alpha
    {
        alia_srgba8 const got
            = alia_color_op_resolve(&palette, gray, alia_color_op_none());
        CHECK(got.r == 128);
        CHECK(got.g == 128);
        CHECK(got.b == 128);
        CHECK(got.a == 200);
    }

    // replace discards the base
    {
        alia_srgba8 const got = alia_color_op_resolve(
            &palette, white, alia_color_op_replace(black_half));
        CHECK(got.r == 0);
        CHECK(got.g == 0);
        CHECK(got.b == 0);
        CHECK(got.a == 128);
    }

    // adjust darkens, and zero chroma collapses a hue toward gray
    {
        alia_srgba8 const rest
            = alia_color_op_resolve(&palette, gray, alia_color_op_none());
        alia_srgba8 const darker = alia_color_op_resolve(
            &palette, gray, alia_color_op_adjust(-40, 255));
        CHECK(darker.r < rest.r);
        CHECK(darker.g < rest.g);
        CHECK(darker.b < rest.b);
        CHECK(darker.a == rest.a);

        alia_srgba8 const saturated
            = alia_color_op_resolve(&palette, red, alia_color_op_none());
        alia_srgba8 const grayed = alia_color_op_resolve(
            &palette, red, alia_color_op_adjust(0, 0));
        CHECK(channel_spread(grayed) < channel_spread(saturated));
        CHECK(grayed.a == saturated.a);
    }

    // blend amount 0 keeps the base; an opaque source replaces it
    {
        alia_srgba8 const clear = alia_color_op_resolve(
            &palette,
            white,
            alia_color_op_blend(alia_palette_color_make(SLOT_BLACK, 0)));
        CHECK(clear.r == 255);
        CHECK(clear.g == 255);
        CHECK(clear.b == 255);
        CHECK(clear.a == 255);

        alia_srgba8 const covered = alia_color_op_resolve(
            &palette, white, alia_color_op_blend(black));
        CHECK(covered.r == 0);
        CHECK(covered.g == 0);
        CHECK(covered.b == 0);
        CHECK(covered.a == 255);

        alia_srgba8 const tint = alia_color_op_resolve(
            &palette, white, alia_color_op_blend(black_half));
        CHECK(tint.a == 255);
        CHECK(tint.r == tint.g);
        CHECK(tint.g == tint.b);
        CHECK(tint.r > 0);
        CHECK(tint.r < 255);
    }

    // disabled wins over hover; active wins over hover; focus uses the base
    {
        alia_responsive_color color;
        color.base = white;
        color.hovered = alia_color_op_replace(black);
        color.active = alia_color_op_replace(red);
        color.disabled = alia_color_op_replace(gray);

        alia_srgba8 const disabled = alia_responsive_color_resolve(
            &palette,
            color,
            (uint8_t) (ALIA_INTERACTION_STATUS_DISABLED
                       | ALIA_INTERACTION_STATUS_HOVERED));
        CHECK(disabled.r == 128);
        CHECK(disabled.g == 128);
        CHECK(disabled.b == 128);
        CHECK(disabled.a == 200);

        alia_srgba8 const active = alia_responsive_color_resolve(
            &palette,
            color,
            (uint8_t) (ALIA_INTERACTION_STATUS_HOVERED
                       | ALIA_INTERACTION_STATUS_ACTIVE));
        CHECK(active.r == 200);
        CHECK(active.g == 40);
        CHECK(active.b == 40);
        CHECK(active.a == 255);

        alia_srgba8 const focused = alia_responsive_color_resolve(
            &palette, color, ALIA_INTERACTION_STATUS_FOCUSED);
        CHECK(focused.r == 255);
        CHECK(focused.g == 255);
        CHECK(focused.b == 255);
        CHECK(focused.a == 255);

        alia_srgba8 const hovered = alia_responsive_color_resolve(
            &palette, color, ALIA_INTERACTION_STATUS_HOVERED);
        CHECK(hovered.r == 0);
        CHECK(hovered.g == 0);
        CHECK(hovered.b == 0);
        CHECK(hovered.a == 255);
    }
}

TEST_CASE("button chrome uses responsive colors")
{
    alia_button_style style;
    alia_button_style_generate(&style, NULL);
    CHECK(style.fill.hovered.kind == ALIA_COLOR_OP_ADJUST);
    CHECK(style.fill.hovered.adjust.l == -10);
    CHECK(style.fill.hovered.adjust.c_scale == 255);
    CHECK(style.fill.active.kind == ALIA_COLOR_OP_ADJUST);
    CHECK(style.fill.active.adjust.l == -20);
    CHECK(style.fill.active.adjust.c_scale == 255);
    CHECK(style.fill.disabled.kind == ALIA_COLOR_OP_REPLACE);
    CHECK(style.label.hovered.kind == ALIA_COLOR_OP_NONE);
    CHECK(style.label.active.kind == ALIA_COLOR_OP_NONE);
    CHECK(style.label.disabled.kind == ALIA_COLOR_OP_REPLACE);
    CHECK(style.border_width == 0.f);
    CHECK(style.border.hovered.kind == ALIA_COLOR_OP_NONE);
    CHECK(style.border.active.kind == ALIA_COLOR_OP_NONE);
    CHECK(style.border.disabled.kind == ALIA_COLOR_OP_NONE);

    alia_button_style_apply_swatch(
        &style, ALIA_PALETTE_SWATCH_DANGER, ALIA_BUTTON_CHROME_OUTLINE);
    CHECK(style.fill.base.alpha == 0);
    CHECK(style.fill.hovered.kind == ALIA_COLOR_OP_REPLACE);
    CHECK(style.fill.hovered.color.alpha == 0x20);
    CHECK(style.fill.active.color.alpha == 0x40);
    CHECK(style.fill.hovered.color.index == style.fill.base.index);
    CHECK(style.label.hovered.kind == ALIA_COLOR_OP_NONE);
    CHECK(style.label.active.kind == ALIA_COLOR_OP_NONE);
    CHECK(style.label.disabled.kind == ALIA_COLOR_OP_REPLACE);
    CHECK(
        style.label.base.index
        == alia_palette_index_swatch(
            ALIA_PALETTE_SWATCH_DANGER, ALIA_PALETTE_SWATCH_PART_TEXT));
    CHECK(style.border.hovered.kind == ALIA_COLOR_OP_NONE);
    CHECK(style.border.active.kind == ALIA_COLOR_OP_NONE);
    CHECK(style.border.disabled.kind == ALIA_COLOR_OP_REPLACE);
    CHECK(
        style.border.base.index
        == alia_palette_index_swatch(
            ALIA_PALETTE_SWATCH_DANGER, ALIA_PALETTE_SWATCH_PART_OUTLINE));
    CHECK(style.border_width == 1.5f);
}
