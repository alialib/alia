#ifndef ALIA_PLATFORMS_WEB_HOST_H
#define ALIA_PLATFORMS_WEB_HOST_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/api.h>
#include <alia/host/frame.h>

#include <stdbool.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// opaque host object - The app owns instances (stack, heap, or file-scope).
typedef struct alia_web_host alia_web_host;

typedef struct alia_web_host_config
{
    alia_ui_system* ui;
    // CSS selector for the canvas element (e.g. "#canvas").
    char const* canvas_selector;
    alia_host_frame_handler frame;
    // When false, frames run only while `alia_ui_needs_tick` is true (plus
    // after input). When true, every animation frame invokes `frame`.
    bool continuous;
} alia_web_host_config;

alia_web_host*
alia_web_host_create(void);

void
alia_web_host_destroy(alia_web_host* host);

// Create a WebGL 2 context on `canvas_selector` and make it current.
bool
alia_web_host_init_webgl(alia_web_host* host, char const* canvas_selector);

// Resize the canvas backing store from CSS layout and DPR, then update the UI
// surface size and DPI.
void
alia_web_host_sync_surface(
    alia_web_host* host, alia_ui_system* ui, char const* canvas_selector);

// Register resize and input callbacks on the canvas. Does not start the loop.
void
alia_web_host_install(alia_web_host* host, alia_web_host_config const* config);

// Schedule one animation frame if not already scheduled.
void
alia_web_host_request_frame(alia_web_host* host);

// Install callbacks (if not already) and schedule the first
// requestAnimationFrame. Returns immediately; the Emscripten runtime stays
// alive while callbacks remain registered. Callers must keep host/UI storage
// alive and must not destroy it after return.
void
alia_web_host_run(alia_web_host* host, alia_web_host_config const* config);

ALIA_EXTERN_C_END

#endif /* ALIA_PLATFORMS_WEB_HOST_H */
