#include <alia/abi/kernel/substrate.h>

#include <alia/abi/base/arena.h>
#include <alia/abi/ui/events.h>
#include <alia/impl/events.hpp>

#include "substrate_harness.h"
#include <alia/kernel/substrate.h>

#include <cstdlib>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace {

size_t const max_arena_capacity = 16384;

static void*
aligned_alloc_portable(size_t align, size_t size)
{
#if defined(_MSC_VER)
    return _aligned_malloc(size, align);
#else
    // C11 aligned_alloc requires size to be a multiple of align.
    size_t rounded = (size + align - 1u) / align * align;
    return aligned_alloc(align, rounded);
#endif
}

static void
aligned_free_portable(void* p)
{
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    free(p);
#endif
}

size_t
test_grow_fn(
    void* user, void* base, size_t existing_capacity, size_t bytes_requested)
{
    (void) user;
    (void) base;
    (void) bytes_requested;

    ALIA_ASSERT(existing_capacity * 2 <= max_arena_capacity);
    return existing_capacity * 2;
}

void
test_free_fn(void* user, void* base, size_t capacity)
{
    (void) user;
    (void) capacity;
    aligned_free_portable(base);
}

} // namespace

struct alia_test_substrate_fixture
{
    alia_substrate_system system;
    alia_substrate_traversal traversal;

    // scratch arena used by substrate discovery-mode allocations
    alia_arena* arena = nullptr;
    void* arena_storage = nullptr;
    alia_bump_allocator scratch = {};

    // minimal event state so discovery mode can mark refresh incomplete
    alia_event_traversal event_traversal = {};
    alia_event refresh_event = {};
};

extern "C" {

alia_test_substrate_fixture*
alia_test_substrate_fixture_create(alia_general_allocator allocator)
{
    auto* fixture = static_cast<alia_test_substrate_fixture*>(
        std::malloc(sizeof(alia_test_substrate_fixture)));
    if (!fixture)
        return nullptr;

    // arena storage
    alia_struct_spec arena_spec = alia_arena_object_spec();
    fixture->arena_storage
        = aligned_alloc_portable(arena_spec.align, arena_spec.size);
    if (!fixture->arena_storage)
    {
        std::free(fixture);
        return nullptr;
    }

    void* arena_buffer
        = aligned_alloc_portable(ALIA_MAX_ALIGN, max_arena_capacity);
    if (!arena_buffer)
    {
        aligned_free_portable(fixture->arena_storage);
        fixture->arena_storage = NULL;
        std::free(fixture);
        return nullptr;
    }

    fixture->arena = alia_arena_init(
        fixture->arena_storage,
        arena_buffer,
        max_arena_capacity / 2u, /* initial_capacity */
        alia_arena_controller{
            .user = nullptr, .grow = test_grow_fn, .free = test_free_fn});

    alia_bump_allocator_init(&fixture->scratch, fixture->arena);

    // substrate system + traversal setup
    alia::substrate_system_init(fixture->system, allocator);
    alia::substrate_traversal_init(
        fixture->traversal, fixture->system, &fixture->scratch);

    return fixture;
}

void
alia_test_substrate_fixture_destroy(alia_test_substrate_fixture* fixture)
{
    if (!fixture)
        return;

    // Run destructor lists and free substrate block storage if root still
    // exists (tests may have explicitly cleaned it already).
    if (fixture->system.root_anchor.block)
        alia::substrate_system_cleanup(fixture->system);

    if (fixture->arena)
    {
        alia_arena_cleanup(fixture->arena);
        fixture->arena = nullptr;
    }

    if (fixture->arena_storage)
    {
        aligned_free_portable(fixture->arena_storage);
        fixture->arena_storage = NULL;
    }

    std::free(fixture);
}

alia_substrate_traversal*
alia_test_substrate_fixture_traversal(alia_test_substrate_fixture* fixture)
{
    return fixture ? &fixture->traversal : nullptr;
}

alia_substrate_system*
alia_test_substrate_fixture_system(alia_test_substrate_fixture* fixture)
{
    return fixture ? &fixture->system : nullptr;
}

alia_substrate_anchor*
alia_test_substrate_fixture_root_anchor(alia_test_substrate_fixture* fixture)
{
    return fixture ? &fixture->system.root_anchor : nullptr;
}

void
alia_test_substrate_fixture_prepare_refresh_event(
    alia_test_substrate_fixture* fixture, alia_context* ctx)
{
    if (!fixture || !ctx)
        return;

    fixture->refresh_event = alia_make_refresh_event(alia_refresh{false});
    fixture->event_traversal.event = &fixture->refresh_event;
    fixture->event_traversal.aborted = false;
    ctx->events = &fixture->event_traversal;
}

void
alia_test_substrate_fixture_reset_traversal(
    alia_test_substrate_fixture* fixture)
{
    if (!fixture)
        return;

    // Reset scratch arena and traversal state so discovery allocations start
    // from a clean offset each time.
    alia_arena_reset(&fixture->scratch);
    alia::substrate_traversal_init(
        fixture->traversal, fixture->system, &fixture->scratch);
}

void
alia_test_substrate_fixture_cleanup_root_block(
    alia_test_substrate_fixture* fixture)
{
    if (!fixture)
        return;

    if (fixture->system.root_anchor.block)
    {
        alia::substrate_block_cleanup(
            fixture->system, fixture->system.root_anchor.block);
        fixture->system.root_anchor.block = nullptr;
    }
}

} // extern "C"
