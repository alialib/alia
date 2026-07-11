#ifndef ALIA_SHELL_D3D11_APP_H
#define ALIA_SHELL_D3D11_APP_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/api.h>
#include <alia/host/frame.h>
#include <alia/host/host.h>
#include <alia/host/window_state.h>
#include <alia/renderers/d3d11/renderer.h>
#include <alia/shell/d3d11/shell.h>

#include <stdbool.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_d3d11_app
{
    alia_d3d11_shell* shell;
    alia_ui_system* ui;
    void* ui_storage;
    alia_d3d11_renderer* renderer;
    void* renderer_storage;
    alia_host* host;
} alia_d3d11_app;

typedef struct alia_d3d11_app_config
{
    alia_ui_controller inner;
    alia_d3d11_shell_config shell;
    alia_host_frame_handler frame;
    bool continuous;
    alia_window_state_handler on_window_state_changed;

    char const* title;
    alia_window_state window_state;
    alia_host_window_options const* window_options;
} alia_d3d11_app_config;

int
alia_d3d11_app_init(alia_d3d11_app_config const* config, alia_d3d11_app* app);

void
alia_d3d11_app_run_loop(
    alia_d3d11_app_config const* config, alia_d3d11_app* app);

void
alia_d3d11_app_destroy(alia_d3d11_app* app);

int
alia_d3d11_app_run(alia_d3d11_app_config const* config, alia_d3d11_app* app);

ALIA_EXTERN_C_END

#endif /* ALIA_SHELL_D3D11_APP_H */
