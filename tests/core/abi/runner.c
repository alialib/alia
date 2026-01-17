#include "acutest.h"

// geometry
void
affine2_tests(void);
void
box_tests(void);
void
insets_tests(void);
void
vec2_tests(void);

TEST_LIST
    = {{"geometry/affine2", affine2_tests},
       {"geometry/box", box_tests},
       {"geometry/insets", insets_tests},
       {"geometry/vec2", vec2_tests},
       {NULL, NULL}};
