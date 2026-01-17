#include "acutest.h"

// geometry
void
vec2_tests(void);
void
box_tests(void);
void
insets_tests(void);
void
affine2_tests(void);

TEST_LIST
    = {{"vec2", vec2_tests},
       {"box", box_tests},
       {"insets", insets_tests},
       {"affine2", affine2_tests},
       {NULL, NULL}};
