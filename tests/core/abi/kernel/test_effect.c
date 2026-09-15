#include <alia/abi/base/arena.h>
#include <alia/abi/context.h>
#include <alia/abi/kernel/effect.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

// Acutest main is provided by `tests/core/abi/runner.c`.
#define TEST_NO_MAIN
#include "acutest.h"

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

typedef struct effect_rig
{
    void* storage;
    void* buffer;
    alia_arena* arena;
    alia_bump_allocator alloc;
    alia_effect_log effects;
    alia_context ctx;
} effect_rig;

static void
effect_rig_init(effect_rig* rig)
{
    memset(rig, 0, sizeof(*rig));
    alia_struct_spec spec = alia_arena_object_spec();
    rig->storage = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(rig->storage != NULL);
    rig->buffer = aligned_alloc_portable(ALIA_MAX_ALIGN, 4096);
    TEST_ASSERT(rig->buffer != NULL);
    rig->arena = alia_arena_init(
        rig->storage, rig->buffer, 4096, alia_arena_no_controller());
    TEST_ASSERT(rig->arena != NULL);
    alia_bump_allocator_init(&rig->alloc, rig->arena);
    rig->ctx.scratch = &rig->alloc;
    rig->ctx.effects = &rig->effects;
}

static void
effect_rig_destroy(effect_rig* rig)
{
    alia_arena_destroy(rig->arena);
    aligned_free_portable(rig->storage);
    aligned_free_portable(rig->buffer);
}

static int g_custom_runs;
static int g_custom_seen;

typedef struct increment_effect
{
    alia_effect base;
    int* counter;
} increment_effect;

static void
increment_effect_run(alia_effect* self)
{
    increment_effect* inc = (increment_effect*) self;
    g_custom_seen = *inc->counter;
    *inc->counter += 1;
    g_custom_runs += 1;
}

static void
test_write_applies_after_run(void)
{
    effect_rig rig;
    effect_rig_init(&rig);

    int dst = 0;
    int const src = 42;
    alia_defer_write(&rig.ctx, &dst, &src, sizeof(src), "int");
    TEST_CHECK(dst == 0);
    TEST_CHECK(rig.ctx.effects->head != NULL);
    TEST_CHECK(rig.ctx.effects->head->label != NULL);
    TEST_CHECK(strcmp(rig.ctx.effects->head->label, "int") == 0);

    alia_run_effects(&rig.ctx);
    TEST_CHECK(dst == 42);
    TEST_CHECK(rig.ctx.effects->head == NULL);
    TEST_CHECK(rig.ctx.effects->tail == NULL);

    effect_rig_destroy(&rig);
}

static void
test_fifo_order(void)
{
    effect_rig rig;
    effect_rig_init(&rig);

    int value = 10;
    int const first = 1;
    alia_defer_write(&rig.ctx, &value, &first, sizeof(first), "first");

    increment_effect* inc = (increment_effect*) alia_arena_ptr(
        &rig.alloc,
        alia_arena_alloc(
            &rig.alloc, ALIA_MIN_ALIGNED_SIZE(sizeof(increment_effect))));
    inc->base.run = increment_effect_run;
    inc->base.label = "inc";
    inc->counter = &value;
    alia_defer_effect(&rig.ctx, &inc->base);

    int const third = 99;
    alia_defer_write(&rig.ctx, &value, &third, sizeof(third), "third");

    TEST_CHECK(value == 10);
    g_custom_runs = 0;
    g_custom_seen = 0;
    alia_run_effects(&rig.ctx);
    TEST_CHECK(g_custom_runs == 1);
    TEST_CHECK(g_custom_seen == 1);
    TEST_CHECK(value == 99);

    effect_rig_destroy(&rig);
}

static void
test_empty_run_is_noop(void)
{
    effect_rig rig;
    effect_rig_init(&rig);
    alia_run_effects(&rig.ctx);
    TEST_CHECK(rig.ctx.effects->head == NULL);
    effect_rig_destroy(&rig);
}

void
effect_tests(void)
{
    test_write_applies_after_run();
    test_fifo_order();
    test_empty_run_is_noop();
}
