#ifndef ALIA_SHELL_GL_APP_H
#define ALIA_SHELL_GL_APP_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/api.h>
#include <alia/host/frame.h>
#include <alia/host/host.h>
#include <alia/host/window_state.h>
#include <alia/renderers/gl/renderer.h>
#include <alia/shell/gl/shell.h>

#include <stdbool.h>

ALIA_EXTERN_C_BEGIN

// TODO: Make this opaque.
typedef struct alia_gl_app
{
    alia_gl_shell* shell;
    alia_ui_system* ui;
    void* ui_storage;
    alia_gl_renderer* renderer;
    void* renderer_storage;
    alia_host* host;
} alia_gl_app;

// TODO: Revisit this struct.
typedef struct alia_gl_app_config
{
    alia_ui_controller inner;
    alia_gl_shell_config shell;
    alia_host_frame_handler frame;
    bool continuous;
    alia_window_state_handler on_window_state_changed;

    char const* title;
    alia_window_state window_state;
    alia_host_window_options const* window_options;
    char const* canvas_selector;
} alia_gl_app_config;

// Bootstrap shell, UI, platform host, renderer, and initial layout refresh.
// On failure, call `alia_gl_app_destroy` before returning. Returns 0 on
// success.
int
alia_gl_app_init(alia_gl_app_config const* config, alia_gl_app* app);

// Run the platform host loop. Requires a successful `alia_gl_app_init`.
// Desktop: blocks until exit, then callers may destroy.
// Web: schedules RAF and returns; keep `app` alive (static/heap) and do not
// call `alia_gl_app_destroy` afterward.
void
alia_gl_app_run_loop(alia_gl_app_config const* config, alia_gl_app* app);

void
alia_gl_app_destroy(alia_gl_app* app);

// Convenience: init + run_loop. Destroys only on desktop (where run_loop
// returns after the window closes).
int
alia_gl_app_run(alia_gl_app_config const* config, alia_gl_app* app);

ALIA_EXTERN_C_END

#endif /* ALIA_SHELL_GL_APP_H */
