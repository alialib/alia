#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t alia_interaction_status_t;

#define ALIA_INTERACTION_STATUS_LIST(X)                                       \
    X(0x1, DISABLED)                                                          \
    X(0x2, HOVERED)                                                           \
    X(0x4, ACTIVE)                                                            \
    X(0x8, FOCUSED)

#define X(code, NAME) ALIA_INTERACTION_STATUS_##NAME = (code),
enum
{
    ALIA_INTERACTION_STATUS_LIST(X)
};
#undef X

#ifdef __cplusplus
}
#endif
