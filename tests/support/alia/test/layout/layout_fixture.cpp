#include <alia/test/layout/layout_fixture.hpp>

#include <alia/abi/base/allocator.h>
#include <alia/abi/base/arena.h>
#include <alia/abi/base/memory.h>
#include <alia/abi/ui/system/memory.h>
#include <alia/base/memory.h>
#include <alia/abi/base/stack.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/events.h>
#include <alia/abi/ui/geometry/api.h>
#include <alia/base/stack.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/substrate.h>
#include <alia/ui/layout/system.h>

#include <cstdlib>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace {

size_t const stack_buffer_size = 64 * 1024;
size_t const substrate_scratch_capacity = 16 * 1024;

// Large enough for layout UI that uses a modest amount of substrate state
// (e.g. detached handles). Pre-sized so tests avoid discovery re-traversal.
alia_struct_spec const root_block_spec = {.size = 4096u, .align = 16u};

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

void*
substrate_alloc(void* user_data, size_t size, size_t alignment)
{
    (void) user_data;
    if (alignment <= sizeof(void*))
        return std::malloc(size);
    return aligned_alloc_portable(alignment, size);
}

void
substrate_free(void* user_data, void* ptr, size_t size, size_t alignment)
{
    (void) user_data;
    (void) size;
    if (alignment <= sizeof(void*))
        std::free(ptr);
    else
        aligned_free_portable(ptr);
}

} // namespace

struct layout_fixture
{
    alia_memory_system memory{};
    alia_layout_system layout = {};
    alia_stack stack = {};
    void* stack_buffer = nullptr;

    float spacing = 0.f;
    alia_geometry_context geometry = {.scale = 1.f};

    alia_event_traversal event_traversal = {};
    alia_event refresh_event = {};
    alia_event draw_event = {};

    alia_layout_context layout_context = {};
    alia_ui_context context = {};

    alia_substrate_system substrate_system{};
    alia_substrate_traversal substrate_traversal{};
    void* substrate_scratch_buffer = nullptr;
    alia_bump_allocator substrate_scratch{};
    bool substrate_ready = false;
};

static void
wire_context(layout_fixture& fixture, bool refresh)
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

    fixture.layout_context.system = &fixture.layout;
    fixture.layout_context.emission.next_ptr
        = &fixture.layout.root.first_child;
    if (refresh)
    {
        fixture.layout_context.emission.arena = alia_memory_open_bump(
            &fixture.memory, fixture.layout.node_pool);
        fixture.layout_context.placement = {};
    }
    else
    {
        fixture.layout_context.emission.arena = {};
        fixture.layout_context.placement = alia_memory_open_bump(
            &fixture.memory, fixture.layout.placement_pool);
    }

    alia_stack_reset(&fixture.stack);

    if (fixture.substrate_ready)
    {
        alia_arena_reset(&fixture.substrate_scratch);
        alia::substrate_traversal_init(
            fixture.substrate_traversal,
            fixture.substrate_system,
            &fixture.substrate_scratch,
            0,
            true);
    }

    fixture.context = alia_ui_context{
        .kernel = {
            .object = nullptr,
            .substrate = fixture.substrate_ready ? &fixture.substrate_traversal
                                                : nullptr,
            .events = &fixture.event_traversal,
            .stack = &fixture.stack,
            .tick_count = 0,
        },
        .system = nullptr,
        .style = {.spacing = fixture.spacing},
        .geometry = fixture.geometry,
        .input = nullptr,
        .layout = &fixture.layout_context,
        .draw = nullptr,
        .palette = nullptr,
    };
}

static void
run_with_substrate_root(
    layout_fixture& fixture, void (*fn)(alia_ui_context*, void*), void* user)
{
    if (fixture.substrate_ready)
    {
        alia_struct_spec spec = root_block_spec;
        alia_substrate_begin_block(
            alia_ctx_kernel(&fixture.context),
            &fixture.substrate_system.root_anchor,
            &spec);
        fn(&fixture.context, user);
        (void) alia_substrate_end_block(alia_ctx_kernel(&fixture.context));
    }
    else
    {
        fn(&fixture.context, user);
    }
}

layout_fixture*
layout_fixture_create()
{
    auto* fixture
        = static_cast<layout_fixture*>(std::malloc(sizeof(layout_fixture)));
    if (!fixture)
        return nullptr;

    new (fixture) layout_fixture{};

    alia_memory_system_init(&fixture->memory, alia_memory_ops_system());
    alia_layout_system_init(&fixture->layout, &fixture->memory);

    fixture->stack_buffer
        = aligned_alloc_portable(ALIA_MAX_ALIGN, stack_buffer_size);
    if (!fixture->stack_buffer)
    {
        layout_fixture_destroy(fixture);
        return nullptr;
    }

    alia_stack_bind(
        &fixture->stack,
        fixture->stack_buffer,
        stack_buffer_size,
        alia_grow_callback_none());

    fixture->substrate_scratch_buffer = aligned_alloc_portable(
        ALIA_MAX_ALIGN, substrate_scratch_capacity);
    if (!fixture->substrate_scratch_buffer)
    {
        layout_fixture_destroy(fixture);
        return nullptr;
    }
    alia_bump_allocator_bind(
        &fixture->substrate_scratch,
        fixture->substrate_scratch_buffer,
        substrate_scratch_capacity,
        alia_grow_callback_none());

    alia_general_allocator allocator
        = {.alloc = substrate_alloc,
           .free = substrate_free,
           .user_data = nullptr};
    alia::substrate_system_init(fixture->substrate_system, allocator);
    alia::substrate_traversal_init(
        fixture->substrate_traversal,
        fixture->substrate_system,
        &fixture->substrate_scratch,
        0,
        true);
    fixture->substrate_ready = true;

    return fixture;
}

void
layout_fixture_destroy(layout_fixture* fixture)
{
    if (!fixture)
        return;

    if (fixture->substrate_ready)
    {
        if (fixture->substrate_system.root_anchor.block)
            alia::substrate_system_destroy(fixture->substrate_system);
        fixture->substrate_ready = false;
    }

    if (fixture->substrate_scratch_buffer)
    {
        aligned_free_portable(fixture->substrate_scratch_buffer);
        fixture->substrate_scratch_buffer = nullptr;
    }

    alia_memory_system_destroy(&fixture->memory);

    if (fixture->stack_buffer)
    {
        aligned_free_portable(fixture->stack_buffer);
        fixture->stack_buffer = nullptr;
    }

    std::free(fixture);
}

alia_ui_context*
layout_fixture_context(layout_fixture* fixture)
{
    return fixture ? &fixture->context : nullptr;
}

alia_layout_system*
layout_fixture_layout_system(layout_fixture* fixture)
{
    return fixture ? &fixture->layout : nullptr;
}

alia_layout_node*
layout_fixture_root_child(layout_fixture* fixture)
{
    return fixture ? fixture->layout.root.first_child : nullptr;
}

alia_memory_pool_stats
layout_fixture_node_arena_stats(layout_fixture* fixture)
{
    if (!fixture)
        return alia_memory_pool_stats{};
    return alia_memory_pool_query(
        &fixture->memory, fixture->layout.node_pool);
}

void*
layout_fixture_placement_arena_identity(layout_fixture* fixture)
{
    return fixture ? static_cast<void*>(&fixture->layout.placement) : nullptr;
}

void
layout_fixture_set_spacing(layout_fixture* fixture, float spacing)
{
    if (!fixture)
        return;
    fixture->spacing = spacing;
    fixture->context.style.spacing = spacing;
}

void
layout_fixture_set_scale(layout_fixture* fixture, float scale)
{
    if (!fixture)
        return;
    fixture->geometry.scale = scale;
    fixture->context.geometry.scale = scale;
}

void
layout_fixture_run_refresh_impl(
    layout_fixture* fixture, void (*fn)(alia_ui_context*, void*), void* user)
{
    if (!fixture || !fn)
        return;

    fixture->layout.root.first_child = nullptr;
    wire_context(*fixture, true);
    run_with_substrate_root(*fixture, fn, user);
    *fixture->layout_context.emission.next_ptr = nullptr;
    alia_memory_commit_bump(
        &fixture->memory,
        fixture->layout.node_pool,
        &fixture->layout_context.emission.arena);
    alia_memory_trim(
        &fixture->memory, fixture->layout.node_pool, ALIA_MEMORY_RETAIN_USED);
}

void
layout_fixture_resolve(layout_fixture* fixture, alia_vec2f available)
{
    if (!fixture)
        return;
    alia_layout_system_resolve(&fixture->layout, available);
}

void
layout_fixture_run_spatial_impl(
    layout_fixture* fixture, void (*fn)(alia_ui_context*, void*), void* user)
{
    if (!fixture || !fn)
        return;

    wire_context(*fixture, false);
    run_with_substrate_root(*fixture, fn, user);
    alia_memory_discard_bump(
        &fixture->memory, fixture->layout.placement_pool);
}

bool
alia_layout_context_is_refresh(alia_ui_context* ctx)
{
    return ctx != nullptr && alia::is_refresh_event(*ctx);
}
