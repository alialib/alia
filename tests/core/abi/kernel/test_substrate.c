#include "substrate_harness.h"

#include <alia/abi/base/stack.h>
#include <alia/abi/context.h>
#include <alia/abi/kernel/ids.h>
#include <alia/abi/kernel/substrate.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Acutest main is provided by `tests/core/abi/runner.c`.
#define TEST_NO_MAIN
#include "acutest.h"

// --------- aligned allocation helpers ---------

static int
is_pow2_u32(uint32_t x)
{
    return x && ((x & (x - 1u)) == 0u);
}

static int
is_aligned_ptr(void const* p, uint32_t align)
{
    return is_pow2_u32(align)
        && ((((uintptr_t) p) & (uintptr_t) (align - 1u)) == 0u);
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

// --------- substrate allocator (honors alignment for block storage) ---------

static void*
test_block_alloc(void* user_data, size_t size, size_t alignment)
{
    (void) user_data;
    if (alignment <= sizeof(void*))
        return malloc(size);
    return aligned_alloc_portable(alignment, size);
}

static void
test_block_free(void* user_data, void* ptr, size_t size, size_t alignment)
{
    (void) user_data;
    (void) size;
    if (alignment <= sizeof(void*))
        free(ptr);
    else
        aligned_free_portable(ptr);
}

// --------- destructor ordering test support ---------

typedef struct test_state
{
    int ids[6];
    int count;
    int clear_cache_ids[6];
    int clear_cache_count;
} test_state;

typedef struct test_object
{
    test_state* state;
    int id;
    int cached_value;
} test_object;

static void
test_object_cleanup(
    alia_substrate_system* system, void* p, alia_substrate_cleanup_mode mode)
{
    (void) system;
    test_object* obj = (test_object*) p;
    switch (mode)
    {
        case ALIA_SUBSTRATE_CLEAR_CACHE:
            obj->state->clear_cache_ids[obj->state->clear_cache_count++]
                = obj->id;
            obj->cached_value = 0;
            break;
        case ALIA_SUBSTRATE_DESTROY:
            obj->state->ids[obj->state->count++] = obj->id;
            break;
    }
}

// --------- helpers to build a minimal C substrate context ---------

typedef struct substrate_test_ctx
{
    alia_test_substrate_fixture* fixture;
    alia_stack* stack;
    void* stack_obj_storage;
    void* stack_buffer;
    alia_context ctx;
} substrate_test_ctx;

static void
substrate_test_ctx_init(substrate_test_ctx* t)
{
    memset(t, 0, sizeof(*t));

    alia_general_allocator allocator = {
        .alloc = test_block_alloc, .free = test_block_free, .user_data = NULL};
    t->fixture = alia_test_substrate_fixture_create(allocator);
    TEST_ASSERT(t->fixture != NULL);

    // Stack initialization.
    alia_struct_spec stack_spec = alia_stack_object_spec();
    t->stack_obj_storage
        = aligned_alloc_portable(stack_spec.align, stack_spec.size);
    TEST_ASSERT(t->stack_obj_storage != NULL);
    t->stack_buffer = aligned_alloc_portable(ALIA_MAX_ALIGN, 64u * 1024u);
    TEST_ASSERT(t->stack_buffer != NULL);

    t->stack
        = alia_stack_init(t->stack_obj_storage, t->stack_buffer, 64u * 1024u);
    TEST_ASSERT(t->stack != NULL);
    alia_stack_reset(t->stack);

    t->ctx = (alia_context) {0};
    t->ctx.substrate = alia_test_substrate_fixture_traversal(t->fixture);
    t->ctx.stack = t->stack;
}

static void
substrate_test_ctx_destroy(substrate_test_ctx* t)
{
    if (!t)
        return;

    if (t->stack)
        alia_stack_destroy(t->stack);

    if (t->stack_buffer)
        aligned_free_portable(t->stack_buffer);
    if (t->stack_obj_storage)
        aligned_free_portable(t->stack_obj_storage);

    if (t->fixture)
        alia_test_substrate_fixture_destroy(t->fixture);

    memset(t, 0, sizeof(*t));
}

// --------- tests ---------

static void
test_basic_block(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &spec);

    alia_substrate_usage_result use1
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(use1.ptr != NULL);
    TEST_CHECK(use1.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);

    alia_substrate_usage_result use2
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(use2.ptr != NULL);
    TEST_CHECK(((uint8_t*) use2.ptr - (uint8_t*) use1.ptr) == 256);
    TEST_CHECK(use2.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);

    alia_substrate_usage_result use3
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(use3.ptr != NULL);
    TEST_CHECK(((uint8_t*) use3.ptr - (uint8_t*) use2.ptr) == 256);
    TEST_CHECK(use3.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);

    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_basic_block_discovery(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);

    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    // Enter discovery mode by passing align=0.
    alia_struct_spec discovery_spec = {.size = 0u, .align = 0u};
    alia_test_substrate_fixture_prepare_refresh_event(t.fixture, &t.ctx);
    alia_substrate_begin_block(&t.ctx, root, &discovery_spec);

    alia_substrate_usage_result use1
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(use1.ptr != NULL);
    TEST_CHECK(use1.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY);

    alia_substrate_usage_result use2
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(use2.ptr != NULL);
    TEST_CHECK(((uint8_t*) use2.ptr - (uint8_t*) use1.ptr) == 256);
    TEST_CHECK(use2.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY);

    alia_substrate_usage_result use3
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(use3.ptr != NULL);
    TEST_CHECK(((uint8_t*) use3.ptr - (uint8_t*) use2.ptr) == 256);
    TEST_CHECK(use3.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY);

    alia_struct_spec computed = alia_substrate_end_block(&t.ctx);
    // 32 is the current size of a block header.
    // (Hopefully this will decrease soon.)
    TEST_CHECK(computed.size == 768u + 32u);
    TEST_CHECK(computed.align == 16u);

    // Now allocate/init using computed spec.
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_struct_spec init_spec = computed;
    alia_substrate_begin_block(&t.ctx, root, &init_spec);

    alia_substrate_usage_result init1
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(init1.ptr != NULL);
    TEST_CHECK(init1.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);

    alia_substrate_usage_result init2
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(init2.ptr != NULL);
    TEST_CHECK(((uint8_t*) init2.ptr - (uint8_t*) init1.ptr) == 256);
    TEST_CHECK(init2.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);

    alia_substrate_usage_result init3
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(init3.ptr != NULL);
    TEST_CHECK(((uint8_t*) init3.ptr - (uint8_t*) init2.ptr) == 256);
    TEST_CHECK(init3.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);

    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_substrate_use_anchor(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &spec);

    alia_substrate_anchor* child = alia_substrate_use_anchor(&t.ctx);
    TEST_ASSERT(child != NULL);

    // Allocate inside the child.
    alia_substrate_begin_block(&t.ctx, child, &spec);
    alia_substrate_usage_result use1
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(use1.ptr != NULL);
    TEST_CHECK(use1.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);

    alia_substrate_usage_result use2
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(use2.ptr != NULL);
    TEST_CHECK(((uint8_t*) use2.ptr - (uint8_t*) use1.ptr) == 256);
    TEST_CHECK(use2.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);

    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_substrate_destructors(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    test_state state;
    memset(&state, 0, sizeof(state));
    state.count = 0;

    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &spec);

    // Root objects 1 and 2.
    alia_substrate_usage_result r1 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_cleanup);
    TEST_ASSERT(r1.ptr != NULL);
    TEST_CHECK(r1.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);
    ((test_object*) r1.ptr)->state = &state;
    ((test_object*) r1.ptr)->id = 1;

    alia_substrate_usage_result r2 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_cleanup);
    TEST_ASSERT(r2.ptr != NULL);
    TEST_CHECK(r2.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);
    ((test_object*) r2.ptr)->state = &state;
    ((test_object*) r2.ptr)->id = 2;

    // Child block.
    alia_substrate_anchor* child = alia_substrate_use_anchor(&t.ctx);
    TEST_ASSERT(child != NULL);

    alia_substrate_begin_block(&t.ctx, child, &spec);

    alia_substrate_usage_result c1 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_cleanup);
    TEST_ASSERT(c1.ptr != NULL);
    TEST_CHECK(c1.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);
    ((test_object*) c1.ptr)->state = &state;
    ((test_object*) c1.ptr)->id = 3;

    alia_substrate_usage_result c2 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_cleanup);
    TEST_ASSERT(c2.ptr != NULL);
    TEST_CHECK(c2.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);
    ((test_object*) c2.ptr)->state = &state;
    ((test_object*) c2.ptr)->id = 4;

    (void) alia_substrate_end_block(&t.ctx);

    // Back to root: objects 5 and 6.
    alia_substrate_usage_result r3 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_cleanup);
    TEST_ASSERT(r3.ptr != NULL);
    TEST_CHECK(r3.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);
    ((test_object*) r3.ptr)->state = &state;
    ((test_object*) r3.ptr)->id = 5;

    alia_substrate_usage_result r4 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_cleanup);
    TEST_ASSERT(r4.ptr != NULL);
    TEST_CHECK(r4.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);
    ((test_object*) r4.ptr)->state = &state;
    ((test_object*) r4.ptr)->id = 6;

    (void) alia_substrate_end_block(&t.ctx);

    // Destructors should not run until we cleanup the block.
    TEST_CHECK(state.count == 0);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);

    TEST_CHECK(state.count == 6);
    TEST_CHECK(state.ids[0] == 6);
    TEST_CHECK(state.ids[1] == 5);
    TEST_CHECK(state.ids[2] == 4);
    TEST_CHECK(state.ids[3] == 3);
    TEST_CHECK(state.ids[4] == 2);
    TEST_CHECK(state.ids[5] == 1);

    substrate_test_ctx_destroy(&t);
}

static void
test_substrate_deactivate_anchor(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    test_state state;
    memset(&state, 0, sizeof(state));

    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_anchor* child;
    void* child_obj_ptr;

    // First traversal: create a child block with cached data.
    alia_substrate_begin_block(&t.ctx, root, &spec);

    child = alia_substrate_use_anchor(&t.ctx);
    TEST_ASSERT(child != NULL);

    alia_substrate_begin_block(&t.ctx, child, &spec);

    alia_substrate_usage_result c1 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_cleanup);
    TEST_ASSERT(c1.ptr != NULL);
    TEST_CHECK(c1.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);
    child_obj_ptr = c1.ptr;
    ((test_object*) c1.ptr)->state = &state;
    ((test_object*) c1.ptr)->id = 1;
    ((test_object*) c1.ptr)->cached_value = 42;

    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    TEST_CHECK(state.clear_cache_count == 0);
    TEST_CHECK(state.count == 0);

    // Deactivate the child block: cached data should be cleared, but the block
    // is retained for possible reactivation.
    alia_substrate_deactivate_anchor(
        alia_test_substrate_fixture_system(t.fixture), child);
    TEST_CHECK(state.clear_cache_count == 1);
    TEST_CHECK(state.clear_cache_ids[0] == 1);
    TEST_CHECK(state.count == 0);
    TEST_CHECK(((test_object*) child_obj_ptr)->cached_value == 0);

    // Second traversal: the block should be reused in normal mode.
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &spec);

    alia_substrate_anchor* child2 = alia_substrate_use_anchor(&t.ctx);
    TEST_ASSERT(child2 == child);

    alia_substrate_begin_block(&t.ctx, child, &spec);

    alia_substrate_usage_result c2 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_cleanup);
    TEST_ASSERT(c2.ptr != NULL);
    TEST_CHECK(c2.ptr == child_obj_ptr);
    TEST_CHECK(c2.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL);

    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    // Full cleanup should still run destroy callbacks.
    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    TEST_CHECK(state.count == 1);
    TEST_CHECK(state.ids[0] == 1);

    substrate_test_ctx_destroy(&t);
}

static uint64_t
test_u64_from_id(alia_id_view id)
{
    TEST_ASSERT(id.type_id == ALIA_ID_TYPE_U64);
    uint64_t value;
    memcpy(&value, id.payload.inline_data, sizeof(value));
    return value;
}

static void
test_substrate_path_for_object_flat(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &spec);

    alia_substrate_usage_result use1
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    alia_substrate_usage_result use2
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(use1.ptr != NULL);
    TEST_ASSERT(use2.ptr != NULL);

    alia_id_view path1a = alia_substrate_path_for_object(&t.ctx, use1.ptr);
    alia_id_view path1b = alia_substrate_path_for_object(&t.ctx, use1.ptr);
    alia_id_view path2 = alia_substrate_path_for_object(&t.ctx, use2.ptr);

    TEST_CHECK(!alia_id_view_is_null(path1a));
    TEST_CHECK(path1a.type_id == ALIA_ID_TYPE_U64);
    TEST_CHECK(alia_id_view_equals(path1a, path1b));
    TEST_CHECK(alia_id_view_hash(path1a) == alia_id_view_hash(path1b));
    TEST_CHECK(!alia_id_view_equals(path1a, path2));
    TEST_CHECK(test_u64_from_id(path1a) == 32u);
    TEST_CHECK(test_u64_from_id(path2) == 288u);

    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_substrate_path_for_object_nested(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &spec);

    alia_substrate_anchor* child = alia_substrate_use_anchor(&t.ctx);
    TEST_ASSERT(child != NULL);

    alia_substrate_begin_block(&t.ctx, child, &spec);
    alia_substrate_usage_result use
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(use.ptr != NULL);

    alia_id_view path = alia_substrate_path_for_object(&t.ctx, use.ptr);
    TEST_CHECK(path.type_id == ALIA_ID_TYPE_PAIR);
    TEST_CHECK(
        test_u64_from_id(alia_id_view_pair_right(path))
        == (uint64_t) ((uint8_t*) use.ptr - (uint8_t*) child->block));
    TEST_CHECK(alia_id_view_pair_left(path).type_id == ALIA_ID_TYPE_U64);
    TEST_CHECK(
        test_u64_from_id(alia_id_view_pair_left(path))
        != test_u64_from_id(alia_id_view_pair_right(path)));

    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_substrate_path_for_object_stable_normal_pass(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &spec);
    alia_substrate_usage_result init_use
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(init_use.ptr != NULL);
    TEST_CHECK(init_use.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);
    alia_id_view init_path
        = alia_substrate_path_for_object(&t.ctx, init_use.ptr);
    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &spec);
    alia_substrate_usage_result normal_use
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(normal_use.ptr != NULL);
    TEST_CHECK(normal_use.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL);
    alia_id_view normal_path
        = alia_substrate_path_for_object(&t.ctx, normal_use.ptr);
    (void) alia_substrate_end_block(&t.ctx);

    TEST_CHECK(alia_id_view_equals(init_path, normal_path));

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_substrate_path_for_object_discovery(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);

    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_struct_spec discovery_spec = {.size = 0u, .align = 0u};
    alia_test_substrate_fixture_prepare_refresh_event(t.fixture, &t.ctx);
    alia_substrate_begin_block(&t.ctx, root, &discovery_spec);

    alia_substrate_usage_result use
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(use.ptr != NULL);
    TEST_CHECK(
        alia_id_view_is_null(alia_substrate_path_for_object(&t.ctx, use.ptr)));

    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_substrate_path_capture_smoke(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &spec);
    alia_substrate_anchor* child = alia_substrate_use_anchor(&t.ctx);
    alia_substrate_begin_block(&t.ctx, child, &spec);
    alia_substrate_usage_result use
        = alia_substrate_use_memory(&t.ctx, 256u, 16u);
    TEST_ASSERT(use.ptr != NULL);

    alia_id_view path = alia_substrate_path_for_object(&t.ctx, use.ptr);
    alia_struct_spec cap_spec = alia_captured_id_spec(path);
    TEST_ASSERT(cap_spec.size > 0);
    void* mem = aligned_alloc_portable(cap_spec.align, cap_spec.size);
    TEST_ASSERT(mem != NULL);
    alia_captured_id_capture_into(path, mem, cap_spec.size);
    alia_captured_id* captured = (alia_captured_id*) mem;
    TEST_CHECK(alia_captured_id_matches_view(captured, path));
    alia_captured_id_release(mem);
    aligned_free_portable(mem);

    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_substrate_generations(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    for (int generation = 0; generation < 2; ++generation)
    {
        alia_test_substrate_fixture_reset_traversal(t.fixture, true);
        alia_stack_reset(t.stack);

        alia_substrate_begin_block(&t.ctx, root, &spec);
        alia_substrate_usage_result use
            = alia_substrate_use_memory(&t.ctx, 256u, 16u);

        TEST_ASSERT(use.ptr != NULL);
        TEST_CHECK(use.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);
        TEST_CHECK(use.generation == generation);

        (void) alia_substrate_end_block(&t.ctx);

        alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    }

    substrate_test_ctx_destroy(&t);
}

// --------- key scope tests ---------

static alia_struct_spec keyed_block_spec = {.size = 512u, .align = 16u};
static alia_struct_spec root_block_spec = {.size = 4096u, .align = 16u};

typedef struct keyed_visit_state
{
    test_state destroy_state;
    void* ptrs[8];
    int ptr_count;
} keyed_visit_state;

static void*
keyed_visit_object(
    substrate_test_ctx* t,
    alia_substrate_key_scope* scope,
    uint64_t key,
    keyed_visit_state* state,
    int object_id)
{
    alia_substrate_begin_keyed_block(
        &t->ctx, scope, alia_id_view_make_u64(key), &keyed_block_spec);
    alia_substrate_usage_result use = alia_substrate_use_object(
        &t->ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_cleanup);
    TEST_ASSERT(use.ptr != NULL);
    test_object* obj = (test_object*) use.ptr;
    if (use.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
    {
        obj->state = &state->destroy_state;
        obj->id = object_id;
        obj->cached_value = 0;
        if (state->ptr_count < 8)
            state->ptrs[state->ptr_count++] = use.ptr;
    }
    return use.ptr;
}

static void
keyed_visit_end(substrate_test_ctx* t)
{
    alia_substrate_end_keyed_block(&t->ctx);
}

static void
run_keyed_pass(
    substrate_test_ctx* t,
    alia_substrate_key_table* table,
    keyed_visit_state* state,
    uint64_t const* keys,
    size_t key_count)
{
    alia_substrate_key_scope* scope
        = alia_substrate_begin_key_scope(&t->ctx, table);
    for (size_t i = 0; i < key_count; ++i)
    {
        keyed_visit_object(t, scope, keys[i], state, (int) keys[i]);
        keyed_visit_end(t);
    }
    alia_substrate_end_key_scope(&t->ctx, scope);
}

static void
test_keyed_block_basic(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    keyed_visit_state state;
    memset(&state, 0, sizeof(state));

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    alia_substrate_key_table* table = alia_substrate_use_key_table(
        &t.ctx, ALIA_SUBSTRATE_KEY_TABLE_NORMAL);
    TEST_ASSERT(table != NULL);

    uint64_t keys[] = {1u, 2u};
    run_keyed_pass(&t, table, &state, keys, 2);
    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    alia_substrate_key_scope* scope
        = alia_substrate_begin_key_scope(&t.ctx, table);
    void* p1 = keyed_visit_object(&t, scope, 1u, &state, 1);
    keyed_visit_end(&t);
    void* p2 = keyed_visit_object(&t, scope, 2u, &state, 2);
    keyed_visit_end(&t);
    alia_substrate_end_key_scope(&t.ctx, scope);
    (void) alia_substrate_end_block(&t.ctx);

    TEST_CHECK(p1 == state.ptrs[0]);
    TEST_CHECK(p2 == state.ptrs[1]);
    TEST_CHECK(state.destroy_state.count == 0);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_keyed_block_reorder(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    keyed_visit_state state;
    memset(&state, 0, sizeof(state));

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    alia_substrate_key_table* table = alia_substrate_use_key_table(
        &t.ctx, ALIA_SUBSTRATE_KEY_TABLE_NORMAL);
    uint64_t pass1[] = {2u, 1u};
    run_keyed_pass(&t, table, &state, pass1, 2);
    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    uint64_t pass2[] = {1u, 2u};
    run_keyed_pass(&t, table, &state, pass2, 2);
    (void) alia_substrate_end_block(&t.ctx);

    TEST_CHECK(state.ptr_count == 2);
    TEST_CHECK(state.destroy_state.count == 0);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_keyed_block_sweep_stale(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    keyed_visit_state state;
    memset(&state, 0, sizeof(state));

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    alia_substrate_key_table* table = alia_substrate_use_key_table(
        &t.ctx, ALIA_SUBSTRATE_KEY_TABLE_NORMAL);
    uint64_t pass1[] = {1u, 2u, 3u};
    run_keyed_pass(&t, table, &state, pass1, 3);
    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    uint64_t pass2[] = {1u, 2u};
    run_keyed_pass(&t, table, &state, pass2, 2);
    alia_substrate_sweep_table_keys(&t.ctx, table);
    (void) alia_substrate_end_block(&t.ctx);

    TEST_CHECK(state.destroy_state.count == 1);
    TEST_CHECK(state.destroy_state.ids[0] == 3);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_keyed_scope_requires_explicit_delete(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    keyed_visit_state state;
    memset(&state, 0, sizeof(state));

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    alia_substrate_key_table* table = alia_substrate_use_key_table(
        &t.ctx, ALIA_SUBSTRATE_KEY_TABLE_REQUIRES_EXPLICIT_DELETE);
    uint64_t pass1[] = {1u, 2u, 3u};
    run_keyed_pass(&t, table, &state, pass1, 3);
    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    uint64_t pass2[] = {1u, 2u};
    run_keyed_pass(&t, table, &state, pass2, 2);
    alia_substrate_sweep_table_keys(&t.ctx, table);
    TEST_CHECK(state.destroy_state.count == 0);

    alia_substrate_delete_key(table, alia_id_view_make_u64(3u));
    (void) alia_substrate_end_block(&t.ctx);
    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    uint64_t pass3[] = {1u, 2u};
    run_keyed_pass(&t, table, &state, pass3, 2);
    alia_substrate_sweep_table_keys(&t.ctx, table);
    (void) alia_substrate_end_block(&t.ctx);
    TEST_CHECK(state.destroy_state.count == 1);
    TEST_CHECK(state.destroy_state.ids[0] == 3);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_keyed_block_soft_delete_while_active(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    keyed_visit_state state;
    memset(&state, 0, sizeof(state));

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    alia_substrate_key_table* table = alia_substrate_use_key_table(
        &t.ctx, ALIA_SUBSTRATE_KEY_TABLE_NORMAL);
    alia_substrate_key_scope* scope
        = alia_substrate_begin_key_scope(&t.ctx, table);
    void* ptr = keyed_visit_object(&t, scope, 5u, &state, 5);
    alia_substrate_delete_key(table, alia_id_view_make_u64(5u));
    void* ptr2 = keyed_visit_object(&t, scope, 5u, &state, 5);
    keyed_visit_end(&t);
    alia_substrate_end_key_scope(&t.ctx, scope);
    (void) alia_substrate_end_block(&t.ctx);

    TEST_CHECK(ptr == ptr2);
    TEST_CHECK(state.destroy_state.count == 0);

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    scope = alia_substrate_begin_key_scope(&t.ctx, table);
    alia_substrate_sweep_table_keys(&t.ctx, table);
    alia_substrate_end_key_scope(&t.ctx, scope);
    (void) alia_substrate_end_block(&t.ctx);

    TEST_CHECK(state.destroy_state.count == 1);
    TEST_CHECK(state.destroy_state.ids[0] == 5);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_keyed_block_nested_scopes(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    keyed_visit_state outer_state;
    keyed_visit_state inner_state;
    memset(&outer_state, 0, sizeof(outer_state));
    memset(&inner_state, 0, sizeof(inner_state));

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    alia_substrate_key_table* outer_table = alia_substrate_use_key_table(
        &t.ctx, ALIA_SUBSTRATE_KEY_TABLE_NORMAL);
    alia_substrate_key_scope* outer
        = alia_substrate_begin_key_scope(&t.ctx, outer_table);
    alia_substrate_begin_keyed_block(
        &t.ctx, outer, alia_id_view_make_u64(1u), &keyed_block_spec);
    alia_substrate_key_table* inner_table = alia_substrate_use_key_table(
        &t.ctx, ALIA_SUBSTRATE_KEY_TABLE_NORMAL);
    alia_substrate_key_scope* inner
        = alia_substrate_begin_key_scope(&t.ctx, inner_table);
    void* inner_ptr = keyed_visit_object(&t, inner, 1u, &inner_state, 101);
    keyed_visit_end(&t);
    alia_substrate_end_key_scope(&t.ctx, inner);
    alia_substrate_usage_result outer_use = alia_substrate_use_memory(
        &t.ctx, sizeof(test_object), _Alignof(test_object));
    TEST_ASSERT(outer_use.ptr != NULL);
    void* outer_ptr = outer_use.ptr;
    test_object* outer_obj = (test_object*) outer_use.ptr;
    if (outer_use.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
    {
        outer_obj->state = &outer_state.destroy_state;
        outer_obj->id = 1;
        outer_state.ptrs[outer_state.ptr_count++] = outer_use.ptr;
    }
    alia_substrate_end_keyed_block(&t.ctx);
    alia_substrate_end_key_scope(&t.ctx, outer);
    (void) alia_substrate_end_block(&t.ctx);

    TEST_CHECK(outer_ptr != inner_ptr);
    TEST_CHECK(outer_state.ptr_count == 1);
    TEST_CHECK(inner_state.ptr_count == 1);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_keyed_block_partial_pass(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    keyed_visit_state state;
    memset(&state, 0, sizeof(state));

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    alia_substrate_key_table* table = alia_substrate_use_key_table(
        &t.ctx, ALIA_SUBSTRATE_KEY_TABLE_NORMAL);
    uint64_t pass1[] = {1u, 2u, 3u};
    run_keyed_pass(&t, table, &state, pass1, 3);
    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_reset_traversal(t.fixture, false);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    uint64_t partial[] = {1u, 2u};
    run_keyed_pass(&t, table, &state, partial, 2);
    alia_substrate_sweep_table_keys(&t.ctx, table);
    (void) alia_substrate_end_block(&t.ctx);
    TEST_CHECK(state.destroy_state.count == 0);

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    run_keyed_pass(&t, table, &state, partial, 2);
    alia_substrate_sweep_table_keys(&t.ctx, table);
    (void) alia_substrate_end_block(&t.ctx);
    TEST_CHECK(state.destroy_state.count == 1);
    TEST_CHECK(state.destroy_state.ids[0] == 3);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_keyed_block_partial_preserves_prediction(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    keyed_visit_state state;
    memset(&state, 0, sizeof(state));

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    alia_substrate_key_table* table = alia_substrate_use_key_table(
        &t.ctx, ALIA_SUBSTRATE_KEY_TABLE_NORMAL);
    uint64_t pass1[] = {1u, 2u, 3u};
    run_keyed_pass(&t, table, &state, pass1, 3);
    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_reset_traversal(t.fixture, false);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    uint64_t scrambled[] = {3u, 1u, 2u};
    run_keyed_pass(&t, table, &state, scrambled, 3);
    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    run_keyed_pass(&t, table, &state, pass1, 3);
    (void) alia_substrate_end_block(&t.ctx);

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    uint64_t reordered[] = {3u, 1u, 2u};
    run_keyed_pass(&t, table, &state, reordered, 3);
    (void) alia_substrate_end_block(&t.ctx);

    TEST_CHECK(state.ptr_count == 3);
    TEST_CHECK(state.destroy_state.count == 0);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_key_table_cache_clear_on_deactivate(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    alia_substrate_system* system
        = alia_test_substrate_fixture_system(t.fixture);
    keyed_visit_state state;
    memset(&state, 0, sizeof(state));

    alia_substrate_anchor* child = NULL;
    alia_substrate_key_table* table = NULL;
    void* keyed_ptrs[2];

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    child = alia_substrate_use_anchor(&t.ctx);
    TEST_ASSERT(child != NULL);
    alia_substrate_begin_block(&t.ctx, child, &root_block_spec);

    table = alia_substrate_use_key_table(
        &t.ctx, ALIA_SUBSTRATE_KEY_TABLE_NORMAL);
    TEST_ASSERT(table != NULL);

    alia_substrate_key_scope* scope
        = alia_substrate_begin_key_scope(&t.ctx, table);

    keyed_ptrs[0] = keyed_visit_object(&t, scope, 1u, &state, 1);
    ((test_object*) keyed_ptrs[0])->cached_value = 42;
    keyed_visit_end(&t);

    keyed_ptrs[1] = keyed_visit_object(&t, scope, 2u, &state, 2);
    ((test_object*) keyed_ptrs[1])->cached_value = 99;
    keyed_visit_end(&t);

    alia_substrate_end_key_scope(&t.ctx, scope);
    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    TEST_CHECK(state.destroy_state.clear_cache_count == 0);
    TEST_CHECK(state.destroy_state.count == 0);

    alia_substrate_deactivate_anchor(system, child);

    TEST_CHECK(state.destroy_state.clear_cache_count == 2);
    TEST_CHECK(
        (state.destroy_state.clear_cache_ids[0] == 1
         && state.destroy_state.clear_cache_ids[1] == 2)
        || (state.destroy_state.clear_cache_ids[0] == 2
            && state.destroy_state.clear_cache_ids[1] == 1));
    TEST_CHECK(state.destroy_state.count == 0);
    TEST_CHECK(((test_object*) keyed_ptrs[0])->cached_value == 0);
    TEST_CHECK(((test_object*) keyed_ptrs[1])->cached_value == 0);

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);
    alia_substrate_begin_block(&t.ctx, child, &root_block_spec);

    scope = alia_substrate_begin_key_scope(&t.ctx, table);

    alia_substrate_begin_keyed_block(
        &t.ctx, scope, alia_id_view_make_u64(1u), &keyed_block_spec);
    alia_substrate_usage_result reuse1 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_cleanup);
    TEST_ASSERT(reuse1.ptr != NULL);
    TEST_CHECK(reuse1.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL);
    TEST_CHECK(reuse1.ptr == keyed_ptrs[0]);
    alia_substrate_end_keyed_block(&t.ctx);

    alia_substrate_begin_keyed_block(
        &t.ctx, scope, alia_id_view_make_u64(2u), &keyed_block_spec);
    alia_substrate_usage_result reuse2 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_cleanup);
    TEST_ASSERT(reuse2.ptr != NULL);
    TEST_CHECK(reuse2.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL);
    TEST_CHECK(reuse2.ptr == keyed_ptrs[1]);
    alia_substrate_end_keyed_block(&t.ctx);

    alia_substrate_end_key_scope(&t.ctx, scope);
    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    TEST_CHECK(state.destroy_state.clear_cache_count == 2);
    TEST_CHECK(state.destroy_state.count == 0);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

static void
test_key_table_cache_clear_preserves_sibling_block(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_anchor* root
        = alia_test_substrate_fixture_root_anchor(t.fixture);
    alia_substrate_system* system
        = alia_test_substrate_fixture_system(t.fixture);
    alia_struct_spec sibling_spec = {.size = 1024u, .align = 16u};

    test_state sibling_state;
    memset(&sibling_state, 0, sizeof(sibling_state));
    keyed_visit_state keyed_state;
    memset(&keyed_state, 0, sizeof(keyed_state));

    alia_substrate_anchor* keyed_child = NULL;
    alia_substrate_anchor* sibling_child = NULL;
    void* sibling_obj_ptr = NULL;
    void* keyed_ptr = NULL;

    alia_test_substrate_fixture_advance_frame(t.fixture);
    alia_test_substrate_fixture_reset_traversal(t.fixture, true);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &root_block_spec);

    keyed_child = alia_substrate_use_anchor(&t.ctx);
    TEST_ASSERT(keyed_child != NULL);
    alia_substrate_begin_block(&t.ctx, keyed_child, &root_block_spec);
    alia_substrate_key_table* table = alia_substrate_use_key_table(
        &t.ctx, ALIA_SUBSTRATE_KEY_TABLE_NORMAL);
    alia_substrate_key_scope* scope
        = alia_substrate_begin_key_scope(&t.ctx, table);
    keyed_ptr = keyed_visit_object(&t, scope, 1u, &keyed_state, 1);
    ((test_object*) keyed_ptr)->cached_value = 42;
    keyed_visit_end(&t);
    alia_substrate_end_key_scope(&t.ctx, scope);
    (void) alia_substrate_end_block(&t.ctx);

    sibling_child = alia_substrate_use_anchor(&t.ctx);
    TEST_ASSERT(sibling_child != NULL);
    alia_substrate_begin_block(&t.ctx, sibling_child, &sibling_spec);
    alia_substrate_usage_result sibling_use = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_cleanup);
    TEST_ASSERT(sibling_use.ptr != NULL);
    sibling_obj_ptr = sibling_use.ptr;
    ((test_object*) sibling_use.ptr)->state = &sibling_state;
    ((test_object*) sibling_use.ptr)->id = 10;
    ((test_object*) sibling_use.ptr)->cached_value = 77;
    (void) alia_substrate_end_block(&t.ctx);
    (void) alia_substrate_end_block(&t.ctx);

    alia_substrate_deactivate_anchor(system, keyed_child);

    TEST_CHECK(keyed_state.destroy_state.clear_cache_count == 1);
    TEST_CHECK(keyed_state.destroy_state.clear_cache_ids[0] == 1);
    TEST_CHECK(keyed_state.destroy_state.count == 0);
    TEST_CHECK(((test_object*) keyed_ptr)->cached_value == 0);

    TEST_CHECK(sibling_state.clear_cache_count == 0);
    TEST_CHECK(sibling_state.count == 0);
    TEST_CHECK(((test_object*) sibling_obj_ptr)->cached_value == 77);

    alia_test_substrate_fixture_cleanup_root_block(t.fixture);
    substrate_test_ctx_destroy(&t);
}

void
substrate_tests(void)
{
    test_basic_block();
    test_basic_block_discovery();
    test_substrate_use_anchor();
    test_substrate_destructors();
    test_substrate_deactivate_anchor();
    test_substrate_path_for_object_flat();
    test_substrate_path_for_object_nested();
    test_substrate_path_for_object_stable_normal_pass();
    test_substrate_path_for_object_discovery();
    test_substrate_path_capture_smoke();
    test_substrate_generations();
    test_keyed_block_basic();
    test_keyed_block_reorder();
    test_keyed_block_sweep_stale();
    test_keyed_scope_requires_explicit_delete();
    test_keyed_block_soft_delete_while_active();
    test_keyed_block_nested_scopes();
    test_keyed_block_partial_pass();
    test_keyed_block_partial_preserves_prediction();
    test_key_table_cache_clear_on_deactivate();
    test_key_table_cache_clear_preserves_sibling_block();
}
