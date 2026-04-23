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
insets_tests(void);
void
vec2_tests(void);

// kernel

void
substrate_tests(void);
void
ids_tests(void);

TEST_LIST
    = {{"base/geometry/affine2", affine2_tests},
       {"base/geometry/box", box_tests},
       {"base/geometry/insets", insets_tests},
       {"base/geometry/vec2", vec2_tests},
       {"base/arena", arena_tests},
       {"base/color", color_tests},
       {"base/stack", stack_tests},
       {"kernel/ids", ids_tests},
       {"kernel/substrate", substrate_tests},
       {NULL, NULL}};
