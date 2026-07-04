#ifndef ALIA_ABI_UI_INPUT_TOUCH_GESTURE_H
#define ALIA_ABI_UI_INPUT_TOUCH_GESTURE_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/kernel/routing.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// How a single-finger touch sequence should be interpreted by platform hosts.
// Wheel/trackpad scrolling is separate and does not use this resolution.
typedef enum alia_touch_gesture_kind
{
    // No gesture chosen yet (e.g. before hit testing).
    ALIA_TOUCH_GESTURE_NONE = 0,
    // Route motion/press/release as pointer input to `target`.
    ALIA_TOUCH_GESTURE_POINTER,
    // Route finger pan deltas as scroll input to `target`.
    ALIA_TOUCH_GESTURE_PAN_SCROLL,
} alia_touch_gesture_kind;

typedef struct alia_ui_touch_gesture_resolution
{
    alia_touch_gesture_kind kind;
    alia_element_id target;
} alia_ui_touch_gesture_resolution;

// Run `ALIA_EVENT_TOUCH_GESTURE_HIT_TEST` at `position` and pick a gesture.
// Resolves layout from the current surface size before hit testing. Drag
// handles opt in via `ALIA_HIT_TEST_TOUCH_DRAG` on
// `alia_element_box_region` with `ALIA_HIT_TEST_TOUCH_DRAG`.
alia_ui_touch_gesture_resolution
alia_ui_resolve_touch_gesture(alia_ui_system* ui, alia_vec2f position);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_INPUT_TOUCH_GESTURE_H */
