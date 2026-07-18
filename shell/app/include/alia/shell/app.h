#ifndef ALIA_SHELL_APP_H
#define ALIA_SHELL_APP_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/msdf.h>
#include <alia/abi/ui/system/api.h>
#include <alia/host/frame.h>
#include <alia/host/host.h>
#include <alia/host/window_state.h>
#include <alia/shell/shell.h>

#include <stdbool.h>
#include <stddef.h>

ALIA_EXTERN_C_BEGIN

// Product bootstrap: host + renderer + shell + UI.
// Link `alia_shell_app` (CMake selects D3D11 or GL).

enum
{
    ALIA_APP_STORAGE_SIZE = 256
};

typedef struct alia_app
{
    alignas(16) unsigned char bytes[ALIA_APP_STORAGE_SIZE];
} alia_app;

typedef struct alia_app_config
{
    alia_ui_controller inner;
    alia_shell_config shell;
    alia_host_frame_handler frame;
    bool continuous;
    alia_window_state_handler on_window_state_changed;

    char const* title;
    alia_window_state window_state;
    alia_host_window_options const* window_options;
    // Web: CSS selector for the canvas (e.g. "#canvas"). Ignored on desktop.
    char const* canvas_selector;
} alia_app_config;

// Bootstrap shell, UI, platform host, renderer, and initial layout refresh.
// Returns 0 on success.
int
alia_app_init(alia_app_config const* config, alia_app* app);

// Desktop: blocks until the window closes.
// Web: schedules requestAnimationFrame and returns; keep `app` alive for the
// page lifetime and do not call `alia_app_destroy` afterward.
void
alia_app_run_loop(alia_app_config const* config, alia_app* app);

void
alia_app_destroy(alia_app* app);

// Convenience: init + run_loop. Destroys only on desktop.
int
alia_app_run(alia_app_config const* config, alia_app* app);

alia_ui_system*
alia_app_ui(alia_app* app);

bool
alia_app_setup_stock_text(alia_app* app);

alia_msdf_text_engine*
alia_app_text_engine(alia_app* app);

size_t
alia_app_typeface_count(alia_app* app);

alia_typeface_id
alia_app_typeface(alia_app* app, size_t index);

// Update + draw for one frame (call from the host frame handler).
void
alia_app_shell_frame(alia_ui_system* ui);

// Draw pass only.
void
alia_app_shell_draw(alia_ui_system* ui);

#if defined(ALIA_SHELL_BACKEND_D3D11)
struct alia_d3d11_renderer;
struct alia_d3d11_renderer*
alia_app_d3d11_renderer(alia_app* app);
#elif defined(ALIA_SHELL_BACKEND_GL)
struct alia_gl_renderer;
struct alia_gl_renderer*
alia_app_gl_renderer(alia_app* app);
#endif

ALIA_EXTERN_C_END

#endif /* ALIA_SHELL_APP_H */
