// Smoke test: layout ABI headers must compile as C.
#include <alia/abi/ui/layout/components.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/layout/protocol.h>
#include <alia/abi/ui/layout/system.h>
#include <alia/abi/ui/layout/utilities/placement.h>

#define TEST_NO_MAIN
#include "acutest.h"

void
layout_abi_headers_tests(void)
{
    TEST_CHECK(sizeof(alia_layout_node) > 0);
    TEST_CHECK(sizeof(alia_flow_fragment) > 0);
}
