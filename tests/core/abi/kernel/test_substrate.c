#include "substrate_harness.h"

#include <alia/abi/base/stack.h>
#include <alia/abi/context.h>
#include <alia/abi/kernel/substrate.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Acutest main is provided by `tests/core/abi/runner.c`.
#define TEST_NO_MAIN
#include "acutest.h"

// --------- substrate allocator (backed by malloc/free) ---------

static void*
test_block_alloc(void* user_data, size_t size, size_t alignment)
{
    (void) user_data;
    (void) alignment;
    return malloc(size);
}

static void
test_block_free(void* user_data, void* ptr)
{
    (void) user_data;
    free(ptr);
}

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

// --------- destructor ordering test support ---------

typedef struct test_state
{
    int ids[6];
    int count;
} test_state;

typedef struct test_object
{
    test_state* state;
    int id;
} test_object;

static void
test_object_destructor(void* p)
{
    test_object* obj = (test_object*) p;
    obj->state->ids[obj->state->count++] = obj->id;
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

    alia_substrate_allocator allocator = {
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
        alia_stack_cleanup(t->stack);

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

    alia_substrate_block* root
        = alia_test_substrate_fixture_root_block(t.fixture);
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    alia_test_substrate_fixture_reset_traversal(t.fixture);
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

    alia_substrate_block* root
        = alia_test_substrate_fixture_root_block(t.fixture);

    alia_test_substrate_fixture_reset_traversal(t.fixture);
    alia_stack_reset(t.stack);

    // Enter discovery mode by passing align=0.
    alia_struct_spec discovery_spec = {.size = 0u, .align = 0u};
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
    TEST_CHECK(computed.size == 768u);
    TEST_CHECK(computed.align == 16u);

    // Now allocate/init using computed spec.
    alia_test_substrate_fixture_reset_traversal(t.fixture);
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
test_substrate_use_block(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_block* root
        = alia_test_substrate_fixture_root_block(t.fixture);
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    alia_test_substrate_fixture_reset_traversal(t.fixture);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &spec);

    alia_substrate_block* child = alia_substrate_use_block(&t.ctx);
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

    alia_substrate_block* root
        = alia_test_substrate_fixture_root_block(t.fixture);
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    test_state state;
    memset(&state, 0, sizeof(state));
    state.count = 0;

    alia_test_substrate_fixture_reset_traversal(t.fixture);
    alia_stack_reset(t.stack);

    alia_substrate_begin_block(&t.ctx, root, &spec);

    // Root objects 1 and 2.
    alia_substrate_usage_result r1 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_destructor);
    TEST_ASSERT(r1.ptr != NULL);
    TEST_CHECK(r1.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);
    ((test_object*) r1.ptr)->state = &state;
    ((test_object*) r1.ptr)->id = 1;

    alia_substrate_usage_result r2 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_destructor);
    TEST_ASSERT(r2.ptr != NULL);
    TEST_CHECK(r2.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);
    ((test_object*) r2.ptr)->state = &state;
    ((test_object*) r2.ptr)->id = 2;

    // Child block.
    alia_substrate_block* child = alia_substrate_use_block(&t.ctx);
    TEST_ASSERT(child != NULL);

    alia_substrate_begin_block(&t.ctx, child, &spec);

    alia_substrate_usage_result c1 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_destructor);
    TEST_ASSERT(c1.ptr != NULL);
    TEST_CHECK(c1.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);
    ((test_object*) c1.ptr)->state = &state;
    ((test_object*) c1.ptr)->id = 3;

    alia_substrate_usage_result c2 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_destructor);
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
        test_object_destructor);
    TEST_ASSERT(r3.ptr != NULL);
    TEST_CHECK(r3.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT);
    ((test_object*) r3.ptr)->state = &state;
    ((test_object*) r3.ptr)->id = 5;

    alia_substrate_usage_result r4 = alia_substrate_use_object(
        &t.ctx,
        sizeof(test_object),
        _Alignof(test_object),
        test_object_destructor);
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
test_substrate_generations(void)
{
    substrate_test_ctx t;
    substrate_test_ctx_init(&t);

    alia_substrate_block* root
        = alia_test_substrate_fixture_root_block(t.fixture);
    alia_struct_spec spec = {.size = 1024u, .align = 16u};

    for (int generation = 0; generation < 2; ++generation)
    {
        alia_test_substrate_fixture_reset_traversal(t.fixture);
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

void
substrate_tests(void)
{
    test_basic_block();
    test_basic_block_discovery();
    test_substrate_use_block();
    test_substrate_destructors();
    test_substrate_generations();
}
