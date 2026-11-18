#include <alia/abi/kernel/instance_ids.h>
#include <alia/abi/ui/drawing/commands.h>

#include <stdint.h>

// Acutest main is provided by `tests/core/abi/runner.c`.
#define TEST_NO_MAIN
#include "acutest.h"

static void
test_draw_id_from_instance(void)
{
    alia_instance_id const base = ALIA_BRACED_INIT(alia_instance_id, 3u);

    alia_draw_id a = alia_draw_id_make(base, 1);
    alia_draw_id b = alia_draw_id_make(base, 2);
    TEST_CHECK(alia_instance_id_equal(a.id, base));
    TEST_CHECK(alia_instance_id_equal(b.id, base));
    TEST_CHECK(a.sub_id == 1);
    TEST_CHECK(b.sub_id == 2);
    TEST_CHECK(!alia_draw_id_equal(a, b));
    TEST_CHECK(!alia_draw_id_equal(a, alia_draw_id_make(base, 0)));
}

void
draw_ids_tests(void)
{
    test_draw_id_from_instance();
}
