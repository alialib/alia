#include <alia/abi/kernel/instance_ids.h>

#include <stdint.h>

// Acutest main is provided by `tests/core/abi/runner.c`.
#define TEST_NO_MAIN
#include "acutest.h"

static void
test_instance_id_basics(void)
{
    TEST_CHECK(!alia_instance_id_is_valid(ALIA_INSTANCE_ID_NONE));

    alia_instance_id const a = ALIA_BRACED_INIT(alia_instance_id, 3u);
    alia_instance_id const b = ALIA_BRACED_INIT(alia_instance_id, 4u);
    TEST_CHECK(alia_instance_id_is_valid(a));
    TEST_CHECK(alia_instance_id_equal(a, a));
    TEST_CHECK(!alia_instance_id_equal(a, b));
    TEST_CHECK(alia_instance_id_equal(
        alia_instance_id_at(a, 1), ALIA_BRACED_INIT(alia_instance_id, 4u)));
}

static void
test_instance_id_pool_and_at(void)
{
    alia_instance_id_pool pool;
    alia_instance_id_pool_init(&pool);

    alia_instance_id const first = alia_instance_id_alloc(&pool);
    TEST_CHECK(alia_instance_id_equal(
        first, ALIA_BRACED_INIT(alia_instance_id, 1u)));

    alia_instance_id const range = alia_instance_id_alloc_range(&pool, 3);
    TEST_CHECK(alia_instance_id_equal(
        range, ALIA_BRACED_INIT(alia_instance_id, 2u)));
    TEST_CHECK(alia_instance_id_equal(
        alia_instance_id_at(range, 1), ALIA_BRACED_INIT(alia_instance_id, 3u)));
    TEST_CHECK(alia_instance_id_equal(
        alia_instance_id_at(range, 2), ALIA_BRACED_INIT(alia_instance_id, 4u)));

    alia_instance_id const next = alia_instance_id_alloc(&pool);
    TEST_CHECK(alia_instance_id_equal(
        next, ALIA_BRACED_INIT(alia_instance_id, 5u)));
    TEST_CHECK(alia_instance_id_equal(
        pool.next, ALIA_BRACED_INIT(alia_instance_id, 6u)));

    // A zero-length carve does not advance the cursor and does not own the
    // returned value.
    TEST_CHECK(alia_instance_id_equal(
        alia_instance_id_alloc_range(&pool, 0),
        ALIA_BRACED_INIT(alia_instance_id, 6u)));
    TEST_CHECK(alia_instance_id_equal(
        pool.next, ALIA_BRACED_INIT(alia_instance_id, 6u)));
}

void
instance_ids_tests(void)
{
    test_instance_id_basics();
    test_instance_id_pool_and_at();
}
