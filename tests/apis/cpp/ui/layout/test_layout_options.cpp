#include <alia/ui/layout/options.hpp>

#include <doctest/doctest.h>

using namespace alia;

TEST_CASE("layout options combine flags and pad with |")
{
    auto const combined = GROW | pad(8.f);
    CHECK(raw_code(combined.flags) == raw_code(GROW));
    CHECK(combined.pad.offsets.left == 8.f);
    CHECK(combined.pad.offsets.right == 8.f);
    CHECK(combined.pad.offsets.top == 8.f);
    CHECK(combined.pad.offsets.bottom == 8.f);

    layout_options const erased = as_layout_options(combined);
    CHECK(erased.has_pad());
    CHECK(raw_code(erased.flags) == raw_code(GROW));

    layout_options merged = default_layout;
    merged |= CENTER_Y;
    merged |= pad(alia_edge_offsets_make_xy(2.f, 4.f));
    CHECK(raw_code(merged.flags) == raw_code(CENTER_Y));
    CHECK(merged.has_pad());
    CHECK(merged.pad_offsets.left == 2.f);
    CHECK(merged.pad_offsets.top == 4.f);

    auto const stacked = GROW | pad(1.f) | pad(2.f) | FILL_X;
    CHECK(raw_code(stacked.flags) == raw_code(GROW | FILL_X));
    CHECK(stacked.pad.offsets.left == 3.f);
}

TEST_CASE("layout options combine width, height, and growth with |")
{
    auto const sized = GROW | width(20.f) | height(10.f) | growth(1.5f);
    CHECK(raw_code(sized.flags) == raw_code(GROW));
    CHECK(sized.width.value == 20.f);
    CHECK(sized.height.value == 10.f);
    CHECK(sized.growth.value == 1.5f);

    layout_options const erased = as_layout_options(sized);
    CHECK(erased.has_width());
    CHECK(erased.width_value == 20.f);
    CHECK(erased.has_height());
    CHECK(erased.height_value == 10.f);
    CHECK(erased.has_growth());
    CHECK(erased.growth_value == 1.5f);

    auto const axes = width(100.f) | height(50.f);
    CHECK(axes.width.value == 100.f);
    CHECK(axes.height.value == 50.f);

    auto const padded_size = pad(4.f) | width(8.f) | height(8.f) | CENTER_Y;
    CHECK(raw_code(padded_size.flags) == raw_code(CENTER_Y));
    CHECK(padded_size.pad.offsets.left == 4.f);
    CHECK(padded_size.width.value == 8.f);
    CHECK(padded_size.height.value == 8.f);
}

TEST_CASE("add_default alignment fills only unset groups")
{
    layout_flag_set const empty = NO_FLAGS;

    CHECK(
        raw_code(add_default_x_alignment(empty, ALIGN_LEFT))
        == raw_code(ALIGN_LEFT));
    CHECK(
        raw_code(add_default_x_alignment(ALIGN_RIGHT, ALIGN_LEFT))
        == raw_code(ALIGN_RIGHT));
    CHECK(
        raw_code(add_default_x_alignment(GROW | CENTER_Y, ALIGN_LEFT))
        == raw_code(GROW | CENTER_Y | ALIGN_LEFT));

    CHECK(
        raw_code(add_default_y_alignment(empty, CENTER_Y))
        == raw_code(CENTER_Y));
    CHECK(
        raw_code(add_default_y_alignment(ALIGN_TOP, CENTER_Y))
        == raw_code(ALIGN_TOP));

    CHECK(
        raw_code(add_default_alignment(GROW, ALIGN_LEFT, CENTER_Y))
        == raw_code(GROW | ALIGN_LEFT | CENTER_Y));
    CHECK(
        raw_code(
            add_default_alignment(FILL_X | ALIGN_TOP, ALIGN_LEFT, CENTER_Y))
        == raw_code(FILL_X | ALIGN_TOP));

    layout_options options = as_layout_options(GROW | pad(4.f));
    options = add_default_x_alignment(options, ALIGN_LEFT);
    CHECK(raw_code(options.flags) == raw_code(GROW | ALIGN_LEFT));
    CHECK(options.has_pad());
    CHECK(options.pad_offsets.left == 4.f);

    auto with_pad = CENTER_X | pad(1.f);
    with_pad = add_default_alignment(with_pad, ALIGN_LEFT, BASELINE_Y);
    CHECK(raw_code(with_pad.flags) == raw_code(CENTER_X | BASELINE_Y));
    CHECK(with_pad.pad.offsets.left == 1.f);
}
