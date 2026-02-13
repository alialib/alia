#ifndef ALIA_ABI_UI_ELEMENTS_H
#define ALIA_ABI_UI_ELEMENTS_H

#include <alia/abi/context.h>
#include <alia/abi/ids.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/api.h>

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

// Detect if the element with the given ID is under the pointer.
static inline bool
alia_element_is_hovered(alia_context* ctx, alia_element_id id)
{
    return ctx->input->hot_element.element == id;
}

static inline bool
alia_no_element_has_capture(alia_context* ctx)
{
    return !alia_element_id_is_valid(ctx->input->element_with_capture.element);
}

// Detect if the given ID has captured the mouse. - Capture is implicitly
// acquired when a mouse button is pressed down while the mouse is over an
// element with the given ID, and it stays acquired as long as the mouse button
// is held down.
static inline bool
alia_element_has_capture(alia_context* ctx, alia_element_id id)
{
    return ctx->input->element_with_capture.element == id;
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_ELEMENTS_H */
