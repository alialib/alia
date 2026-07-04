#ifndef ALIA_HOST_HOST_H
#define ALIA_HOST_HOST_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/api.h>
#include <alia/host/frame.h>
#include <alia/host/window_state.h>

#include <stdbool.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_host alia_host;

typedef struct alia_host_window_options
{
    bool resizable;
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

// Open the platform surface (GLFW window or WebGL canvas). Returns false on
// failure. On desktop, also initializes the GLFW platform if needed.
bool
alia_host_open(alia_host* host, alia_host_open_config const* config);

// Update the UI surface size and DPI from the host surface.
void
alia_host_sync_surface(alia_host* host, alia_ui_system* ui);

// Install callbacks (if not already) and run until exit.
void
alia_host_run(alia_host* host, alia_host_run_config const* config);

ALIA_EXTERN_C_END

#endif /* ALIA_HOST_HOST_H */
