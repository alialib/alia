#ifndef ALIA_ABI_UI_INPUT_ELEMENTS_H
#define ALIA_ABI_UI_INPUT_ELEMENTS_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/kernel/routing.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/constants.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/state.h>

ALIA_EXTERN_C_BEGIN

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

alia_interaction_status_t
alia_element_get_interaction_status(
    alia_context* ctx,
    alia_element_id id,
    alia_interaction_status_t overrides);

alia_element_id
alia_element_get_identity(alia_context* ctx);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_INPUT_ELEMENTS_H */
