#include <alia/abi/base/memory.h>
#include <alia/abi/base/stack.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

static void*
aligned_alloc_portable(size_t align, size_t size)
{
#if defined(_MSC_VER)
    return _aligned_malloc(size, align);
#else
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

static alia_memory_system*
make_system(alia_memory_ops ops, void** storage_out)
{
    alia_struct_spec spec = alia_memory_system_object_spec();
    void* storage = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(storage != NULL);
    *storage_out = storage;
    alia_memory_system* memory = alia_memory_system_init(storage, ops);
    TEST_ASSERT(memory != NULL);
    return memory;
}

static void
test_heap_bump_and_idle_release(void)
{
    alia_memory_ops ops = alia_memory_ops_system();
    ops.vm = NULL;

    void* storage = NULL;
    alia_memory_system* memory = make_system(ops, &storage);

    alia_memory_pool_id id = alia_memory_register_pool(
        memory, ALIA_MEMORY_POOL_BUMP, ALIA_MEMORY_RELEASE_ON_IDLE);
    alia_memory_set_cap(memory, id, 4096);

    alia_bump_allocator bump = alia_memory_open_bump(memory, id);
    TEST_CHECK(bump.base != NULL);
    TEST_CHECK(bump.capacity == 4096);

    (void) alia_arena_alloc(&bump, 128);
    alia_memory_commit_bump(memory, id, &bump);

    alia_memory_pool_stats stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.used == 128);
    TEST_CHECK(stats.peak_used == 128);
    TEST_CHECK(stats.committed == 4096);

    alia_memory_release_idle(memory);
    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.reserved == 0);
    TEST_CHECK(stats.committed == 0);
    TEST_CHECK(stats.used == 0);
    TEST_CHECK(stats.peak_used == 128);

    bump = alia_memory_open_bump(memory, id);
    TEST_CHECK(bump.base != NULL);
    TEST_CHECK(bump.capacity == 4096);
    alia_memory_commit_bump(memory, id, &bump);
    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.used == 0);
    TEST_CHECK(stats.peak_used == 128);

    alia_memory_system_destroy(memory);
    aligned_free_portable(storage);
}

static void
test_publish_keeps_peak(void)
{
    alia_memory_ops ops = alia_memory_ops_system();
    ops.vm = NULL;

    void* storage = NULL;
    alia_memory_system* memory = make_system(ops, &storage);

    alia_memory_pool_id id
        = alia_memory_register_pool(memory, ALIA_MEMORY_POOL_BUMP, 0);
    alia_memory_set_cap(memory, id, 4096);

    alia_bump_allocator bump = alia_memory_open_bump(memory, id);
    (void) alia_arena_alloc(&bump, 64);
    (void) alia_arena_alloc(&bump, 80);
    (void) alia_arena_alloc(&bump, 96);
    alia_memory_commit_bump(memory, id, &bump);

    alia_memory_pool_stats stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.peak_used == 240);
    TEST_CHECK(stats.used == 240);

    bump = alia_memory_open_bump(memory, id);
    (void) alia_arena_alloc(&bump, 64);
    (void) alia_arena_alloc(&bump, 80);
    alia_memory_commit_bump(memory, id, &bump);

    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.peak_used == 240);
    TEST_CHECK(stats.used == 144);

    alia_memory_system_destroy(memory);
    aligned_free_portable(storage);
}

static void
test_caller_storage_is_kept(void)
{
    alia_memory_ops ops = alia_memory_ops_system();
    ops.vm = NULL;

    void* storage = NULL;
    alia_memory_system* memory = make_system(ops, &storage);

    void* buffer = aligned_alloc_portable(ALIA_MAX_ALIGN, 4096);
    TEST_ASSERT(buffer != NULL);

    alia_memory_pool_id id = alia_memory_register_pool(
        memory, ALIA_MEMORY_POOL_BUMP, ALIA_MEMORY_RELEASE_ON_IDLE);
    alia_memory_set_storage(memory, id, buffer, 4096);

    alia_bump_allocator bump = alia_memory_open_bump(memory, id);
    TEST_CHECK(bump.base == buffer);
    TEST_CHECK(bump.capacity == 4096);
    alia_memory_commit_bump(memory, id, &bump);

    alia_memory_release_idle(memory);
    bump = alia_memory_open_bump(memory, id);
    TEST_CHECK(bump.base == buffer);
    alia_memory_commit_bump(memory, id, &bump);

    alia_memory_system_destroy(memory);
    aligned_free_portable(buffer);
    aligned_free_portable(storage);
}

static void
test_virtual_commit_follows_use(void)
{
    alia_memory_ops ops = alia_memory_ops_system();
    if (!ops.vm)
        return;

    void* storage = NULL;
    alia_memory_system* memory = make_system(ops, &storage);

    alia_memory_pool_id id = alia_memory_register_pool(
        memory, ALIA_MEMORY_POOL_BUMP, ALIA_MEMORY_RELEASE_ON_IDLE);
    alia_memory_set_cap(memory, id, 1024u * 1024u);

    alia_bump_allocator bump = alia_memory_open_bump(memory, id);
    TEST_CHECK(bump.base != NULL);
    TEST_CHECK(bump.capacity == 0);

    alia_memory_pool_stats stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.reserved == 1024u * 1024u);
    TEST_CHECK(stats.committed == 0);

    (void) alia_arena_alloc(&bump, 128);

    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.committed == 64u * 1024u);
    TEST_CHECK(stats.reserved == 1024u * 1024u);
    TEST_CHECK(bump.capacity == 64u * 1024u);

    alia_memory_commit_bump(memory, id, &bump);
    alia_memory_trim(memory, id, ALIA_MEMORY_RETAIN_PEAK);
    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.committed == 64u * 1024u);

    alia_memory_release_idle(memory);
    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.committed == 0);
    TEST_CHECK(stats.reserved == 1024u * 1024u);
    bump = alia_memory_open_bump(memory, id);
    TEST_CHECK(bump.base != NULL);
    TEST_CHECK(bump.capacity == 0);
    alia_memory_commit_bump(memory, id, &bump);

    alia_memory_system_destroy(memory);
    aligned_free_portable(storage);
}

static void
test_stack_virtual_commit(void)
{
    alia_memory_ops ops = alia_memory_ops_system();
    if (!ops.vm)
        return;

    void* storage = NULL;
    alia_memory_system* memory = make_system(ops, &storage);

    alia_memory_pool_id id = alia_memory_register_pool(
        memory, ALIA_MEMORY_POOL_STACK, ALIA_MEMORY_RELEASE_ON_IDLE);
    alia_memory_set_cap(memory, id, 1024u * 1024u);

    alia_stack stack = alia_memory_open_stack(memory, id);
    alia_memory_pool_stats stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.reserved == 1024u * 1024u);
    TEST_CHECK(stats.committed == 0);

    void* payload = alia_stack_push(&stack, 64, NULL);
    TEST_CHECK(payload != NULL);
    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.committed == 64u * 1024u);

    alia_memory_commit_stack(memory, id, &stack);
    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.used > 0);
    TEST_CHECK(stats.peak_used == stats.used);

    alia_memory_trim(memory, id, ALIA_MEMORY_RETAIN_PEAK);
    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.committed == 64u * 1024u);

    stack = alia_memory_open_stack(memory, id);
    alia_memory_commit_stack(memory, id, &stack);
    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.used == 0);
    TEST_CHECK(stats.peak_used > 0);

    alia_memory_trim(memory, id, ALIA_MEMORY_RETAIN_PEAK);
    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.committed == 64u * 1024u);

    size_t const lifetime_peak = stats.peak_used;
    alia_memory_release_idle(memory);
    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.committed == 0);
    TEST_CHECK(stats.used == 0);
    TEST_CHECK(stats.peak_used == lifetime_peak);
    TEST_CHECK(stats.reserved == 1024u * 1024u);

    alia_memory_system_destroy(memory);
    aligned_free_portable(storage);
}

static void
test_trim_retain(void)
{
    alia_memory_ops ops = alia_memory_ops_system();
    if (!ops.vm)
        return;

    void* storage = NULL;
    alia_memory_system* memory = make_system(ops, &storage);

    alia_memory_pool_id id
        = alia_memory_register_pool(memory, ALIA_MEMORY_POOL_BUMP, 0);
    alia_memory_set_cap(memory, id, 1024u * 1024u);

    alia_bump_allocator bump = alia_memory_open_bump(memory, id);
    (void) alia_arena_alloc(&bump, 64u * 1024u + 1u);
    alia_memory_commit_bump(memory, id, &bump);
    alia_memory_trim(memory, id, ALIA_MEMORY_RETAIN_PEAK);

    alia_memory_pool_stats stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.committed == 128u * 1024u);

    bump = alia_memory_open_bump(memory, id);
    (void) alia_arena_alloc(&bump, 128);
    alia_memory_commit_bump(memory, id, &bump);
    alia_memory_trim(memory, id, ALIA_MEMORY_RETAIN_PEAK);
    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.committed == 128u * 1024u);
    TEST_CHECK(stats.used < stats.peak_used);

    alia_memory_trim(memory, id, ALIA_MEMORY_RETAIN_USED);
    stats = alia_memory_pool_query(memory, id);
    TEST_CHECK(stats.committed == 64u * 1024u);
    TEST_CHECK(stats.peak_used > stats.committed);

    alia_memory_system_destroy(memory);
    aligned_free_portable(storage);
}

void
memory_tests(void)
{
    test_heap_bump_and_idle_release();
    test_publish_keeps_peak();
    test_caller_storage_is_kept();
    test_virtual_commit_follows_use();
    test_stack_virtual_commit();
    test_trim_retain();
}
