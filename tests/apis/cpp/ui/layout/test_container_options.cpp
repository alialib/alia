#include <alia/ui/layout/container_options.hpp>

#include <doctest/doctest.h>

using namespace alia;

TEST_CASE("container options combine gap, flags, and provide_box with |")
{
    alia_box box;
    auto const combined = gap(8.f) | JUSTIFY_CENTER | provide_box(box)
                        | line_gap(4.f) | minimum_line_height(12.f);

    CHECK(combined.has_gap());
    CHECK(combined.gap_value == 8.f);
    CHECK(raw_code(combined.flags) == raw_code(JUSTIFY_CENTER));
    CHECK(combined.has_provide_box());
    CHECK(combined.box == &box);
    CHECK(combined.has_line_gap());
    CHECK(combined.line_gap_value == 4.f);
    CHECK(combined.has_minimum_line_height());
    CHECK(combined.minimum_line_height_value == 12.f);

    container_options merged = default_container;
    merged |= gap(2.f);
    merged |= BASELINE_GROUP_ALIGN_TOP;
    CHECK(merged.has_gap());
    CHECK(merged.gap_value == 2.f);
    CHECK(raw_code(merged.flags) == raw_code(BASELINE_GROUP_ALIGN_TOP));
}

TEST_CASE("container begin flags OR placement and policy bits")
{
    alia_box box;
    auto const options = JUSTIFY_SPACE_BETWEEN | provide_box(box);
    alia_layout_flags_t const code
        = container_begin_flags(GROW | FILL, options);
    CHECK((code & ALIA_GROW) != 0);
    CHECK((code & ALIA_FILL) != 0);
    CHECK((code & ALIA_JUSTIFY_SPACE_BETWEEN) != 0);
    CHECK((code & ALIA_PROVIDE_BOX) != 0);
}

TEST_CASE("grid options carry provide_box")
{
    alia_box box;
    grid_options const grid = default_grid | provide_box(box);
    CHECK(grid.has_provide_box());
    CHECK(grid.box == &box);

    grid_row_options const row = provide_box(box) | default_grid_row;
    CHECK(row.has_provide_box());
    CHECK(row.box == &box);

    CHECK((grid_begin_flags(GROW, grid) & ALIA_PROVIDE_BOX) != 0);
    CHECK((grid_begin_flags(GROW, grid) & ALIA_GROW) != 0);
    CHECK(
        (grid_row_begin_flags(NO_FLAGS, default_grid_row) & ALIA_PROVIDE_BOX)
        == 0);
}
