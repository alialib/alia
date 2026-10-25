#include <alia/abi/base/arena.h>
#include <alia/abi/base/stack.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/context_ids.h>
#include <alia/abi/ui/events.h>
#include <alia/abi/ui/styling/api.h>
#include <alia/abi/ui/text.h>
#include <alia/impl/context.hpp>
#include <alia/impl/events.hpp>
#include <alia/test/kernel/substrate_fixture.h>

#include <doctest/doctest.h>

#include <cstdint>
#include <cstdlib>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

using namespace alia;
using namespace alia::operators;

namespace {

void*
test_alloc(void*, size_t size, size_t alignment)
{
#if defined(_MSC_VER)
    return _aligned_malloc(size, alignment < 1 ? 1 : alignment);
#else
    size_t const align = alignment < sizeof(void*) ? sizeof(void*) : alignment;
    size_t const rounded = (size + align - 1u) / align * align;
    return std::aligned_alloc(align, rounded);
#endif
}

void
test_free(void*, void* ptr, size_t, size_t)
{
#if defined(_MSC_VER)
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

struct id_pass
{
    alignas(16) unsigned char scratch_mem[16 * 1024];
    alignas(16) unsigned char stack_mem[16 * 1024];
    alia_ui_context ctx{};
    alia_stack stack{};

    id_pass()
    {
        alia_bump_allocator_bind(
            &ctx.kernel.scratch,
            scratch_mem,
            sizeof(scratch_mem),
            alia_grow_callback_none());
        alia_stack_bind(
            &stack, stack_mem, sizeof(stack_mem), alia_grow_callback_none());
        ctx.kernel.stack = &stack;
        ctx.id.metrics = alia_id_view_unit();
        ctx.id.paint = alia_id_view_unit();
    }
};

} // namespace

TEST_CASE("context id push and pop")
{
    id_pass pass;
    alia_id_view const metrics = alia_id_view_make_u64(3);
    alia_id_view const paint = alia_id_view_make_u64(9);

    alia_context_metrics_id_push(&pass.ctx, metrics);
    CHECK((alia_context_metrics_id(&pass.ctx) == metrics));
    CHECK(alia_id_view_is_unit(alia_context_paint_id(&pass.ctx)));

    alia_context_paint_id_push(&pass.ctx, paint);
    CHECK((alia_context_paint_id(&pass.ctx) == paint));
    CHECK((alia_context_metrics_id(&pass.ctx) == metrics));

    alia_context_metrics_id_push(&pass.ctx, alia_id_view_make_u64(4));
    CHECK((alia_context_metrics_id(&pass.ctx) == alia_id_view_make_u64(4)));
    alia_context_metrics_id_pop(&pass.ctx);
    CHECK((alia_context_metrics_id(&pass.ctx) == metrics));

    alia_context_paint_id_pop(&pass.ctx);
    CHECK(alia_id_view_is_unit(alia_context_paint_id(&pass.ctx)));
    alia_context_metrics_id_pop(&pass.ctx);
    CHECK(alia_id_view_is_unit(alia_context_metrics_id(&pass.ctx)));

    alia_resolved_font const font{};
    alia_font_push(&pass.ctx, &font);
    alia_spacing_push(&pass.ctx, 12.f);
    alia_text_color_push(&pass.ctx, alia_srgba8_make(1, 2, 3, 4));
    CHECK(alia_id_view_is_unit(alia_context_metrics_id(&pass.ctx)));
    CHECK(alia_id_view_is_unit(alia_context_paint_id(&pass.ctx)));
    alia_text_color_pop(&pass.ctx);
    alia_spacing_pop(&pass.ctx);
    alia_font_pop(&pass.ctx);
}

struct collapse_pass
{
    alia_test_substrate_fixture* fixture = nullptr;
    id_pass pass;
    alia_struct_spec root_spec{};

    collapse_pass()
    {
        alia_general_allocator const allocator{
            .alloc = test_alloc, .free = test_free, .user_data = nullptr};
        fixture = alia_test_substrate_fixture_create(allocator);
        REQUIRE(fixture);
        pass.ctx.kernel.substrate
            = alia_test_substrate_fixture_traversal(fixture);
    }

    ~collapse_pass()
    {
        alia_test_substrate_fixture_destroy(fixture);
    }

    collapse_pass(collapse_pass const&) = delete;
    collapse_pass&
    operator=(collapse_pass const&) = delete;

    template<class Body>
    void
    visit(Body&& body)
    {
        alia_stack_reset(&pass.stack);
        alia_test_substrate_fixture_reset_traversal(fixture, true);
        alia_substrate_begin_block(
            alia_ctx_kernel(&pass.ctx),
            alia_test_substrate_fixture_root_anchor(fixture),
            &root_spec);
        body();
        root_spec = alia_substrate_end_block(alia_ctx_kernel(&pass.ctx));
    }
};

TEST_CASE("context id collapse")
{
    collapse_pass host;

    // Discover a block large enough for two collapse scopes.
    alia_test_substrate_fixture_prepare_refresh_event(
        host.fixture, &host.pass.ctx);
    host.visit([&] {
        context_id_collapse(ui_context{&host.pass.ctx}, [] {});
        context_id_collapse(ui_context{&host.pass.ctx}, [] {});
    });

    host.pass.ctx.id.metrics = alia_id_view_make_u64(1);
    host.pass.ctx.id.paint = alia_id_view_make_u64(4);
    alia_id_view const outer_metrics = alia_context_metrics_id(&host.pass.ctx);
    alia_id_view const outer_paint = alia_context_paint_id(&host.pass.ctx);

    alia_id_view published_metrics{};
    alia_id_view published_paint{};
    host.visit([&] {
        context_id_collapse(ui_context{&host.pass.ctx}, [&] {
            published_metrics = alia_context_metrics_id(&host.pass.ctx);
            published_paint = alia_context_paint_id(&host.pass.ctx);
            CHECK((published_metrics == alia_id_view_make_u64(1)));
            CHECK((published_paint == alia_id_view_make_u64(1)));
            alia_context_metrics_id_push(
                &host.pass.ctx, alia_id_view_make_u64(7));
            CHECK(
                (alia_context_metrics_id(&host.pass.ctx)
                 == alia_id_view_make_u64(7)));
            CHECK((alia_context_paint_id(&host.pass.ctx) == published_paint));
            alia_context_metrics_id_pop(&host.pass.ctx);
            CHECK((
                alia_context_metrics_id(&host.pass.ctx) == published_metrics));
        });
        CHECK((alia_context_metrics_id(&host.pass.ctx) == outer_metrics));
        CHECK((alia_context_paint_id(&host.pass.ctx) == outer_paint));

        context_id_collapse(ui_context{&host.pass.ctx}, [&] {
            CHECK(
                (alia_context_metrics_id(&host.pass.ctx)
                 == alia_id_view_make_u64(1)));
            CHECK(
                (alia_context_paint_id(&host.pass.ctx)
                 == alia_id_view_make_u64(1)));
        });
    });

    host.visit([&] {
        context_id_collapse(ui_context{&host.pass.ctx}, [&] {
            CHECK((
                alia_context_metrics_id(&host.pass.ctx) == published_metrics));
            CHECK((alia_context_paint_id(&host.pass.ctx) == published_paint));
        });
    });

    host.pass.ctx.id.paint = alia_id_view_make_u64(5);
    host.visit([&] {
        context_id_collapse(ui_context{&host.pass.ctx}, [&] {
            CHECK((
                alia_context_metrics_id(&host.pass.ctx) == published_metrics));
            CHECK((alia_context_paint_id(&host.pass.ctx) != published_paint));
            published_paint = alia_context_paint_id(&host.pass.ctx);
        });
    });

    alia_event draw = alia_make_draw_event(alia_draw{nullptr});
    alia_event_traversal traversal{};
    traversal.event = &draw;
    host.pass.ctx.kernel.events = &traversal;
    host.pass.ctx.id.metrics = alia_id_view_make_u64(9);
    host.visit([&] {
        context_id_collapse(ui_context{&host.pass.ctx}, [&] {
            CHECK((
                alia_context_metrics_id(&host.pass.ctx) == published_metrics));
            CHECK((alia_context_paint_id(&host.pass.ctx) == published_paint));
        });
    });

    alia_test_substrate_fixture_prepare_refresh_event(
        host.fixture, &host.pass.ctx);
    host.visit([&] {
        context_id_collapse(ui_context{&host.pass.ctx}, [&] {
            CHECK((
                alia_context_metrics_id(&host.pass.ctx) != published_metrics));
            CHECK((alia_context_paint_id(&host.pass.ctx) == published_paint));
            published_metrics = alia_context_metrics_id(&host.pass.ctx);
        });
    });

    char const long_metrics[] = "metrics-id-value";
    host.pass.ctx.id.metrics = alia_id_view_make_bytes(
        long_metrics, static_cast<uint32_t>(sizeof(long_metrics) - 1));
    host.visit([&] {
        context_id_collapse(ui_context{&host.pass.ctx}, [&] {
            CHECK((
                alia_context_metrics_id(&host.pass.ctx) != published_metrics));
            CHECK((alia_context_paint_id(&host.pass.ctx) == published_paint));
            published_metrics = alia_context_metrics_id(&host.pass.ctx);
        });
    });
    host.visit([&] {
        context_id_collapse(ui_context{&host.pass.ctx}, [&] {
            CHECK((
                alia_context_metrics_id(&host.pass.ctx) == published_metrics));
            CHECK((alia_context_paint_id(&host.pass.ctx) == published_paint));
        });
    });
}
