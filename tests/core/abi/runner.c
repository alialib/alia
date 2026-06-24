#include "acutest.h"

// base
void
arena_tests(void);
void
color_tests(void);
void
stack_tests(void);

// base/geometry
void
affine2_tests(void);
void
box_tests(void);
void
edge_offsets_tests(void);
void
vec2_tests(void);

// kernel

void
substrate_tests(void);
void
ids_tests(void);

void
layout_abi_headers_tests(void);
void
placement_tests(void);
void
line_tests(void);

TEST_LIST
    = {{"base/geometry/affine2", affine2_tests},
       {"base/geometry/box", box_tests},
       {"base/geometry/edge_offsets", edge_offsets_tests},
       {"base/geometry/vec2", vec2_tests},
       {"base/arena", arena_tests},
       {"base/color", color_tests},
       {"base/stack", stack_tests},
       {"kernel/ids", ids_tests},
       {"kernel/substrate", substrate_tests},
       {"ui/layout/abi_headers", layout_abi_headers_tests},
       {"ui/layout/placement", placement_tests},
       {"ui/layout/line", line_tests},
       {NULL, NULL}};
