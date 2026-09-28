#include <alia/ui/layout/options.hpp>

#include <doctest/doctest.h>

#include <type_traits>

using namespace alia;

TEST_CASE("layout options construct from layout_like packs")
{
    layout_options from_pad = pad(8.f);
    CHECK(from_pad.has_pad());
    CHECK(from_pad.pad_offsets.left == 8.f);

    layout_options from_spec = GROW | pad(4.f) | width(10.f);
    CHECK(raw_code(from_spec.flags) == raw_code(GROW));
    CHECK(from_spec.has_pad());
    CHECK(from_spec.pad_offsets.left == 4.f);
    CHECK(from_spec.has_width());
    CHECK(from_spec.width_value == 10.f);

    layout_options from_spaced = height(12.f) | SPACED;
    CHECK(from_spaced.has_height());
    CHECK(from_spaced.height_value == 12.f);
    CHECK(raw_code(from_spaced.flags) == raw_code(SPACED));
}

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

TEST_CASE("add_default_flush and SPACED")
{
    CHECK(raw_code(add_default_flush(GROW)) == raw_code(GROW | FLUSH));
    CHECK(
        raw_code(add_default_flush(GROW | SPACED)) == raw_code(GROW | SPACED));
    CHECK(raw_code(add_default_flush(FLUSH)) == raw_code(FLUSH));
    CHECK(raw_code(add_default_flush(NO_FLAGS)) == raw_code(FLUSH));

    auto const sized = width(10.f) | height(20.f) | SPACED;
    CHECK(layout_content_size(sized).x == 10.f);
    CHECK(layout_content_size(sized).y == 20.f);

    auto const unsized = without_size(sized);
    CHECK(layout_content_size(unsized).x == 0.f);
    CHECK(layout_content_size(unsized).y == 0.f);
    CHECK(raw_code(unsized.flags) == raw_code(SPACED));
}

TEST_CASE("layout_content_size and without_size preserve piece types")
{
    CHECK(layout_content_size(pad(8.f)).x == 0.f);
    CHECK(layout_content_size(pad(8.f)).y == 0.f);
    CHECK(layout_content_size(width(12.f)).x == 12.f);
    CHECK(layout_content_size(width(12.f)).y == 0.f);
    CHECK(layout_content_size(height(9.f)).x == 0.f);
    CHECK(layout_content_size(height(9.f)).y == 9.f);
    CHECK(layout_content_size(growth(2.f)).x == 0.f);
    CHECK(layout_content_size(GROW).x == 0.f);

    static_assert(std::is_same_v<decltype(without_size(pad(8.f))), pad_spec>);
    static_assert(
        std::is_same_v<decltype(without_size(growth(1.f))), growth_spec>);
    static_assert(
        std::is_same_v<decltype(without_size(GROW)), layout_flag_set>);
    static_assert(
        std::is_same_v<decltype(without_size(width(1.f))), layout_flag_set>);
    static_assert(
        std::is_same_v<decltype(without_size(height(1.f))), layout_flag_set>);

    auto const padded = without_size(pad(8.f));
    CHECK(padded.offsets.left == 8.f);

    auto const stripped_width = without_size(width(10.f));
    CHECK(raw_code(stripped_width) == raw_code(layout_flag_set{NO_FLAGS}));

    static_assert(std::is_same_v<
                  decltype(without_size(GROW | pad(4.f))),
                  layout_spec<layout_flag_set, pad_spec>>);
    auto const kept = without_size(GROW | pad(4.f));
    CHECK(kept.pad.offsets.left == 4.f);
    CHECK(raw_code(kept.flags) == raw_code(GROW));
}

TEST_CASE("length and breadth are exclusive of width and height")
{
    auto const relative = length(20.f) | breadth(8.f) | GROW;
    CHECK(layout_content_size(relative).x == 20.f);
    CHECK(layout_content_size(relative).y == 8.f);
    CHECK(
        (raw_code(relative.flags) & raw_code(AXIS_RELATIVE_SIZE))
        == raw_code(AXIS_RELATIVE_SIZE));
    CHECK((raw_code(relative.flags) & raw_code(GROW)) == raw_code(GROW));

    layout_options const erased = as_layout_options(relative);
    CHECK(erased.has_width());
    CHECK(erased.width_value == 20.f);
    CHECK(erased.has_height());
    CHECK(erased.height_value == 8.f);
    CHECK(
        (raw_code(erased.flags) & raw_code(AXIS_RELATIVE_SIZE))
        == raw_code(AXIS_RELATIVE_SIZE));

    auto const unsized = without_size(relative);
    CHECK(layout_content_size(unsized).x == 0.f);
    CHECK(layout_content_size(unsized).y == 0.f);
    CHECK(
        (raw_code(unsized.flags) & raw_code(AXIS_RELATIVE_SIZE))
        == raw_code(AXIS_RELATIVE_SIZE));

    static_assert(
        std::is_same_v<decltype(without_size(length(5.f))), layout_flag_set>);
    CHECK(raw_code(without_size(length(5.f))) == raw_code(AXIS_RELATIVE_SIZE));
    CHECK(
        raw_code(without_size(breadth(5.f))) == raw_code(AXIS_RELATIVE_SIZE));

    CHECK(layout_content_size(length(12.f)).x == 12.f);
    CHECK(layout_content_size(breadth(9.f)).y == 9.f);
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
