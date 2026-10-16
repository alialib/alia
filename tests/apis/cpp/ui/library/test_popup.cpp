#include <alia/test/layout/layout_fixture.hpp>
#include <alia/ui/library/popup.hpp>

#include <alia/abi/ui/layout/api.h>

#include <doctest/doctest.h>

TEST_CASE("popup content stays out of the parent layout tree")
{
    layout_test_fixture* fixture = layout_test_fixture_create();
    REQUIRE(fixture != nullptr);

    auto ui = [](alia_context* ctx) {
        alia::popup(
            *ctx,
            alia_box_make(
                alia_vec2f_make(10.f, 10.f), alia_vec2f_make(40.f, 20.f)),
            [&] {
                alia_layout_leaf_emit(
                    ctx,
                    alia_layout_content_metrics{
                        .size = alia_vec2f_make(30.f, 12.f),
                        .ascent = 0.f,
                        .descent = 0.f},
                    0);
            });
    };

    layout_test_fixture_run_refresh(fixture, ui);
    CHECK(layout_fixture_root_child(fixture) == nullptr);

    layout_test_fixture_destroy(fixture);
}
