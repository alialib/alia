// Smoke test: layout ABI headers must compile as C.
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/layout/detached.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/layout/metrics.h>
#include <alia/abi/ui/layout/placement.h>
#include <alia/abi/ui/layout/popup.h>
#include <alia/abi/ui/layout/protocol.h>
#include <alia/abi/ui/layout/system.h>
#include <alia/abi/ui/layout/utilities/emission.h>
#include <alia/abi/ui/layout/utilities/placement.h>

#define TEST_NO_MAIN
#include "acutest.h"

void
layout_abi_headers_tests(void)
{
    TEST_CHECK(sizeof(alia_layout_node) > 0);
    TEST_CHECK(sizeof(alia_flow_fragment) > 0);
}
