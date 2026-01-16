#include "acutest.h"

// geometry
void
vec2_tests(void);
void
box_tests(void);

TEST_LIST = {{"vec2", vec2_tests}, {"box", box_tests}, {NULL, NULL}};
