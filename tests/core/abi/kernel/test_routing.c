#include <alia/abi/kernel/routing.h>
#include <alia/abi/kernel/substrate.h>

#include <stdint.h>

// Acutest main is provided by `tests/core/abi/runner.c`.
#define TEST_NO_MAIN
#include "acutest.h"

static void
test_substrate_id_sub_packing(void)
{
    int storage = 0;
    alia_substrate_usage_result use = {
        .ptr = &storage,
        .generation = ALIA_SUBSTRATE_ID_GENERATION_STEP * 3u,
        .mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL,
    };

    alia_substrate_id base = alia_make_substrate_id(use);
    TEST_CHECK(alia_substrate_id_is_valid(base));
    TEST_CHECK(base.ptr == &storage);
    TEST_CHECK(base.stamp == use.generation);
    TEST_CHECK(alia_substrate_id_sub(base) == 0);
    TEST_CHECK(alia_substrate_id_generation(base) == use.generation);

    alia_substrate_id a = alia_substrate_sub_id(base, 1);
    alia_substrate_id b = alia_substrate_sub_id(base, 2);
    TEST_CHECK(a.ptr == base.ptr);
    TEST_CHECK(b.ptr == base.ptr);
    TEST_CHECK(alia_substrate_id_sub(a) == 1);
    TEST_CHECK(alia_substrate_id_sub(b) == 2);
    TEST_CHECK(alia_substrate_id_generation(a) == use.generation);
    TEST_CHECK(alia_substrate_id_generation(b) == use.generation);
    TEST_CHECK(!alia_substrate_id_equal(a, b));
    TEST_CHECK(!alia_substrate_id_equal(a, base));
}

static void
test_element_sub_id_preserves_route(void)
{
    int storage = 0;
    alia_substrate_usage_result use = {
        .ptr = &storage,
        .generation = 0,
        .mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL,
    };

    alia_element_id base
        = alia_element_from_substrate_id(alia_make_substrate_id(use), 42u);
    TEST_CHECK(base.route == 42u);
    TEST_CHECK(alia_element_id_sub(base) == 0);

    alia_element_id thumb = alia_element_sub_id(base, 1);
    TEST_CHECK(thumb.ptr == base.ptr);
    TEST_CHECK(thumb.route == 42u);
    TEST_CHECK(alia_element_id_sub(thumb) == 1);
    TEST_CHECK(alia_substrate_id_generation(alia_element_substrate_id(thumb))
               == 0);
    TEST_CHECK(!alia_element_id_equal(thumb, base));
}

void
routing_tests(void)
{
    test_substrate_id_sub_packing();
    test_element_sub_id_preserves_route();
}
