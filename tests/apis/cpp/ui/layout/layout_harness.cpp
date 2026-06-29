#include "layout_harness.hpp"

#include <alia/abi/base/stack.h>
#include <alia/abi/ui/events.h>
#include <alia/abi/ui/layout/system.h>
#include <alia/abi/ui/style.h>
#include <alia/base/stack.h>
#include <alia/ui/layout/system.h>

#include <cstdlib>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace {

size_t const stack_buffer_size = 64 * 1024;

void*
aligned_alloc_portable(size_t align, size_t size)
{
#if defined(_MSC_VER)
    return _aligned_malloc(size, align);
#else
    size_t rounded = (size + align - 1u) / align * align;
    return aligned_alloc(align, rounded);
#endif
}

void
aligned_free_portable(void* p)
{
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    free(p);
#endif
}

} // namespace

struct layout_test_fixture
{
    alia_layout_system layout = {};
    alia_stack stack = {};
    void* stack_buffer = nullptr;

    alia_style style = {.spacing = 0.f};

    alia_event_traversal event_traversal = {};
    alia_event refresh_event = {};
    alia_event draw_event = {};

    alia_layout_context layout_context = {};
    alia_context context = {};
};

static void
wire_context(layout_test_fixture& fixture, bool refresh)
{
    if (refresh)
    {
        fixture.refresh_event
            = alia_make_refresh_event(alia_refresh{.incomplete = false});
        fixture.event_traversal.event = &fixture.refresh_event;
    }
    else
    {
        fixture.draw_event
            = alia_make_draw_event(alia_draw{.context = nullptr});
        fixture.event_traversal.event = &fixture.draw_event;
    }
    fixture.event_traversal.aborted = false;

    fixture.layout_context.emission.next_ptr
        = &fixture.layout.root.first_child;
    alia_bump_allocator_init(
        &fixture.layout_context.emission.arena, &fixture.layout.node_arena);
    alia_bump_allocator_init(
        &fixture.layout_context.placement, &fixture.layout.placement_arena);

    alia_stack_reset(&fixture.stack);

    fixture.context = alia_context{
        .kernel = nullptr,
        .substrate = nullptr,
        .events = &fixture.event_traversal,
        .stack = &fixture.stack,
        .scratch = nullptr,
        .tick_count = 0,
        .system = nullptr,
        .style = &fixture.style,
        .geometry = nullptr,
        .input = nullptr,
        .layout = &fixture.layout_context,
        .draw = nullptr,
        .palette = nullptr,
    };
}

layout_test_fixture*
layout_test_fixture_create()
{
    auto* fixture = static_cast<layout_test_fixture*>(
        std::malloc(sizeof(layout_test_fixture)));
    if (!fixture)
        return nullptr;

    new (fixture) layout_test_fixture{};

    alia_layout_system_init(&fixture->layout);

    fixture->stack_buffer
        = aligned_alloc_portable(ALIA_MAX_ALIGN, stack_buffer_size);
    if (!fixture->stack_buffer)
    {
        layout_test_fixture_destroy(fixture);
        return nullptr;
    }

    alia_stack_init(&fixture->stack, fixture->stack_buffer, stack_buffer_size);

    return fixture;
}

void
layout_test_fixture_destroy(layout_test_fixture* fixture)
{
    if (!fixture)
        return;

    alia_arena_destroy(&fixture->layout.node_arena);
    alia_arena_destroy(&fixture->layout.scratch_arena);
    alia_arena_destroy(&fixture->layout.placement_arena);

    if (fixture->stack_buffer)
    {
        alia_stack_destroy(&fixture->stack);
        aligned_free_portable(fixture->stack_buffer);
        fixture->stack_buffer = nullptr;
    }

    std::free(fixture);
}

alia_context*
layout_test_fixture_context(layout_test_fixture* fixture)
{
    return fixture ? &fixture->context : nullptr;
}

void
layout_test_fixture_set_spacing(layout_test_fixture* fixture, float spacing)
{
    if (fixture)
        fixture->style.spacing = spacing;
}

void
layout_test_fixture_run_refresh_impl(
    layout_test_fixture* fixture, void (*fn)(alia_context*, void*), void* user)
{
    if (!fixture || !fn)
        return;

    fixture->layout.root.first_child = nullptr;
    wire_context(*fixture, true);
    fn(&fixture->context, user);
    *fixture->layout_context.emission.next_ptr = nullptr;
}

void
layout_test_fixture_resolve(layout_test_fixture* fixture, alia_vec2f available)
{
    if (!fixture)
        return;
    alia_layout_system_resolve(&fixture->layout, available);
}

void
layout_test_fixture_run_spatial_impl(
    layout_test_fixture* fixture, void (*fn)(alia_context*, void*), void* user)
{
    if (!fixture || !fn)
        return;

    wire_context(*fixture, false);
    fn(&fixture->context, user);
}
