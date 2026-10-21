#include <alia/test/layout/layout_fixture.hpp>
#include <alia/ui/library/popup.hpp>

#include <alia/abi/base/arena.h>
#include <alia/abi/base/memory.h>
#include <alia/abi/base/object.h>
#include <alia/abi/ui/geometry/api.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/system/api.h>

#include <doctest/doctest.h>

namespace {

void
noop_controller(void* user_data, alia_ui_context* ctx)
{
    (void) user_data;
    (void) ctx;
}

// surface size for `popup` to read during a non-refresh pass
struct surface_rig
{
    void* storage = nullptr;
    alia_ui_system* system = nullptr;

    explicit surface_rig(alia_vec2i size)
    {
        storage = alia_object_alloc(alia_ui_system_object_spec());
        REQUIRE(storage != nullptr);
        alia_ui_controller controller{
            .fn = noop_controller, .user_data = nullptr};
        system = alia_ui_system_init(
            storage, controller, size, alia_memory_ops_system());
        REQUIRE(system != nullptr);
    }

    ~surface_rig()
    {
        alia_ui_system_destroy(system);
        alia_object_free(storage);
    }

    surface_rig(surface_rig const&) = delete;
    surface_rig&
    operator=(surface_rig const&) = delete;
};

// scratch arena for geometry clip-box storage
struct scratch_rig
{
    void* buffer = nullptr;
    alia_bump_allocator bump{};

    scratch_rig()
    {
        buffer = alia_object_alloc(
            alia_struct_spec{.size = 4096u, .align = ALIA_MAX_ALIGN});
        REQUIRE(buffer != nullptr);
        alia_bump_allocator_bind(
            &bump, buffer, 4096u, alia_grow_callback_none());
    }

    ~scratch_rig()
    {
        alia_object_free(buffer);
    }

    scratch_rig(scratch_rig const&) = delete;
    scratch_rig&
    operator=(scratch_rig const&) = delete;
};

} // namespace

TEST_CASE("popup content stays out of the parent layout tree")
{
    layout_test_fixture* fixture = layout_test_fixture_create();
    REQUIRE(fixture != nullptr);

    auto ui = [](alia_ui_context* ctx) {
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

TEST_CASE("popup places content, resets clip, and raises z")
{
    layout_test_fixture* fixture = layout_test_fixture_create();
    REQUIRE(fixture != nullptr);

    alia_vec2i const surface_size = alia_vec2i_make(200, 150);
    surface_rig surface(surface_size);
    scratch_rig scratch;

    alia_box const anchor = alia_box_make(
        alia_vec2f_make(10.f, 20.f), alia_vec2f_make(40.f, 16.f));
    alia_vec2f const content_size = alia_vec2f_make(80.f, 30.f);
    alia_vec2f const incoming_offset = alia_vec2f_make(5.f, 7.f);
    alia_box const incoming_clip
        = alia_box_make(alia_vec2f_make(1.f, 2.f), alia_vec2f_make(3.f, 4.f));

    // layout anchor plus the current geometry offset
    alia_box const anchor_surface
        = alia_box_translate(anchor, incoming_offset);
    alia_box const placed = alia_popup_place(
        anchor_surface,
        content_size,
        alia_vec2f_make(anchor.size.x, 0.f),
        alia_box_make(
            alia_vec2f_make(0.f, 0.f), alia_vec2i_to_vec2f(surface_size)));

    auto ui = [&](alia_ui_context* ctx) {
        if (!alia_layout_context_is_refresh(ctx))
        {
            ctx->system = surface.system;
            *alia_ctx_scratch(alia_ctx_kernel(ctx)) = scratch.bump;
            alia_ctx_geometry(ctx)->offset = incoming_offset;
            alia_ctx_geometry(ctx)->z_base = ALIA_DEFAULT_Z_BASE;
            alia_ctx_geometry(ctx)->clip.box = incoming_clip;
        }

        alia::popup(*ctx, anchor, [&] {
            if (alia_layout_context_is_refresh(ctx))
            {
                alia_layout_leaf_emit(
                    ctx,
                    alia_layout_content_metrics{
                        .size = content_size, .ascent = 0.f, .descent = 0.f},
                    0);
                return;
            }

            CHECK(alia_box_equal(
                alia_geometry_get_clip_box(ctx),
                alia_box_make(
                    alia_vec2f_make(0.f, 0.f),
                    alia_vec2i_to_vec2f(surface_size))));
            CHECK(
                alia_ctx_geometry(ctx)->z_base
                == ALIA_DEFAULT_Z_BASE + ALIA_Z_LAYER_SPACING);
            CHECK(
                alia_vec2f_equal(alia_ctx_geometry(ctx)->offset, placed.min));
            CHECK(alia_box_equal(
                alia_layout_consume_box(ctx),
                alia_box_make(alia_vec2f_make(0.f, 0.f), placed.size)));
        });

        if (!alia_layout_context_is_refresh(ctx))
        {
            CHECK(alia_vec2f_equal(
                alia_ctx_geometry(ctx)->offset, incoming_offset));
            CHECK(alia_ctx_geometry(ctx)->z_base == ALIA_DEFAULT_Z_BASE);
            CHECK(alia_box_equal(
                alia_ctx_geometry(ctx)->clip.box, incoming_clip));
        }
    };

    layout_test_fixture_run_refresh(fixture, ui);
    layout_test_fixture_resolve(fixture, alia_vec2f_make(0.f, 0.f));
    layout_test_fixture_run_spatial(fixture, ui);

    layout_test_fixture_destroy(fixture);
}
