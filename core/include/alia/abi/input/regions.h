#pragma once

#include <alia/abi/geometry.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t alia_hit_test_flags_t;

#define ALIA_HIT_TEST_FLAGS(X)                                                \
    X(0x1, MOUSE)                                                             \
    X(0x2, WHEEL)

#define ALIA_DEFINE_HIT_TEST_FLAG(code, name) ALIA_HIT_TEST_##name = (code),
enum
{
    ALIA_HIT_TEST_FLAGS(ALIA_DEFINE_HIT_TEST_FLAG)
};
#undef ALIA_DEFINE_HIT_TEST_FLAG

#ifdef __cplusplus
}
#endif
