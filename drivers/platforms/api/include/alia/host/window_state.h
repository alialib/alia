#ifndef ALIA_HOST_WINDOW_STATE_H
#define ALIA_HOST_WINDOW_STATE_H

#include <alia/abi/prelude.h>

#include <stdbool.h>

ALIA_EXTERN_C_BEGIN

// Desktop window placement and presentation mode. `x`, `y`, `width`, and
// `height` are always the windowed (restored) rect; when `fullscreen` is true,
// the monitor rect is implicit.
typedef struct alia_window_state
{
    int x;
    int y;
    int width;
    int height;
    // If `has_position` is false, the platform may choose the placement for
    // the window (e.g. centered).
    bool has_position;
    bool maximized;
    bool fullscreen;
} alia_window_state;

typedef void (*alia_window_state_changed_fn)(
    void* user_data, alia_window_state const* state);

typedef struct alia_window_state_handler
{
    alia_window_state_changed_fn fn;
    void* user_data;
} alia_window_state_handler;

static inline alia_window_state
alia_window_state_make(int width, int height)
{
    alia_window_state state;
    state.x = 0;
    state.y = 0;
    state.width = width;
    state.height = height;
    state.has_position = false;
    state.maximized = false;
    state.fullscreen = false;
    return state;
}

ALIA_EXTERN_C_END

#endif /* ALIA_HOST_WINDOW_STATE_H */
