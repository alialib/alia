#include <alia/abi/base/object.h>
#include <alia/abi/kernel/animation.h>
#include <alia/abi/ui/system/api.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

#define TEST_NO_MAIN
#include "acutest.h"

static int
near(float a, float b)
{
    return fabsf(a - b) < 0.001f;
}

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

static void
noop_controller(void* user_data, alia_ui_context* ctx)
{
    (void) user_data;
    (void) ctx;
}

typedef struct animation_rig
{
    void* storage;
    alia_ui_system* system;
    alia_ui_context ctx;
    uint32_t bits;
} animation_rig;

static void
rig_init(animation_rig* rig)
{
    memset(rig, 0, sizeof(*rig));
    alia_struct_spec spec = alia_ui_system_object_spec();
    rig->storage = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(rig->storage != NULL);
    alia_ui_controller controller;
    controller.fn = noop_controller;
    controller.user_data = NULL;
    rig->system = alia_ui_system_init(
        rig->storage, controller, alia_vec2i_make(64, 64));
    TEST_ASSERT(rig->system != NULL);
    rig->ctx.system = rig->system;
    // The kernel object is the first member of `alia_ui_system`.
    rig->ctx.kernel.object = (alia_kernel*) rig->system;
}

static void
rig_destroy(animation_rig* rig)
{
    aligned_free_portable(rig->storage);
}

static alia_bitref
rig_bit(animation_rig* rig)
{
    alia_bitref bit;
    bit.storage = &rig->bits;
    bit.offset = 0;
    return bit;
}

static void
check_flares(
    animation_rig* rig,
    alia_nanosecond_count tick,
    alia_nanosecond_count const* expected,
    unsigned expected_count)
{
    rig->ctx.kernel.tick_count = tick;
    alia_nanosecond_count got[8];
    memset(got, 0, sizeof(got));
    unsigned count = alia_animation_process_flares(
        alia_ctx_kernel(&rig->ctx), rig_bit(rig), got);
    TEST_CHECK(count == expected_count);
    unsigned i;
    for (i = 0; i < count && i < expected_count; ++i)
        TEST_CHECK(got[i] == expected[i]);
}

static void
test_flares(void)
{
    animation_rig rig;
    rig_init(&rig);

    check_flares(&rig, alia_milliseconds(0), NULL, 0);

    rig.ctx.kernel.tick_count = alia_milliseconds(0);
    alia_animation_fire_flare(
        alia_ctx_kernel(&rig.ctx), rig_bit(&rig), alia_milliseconds(1000));

    {
        alia_nanosecond_count expected[] = {alia_milliseconds(900)};
        check_flares(&rig, alia_milliseconds(100), expected, 1);
    }

    alia_animation_fire_flare(
        alia_ctx_kernel(&rig.ctx), rig_bit(&rig), alia_milliseconds(1000));

    {
        alia_nanosecond_count expected[]
            = {alia_milliseconds(800), alia_milliseconds(900)};
        check_flares(&rig, alia_milliseconds(200), expected, 2);
    }
    {
        alia_nanosecond_count expected[]
            = {alia_milliseconds(500), alia_milliseconds(600)};
        check_flares(&rig, alia_milliseconds(500), expected, 2);
    }
    {
        alia_nanosecond_count expected[] = {alia_milliseconds(50)};
        check_flares(&rig, alia_milliseconds(1050), expected, 1);
    }
    check_flares(&rig, alia_milliseconds(1200), NULL, 0);

    rig_destroy(&rig);
}

/* `true` maps to 0 and `false` maps to 1: the value starts at the true end
 * and animates toward the false end. */
static float
transition_at(animation_rig* rig, alia_nanosecond_count tick, bool state)
{
    rig->ctx.kernel.tick_count = tick;
    alia_animated_transition transition;
    transition.curve = alia_linear_curve;
    transition.duration = alia_milliseconds(1000);
    return alia_transition_float(
        alia_ctx_kernel(&rig->ctx),
        &transition,
        rig_bit(rig),
        state,
        0.f,
        1.f);
}

static void
test_transition(void)
{
    animation_rig rig;
    rig_init(&rig);

    TEST_CHECK(near(transition_at(&rig, alia_milliseconds(0), true), 0.f));
    TEST_CHECK(near(transition_at(&rig, alia_milliseconds(0), false), 0.f));
    TEST_CHECK(near(transition_at(&rig, alia_milliseconds(400), false), 0.4f));
    TEST_CHECK(near(transition_at(&rig, alia_milliseconds(800), false), 0.8f));
    TEST_CHECK(near(transition_at(&rig, alia_milliseconds(1200), false), 1.f));

    rig_destroy(&rig);
}

static void
test_transition_reversal(void)
{
    animation_rig rig;
    rig_init(&rig);

    TEST_CHECK(near(transition_at(&rig, alia_milliseconds(0), true), 0.f));
    TEST_CHECK(near(transition_at(&rig, alia_milliseconds(0), false), 0.f));
    TEST_CHECK(near(transition_at(&rig, alia_milliseconds(400), false), 0.4f));
    TEST_CHECK(near(transition_at(&rig, alia_milliseconds(400), true), 0.4f));
    TEST_CHECK(near(transition_at(&rig, alia_milliseconds(600), true), 0.2f));
    TEST_CHECK(near(transition_at(&rig, alia_milliseconds(1000), true), 0.f));

    rig_destroy(&rig);
}

void
animation_tests(void)
{
    test_flares();
    test_transition();
    test_transition_reversal();
}
