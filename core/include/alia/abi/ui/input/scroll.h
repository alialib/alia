#ifndef ALIA_ABI_UI_INPUT_SCROLL_H
#define ALIA_ABI_UI_INPUT_SCROLL_H

#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

// Scroll deltas passed to `alia_ui_enqueue_scroll` and delivered as
// `alia_scroll_input.delta` use logical/surface pixels of scroll motion:
//   - positive `y` increases vertical scroll offset (wheel down / content up)
//   - positive `x` increases horizontal scroll offset (wheel right)
//
// Platform hosts convert raw wheel/trackpad input into these units before
// enqueueing. Single-finger touch pan (mobile) should use the same deltas
// once a host resolves `ALIA_TOUCH_GESTURE_PAN_SCROLL` via
// `alia_ui_resolve_touch_gesture`. Widgets apply deltas directly (optionally
// scaled by style).

// approximate logical pixels for one mouse wheel notch (used by GLFW host)
#define ALIA_SCROLL_PIXELS_PER_WHEEL_NOTCH 120.f

// approximate logical pixels per line when normalizing DOM_DELTA_LINE wheel
// events (Firefox and similar)
#define ALIA_SCROLL_PIXELS_PER_LINE 40.f

// fallback page step when normalizing DOM_DELTA_PAGE wheel events
#define ALIA_SCROLL_PIXELS_PER_PAGE 600.f

// DOM WheelEvent.deltaMode values
#define ALIA_DOM_DELTA_PIXEL 0u
#define ALIA_DOM_DELTA_LINE 1u
#define ALIA_DOM_DELTA_PAGE 2u

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_INPUT_SCROLL_H */
