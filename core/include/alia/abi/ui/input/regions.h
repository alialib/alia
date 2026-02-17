#ifndef ALIA_ABI_UI_INPUT_REGIONS_H
#define ALIA_ABI_UI_INPUT_REGIONS_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/ids.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/constants.h>

ALIA_EXTERN_C_BEGIN

// Handle all region-related events for a rectangular region.
void
alia_element_box_region(
    alia_context* ctx,
    alia_element_id id,
    alia_box const* region,
    alia_cursor_t cursor);

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

// Hit test a rectangular region.
void
alia_element_hit_test_box_region(
    alia_context* ctx,
    alia_element_id id,
    alia_box const* box,
    alia_hit_test_flags_t flags,
    alia_cursor_t cursor);

// Manually report a mouse hit. This is used by components that want to define
// custom shapes and manually handle hit test events.
void
alia_element_report_mouse_hit(
    alia_context* ctx,
    alia_element_id id,
    alia_box const* bounding_box,
    alia_hit_test_flags_t flags,
    alia_cursor_t cursor);

// Respond to `make_widget_visible` events for a given region.
void
alia_element_handle_visibility(
    alia_context* ctx, alia_element_id id, alia_box const* region);

// Override the cursor for a given element. This can be called after hit
// testing to override the cursor associated with an element.
void
alia_element_override_cursor(
    alia_context* ctx, alia_element_id id, alia_cursor_t cursor);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_INPUT_REGIONS_H */
