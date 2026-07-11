#ifndef ALIA_PLATFORMS_WIN32_HOST_H
#define ALIA_PLATFORMS_WIN32_HOST_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/api.h>
#include <alia/host/frame.h>
#include <alia/host/window_state.h>

#include <stdbool.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;

typedef unsigned int UINT;

ALIA_EXTERN_C_BEGIN

typedef struct alia_win32_host alia_win32_host;

typedef struct alia_win32_host_open_config
{
    char const* title;
    alia_window_state window_state;
    bool resizable;
    // When true, present waits on DXGI's frame-latency waitable object and
    // uses sync interval 1. When false, presents immediately (no wait).
    bool vsync;
} alia_win32_host_open_config;

typedef struct alia_win32_host_run_config
{
    // Optional for continuous probe apps; required when continuous is false.
    alia_ui_system* ui;
    // Called once per frame after the present clock is acquired and before
    // Present. The host has already bound the backbuffer RTV and set the
    // viewport.
    alia_host_frame_handler frame;
    // When false, the loop waits until input, a due timer, or dirty UI.
    bool continuous;
    // When true, the host clears with a slowly pulsing color each frame
    // (useful for present-timing probes).
    bool probe_clear;
    alia_window_state_handler on_window_state_changed;
} alia_win32_host_run_config;

alia_win32_host*
alia_win32_host_create(void);

void
alia_win32_host_destroy(alia_win32_host* host);

bool
alia_win32_host_open(
    alia_win32_host* host, alia_win32_host_open_config const* config);

// Install UI input (if `ui` is set), wire host window ops, and run until the
// window is closed.
void
alia_win32_host_run(
    alia_win32_host* host, alia_win32_host_run_config const* config);

void
alia_win32_host_toggle_fullscreen(alia_win32_host* host);

// Bind a UI system so mouse/keyboard/wheel messages are enqueued into it.
// Prefer passing `ui` via `alia_win32_host_run`; this remains for smoke tests.
void
alia_win32_host_install(alia_win32_host* host, alia_ui_system* ui);

void
alia_win32_host_sync_surface(alia_win32_host* host, alia_ui_system* ui);

ID3D11Device*
alia_win32_host_device(alia_win32_host* host);

ID3D11DeviceContext*
alia_win32_host_context(alia_win32_host* host);

ID3D11RenderTargetView*
alia_win32_host_rtv(alia_win32_host* host);

UINT
alia_win32_host_width(alia_win32_host* host);

UINT
alia_win32_host_height(alia_win32_host* host);

ALIA_EXTERN_C_END

#endif /* ALIA_PLATFORMS_WIN32_HOST_H */
