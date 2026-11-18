#include <alia/abi/kernel/instance_ids.h>
#include <alia/abi/kernel/routing.h>

#include <stdint.h>

// Acutest main is provided by `tests/core/abi/runner.c`.
#define TEST_NO_MAIN
#include "acutest.h"

static void
test_element_id_from_instance(void)
{
    alia_instance_id const instance = ALIA_BRACED_INIT(alia_instance_id, 9u);

    alia_element_id base = alia_element_id_make(instance, 42u);
    TEST_CHECK(base.route == 42u);
    TEST_CHECK(alia_instance_id_equal(
        base.id, ALIA_BRACED_INIT(alia_instance_id, 9u)));

    alia_element_id other = alia_element_id_make(instance, 7u);
    TEST_CHECK(!alia_element_id_equal(other, base));
    other.route = 42u;
    TEST_CHECK(alia_element_id_equal(other, base));
}

void
routing_tests(void)
{
    test_element_id_from_instance();
}
