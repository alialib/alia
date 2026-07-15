#ifndef ALIA_HOST_HOST_H
#define ALIA_HOST_HOST_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/api.h>
#include <alia/host/frame.h>
#include <alia/host/window_state.h>

#include <stdbool.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_host alia_host;

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
typedef struct alia_win32_host alia_win32_host;
#endif

// Product hosts: Win32+DXGI on Windows, web canvas on Emscripten.
// GLFW is not a product host; see alia_glfw (input/surface glue) and the
// glfw_gl_embed example for embedding.

typedef struct alia_host_window_options
{
    bool resizable;
    // When false, the continuous run loop paces frames to the monitor refresh
    // rate instead of using vsync.
    bool vsync;
} alia_host_window_options;

typedef struct alia_host_open_config
{
    // Desktop: window title. Ignored on web.
    char const* title;
    // Desktop: initial/restored window state. Ignored on web.
    alia_window_state window_state;
    // Desktop: optional window hints. Ignored on web.
    alia_host_window_options const* window_options;
    // Web: CSS selector for the canvas element (e.g. "#canvas").
    // Ignored on desktop.
    char const* canvas_selector;
} alia_host_open_config;

typedef struct alia_host_run_config
{
    alia_ui_system* ui;
    alia_host_frame_handler frame;
    // When false, the loop waits until input, a due timer, or dirty UI.
    // When true, polls every iteration (for continuous animation).
    bool continuous;
    // Desktop: window state persistence callback. Ignored on web.
    alia_window_state_handler on_window_state_changed;
} alia_host_run_config;

alia_host*
alia_host_create(void);

void
alia_host_destroy(alia_host* host);

// Open the platform surface. Returns false on failure.
bool
alia_host_open(alia_host* host, alia_host_open_config const* config);

// Update the UI surface size and DPI from the host surface.
void
alia_host_sync_surface(alia_host* host, alia_ui_system* ui);

// Install callbacks (if not already) and enter the host loop.
// Desktop: blocks until the window closes.
// Web: schedules requestAnimationFrame and returns immediately. Callers must
// keep UI/host storage alive (e.g. static or heap) and must not destroy it.
void
alia_host_run(alia_host* host, alia_host_run_config const* config);

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
alia_win32_host*
alia_host_as_win32(alia_host* host);
#endif

ALIA_EXTERN_C_END

#endif /* ALIA_HOST_HOST_H */
