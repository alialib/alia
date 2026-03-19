#include <alia/abi/base/arena.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

static size_t const max_arena_capacity = 16384;

static size_t
test_grow_fn(
    void* user, void* base, size_t existing_capacity, size_t bytes_requested)
{
    (void) user;
    (void) base;
    (void) bytes_requested;

    TEST_ASSERT(existing_capacity * 2 <= max_arena_capacity);
    return existing_capacity * 2;
}

static void
test_free(void* user, void* base, size_t capacity)
{
    (void) user;
    (void) capacity;
    free(base);
}

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

typedef struct test_rig
{
    void* storage;
    alia_arena* arena;
    alia_bump_allocator alloc;
} test_rig;

static void
test_rig_init(test_rig* rig, size_t initial_capacity)
{
    alia_struct_spec spec = alia_arena_object_spec();
    rig->storage = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(rig->storage != NULL);

    void* buffer = malloc(max_arena_capacity);
    TEST_ASSERT(buffer != NULL);

    rig->arena = alia_arena_init(
        rig->storage,
        buffer,
        initial_capacity,
        (alia_arena_controller) {
            .user = NULL, .grow = test_grow_fn, .free = test_free});
    TEST_ASSERT(rig->arena != NULL);

    alia_bump_allocator_init(&rig->alloc, rig->arena);
}

static void
test_rig_destroy(test_rig* rig)
{
    alia_arena_cleanup(rig->arena);
    rig->arena = NULL;
    aligned_free_portable(rig->storage);
    rig->storage = NULL;
}

static alia_bump_allocator*
test_rig_view(test_rig* rig)
{
    return &rig->alloc;
}

static void
test_construct_destruct(void)
{
    test_rig rig;
    test_rig_init(&rig, 1024);

    alia_arena_stats stats = alia_arena_get_stats(rig.arena);
    TEST_CHECK(stats.current_usage == 0);
    TEST_CHECK(stats.peak_usage == 0);

    test_rig_destroy(&rig);
}

static void
test_allocation_alignment(void)
{
    test_rig rig;
    test_rig_init(&rig, 4096);

    // Allocate a bunch with varying alignments and verify alignment.
    for (size_t i = 0; i < 4; ++i)
    {
        size_t size = 64u << i;
        size_t align = (size_t) ALIA_MIN_ALIGN << i;
        alia_offset p
            = alia_arena_alloc_aligned(test_rig_view(&rig), size, align);
        TEST_CHECK((p % align) == 0);
        memset(
            alia_arena_ptr(test_rig_view(&rig), p), 0, size); // Touch memory.
    }

    test_rig_destroy(&rig);
}

static void
test_allocation_growth(void)
{
    test_rig rig;
    test_rig_init(&rig, 1024);

    alia_offset p = alia_arena_alloc(test_rig_view(&rig), 1600);
    TEST_CHECK(p == 0);
    TEST_CHECK(rig.alloc.capacity == 2048);

    test_rig_destroy(&rig);
}

static void
test_mark_jump_reset(void)
{
    test_rig rig;
    test_rig_init(&rig, 1024);

    // Do a base allocation.
    alia_offset a0 = alia_arena_alloc(test_rig_view(&rig), 64);
    (void) a0;

    // Set a mark and allocate more data.
    alia_arena_marker m1 = alia_arena_mark(test_rig_view(&rig));
    alia_offset a1 = alia_arena_alloc(test_rig_view(&rig), 80);

    // Mark again and allocate even more.
    alia_arena_marker m2 = alia_arena_mark(test_rig_view(&rig));
    alia_offset a2 = alia_arena_alloc(test_rig_view(&rig), 96);

    // Jump back to `m2`. The next allocation should reuse the address of `a2`.
    alia_arena_jump(test_rig_view(&rig), m2);
    alia_offset b2 = alia_arena_alloc(test_rig_view(&rig), 96);
    TEST_CHECK(b2 == a2);

    // Jump back to `m1`. The next allocation should reuse the address of `a1`.
    alia_arena_jump(test_rig_view(&rig), m1);
    alia_offset b1 = alia_arena_alloc(test_rig_view(&rig), 80);
    TEST_CHECK(b1 == a1);

    // Commit peak so get_stats returns it.
    alia_bump_allocator_commit_peak(test_rig_view(&rig));
    alia_arena_stats stats = alia_arena_get_stats(rig.arena);
    TEST_CHECK(stats.peak_usage == 240);

    // Reset the allocator and confirm peak is unchanged (still committed).
    alia_arena_reset(test_rig_view(&rig));
    stats = alia_arena_get_stats(rig.arena);
    TEST_CHECK(stats.peak_usage == 240);

    test_rig_destroy(&rig);
}

static void
test_stats(void)
{
    test_rig rig;
    test_rig_init(&rig, 1024);

    (void) alia_arena_alloc(test_rig_view(&rig), 112);
    (void) alia_arena_alloc(test_rig_view(&rig), 203);

    alia_bump_allocator_commit_peak(test_rig_view(&rig));
    alia_arena_stats stats = alia_arena_get_stats(rig.arena);
    TEST_CHECK(stats.peak_usage == 315);
    TEST_CHECK(rig.alloc.offset == 315);

    alia_arena_reset(test_rig_view(&rig));

    stats = alia_arena_get_stats(rig.arena);
    TEST_CHECK(stats.peak_usage == 315);
    TEST_CHECK(rig.alloc.offset == 0);

    test_rig_destroy(&rig);
}

void
arena_tests(void)
{
    test_construct_destruct();
    test_allocation_alignment();
    test_allocation_growth();
    test_mark_jump_reset();
    test_stats();
}
