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

typedef enum alia_host_backend
{
    // Desktop: Win32+DXGI on Windows, GLFW elsewhere. Web: always web.
    ALIA_HOST_BACKEND_AUTO = 0,
    ALIA_HOST_BACKEND_GLFW = 1,
    ALIA_HOST_BACKEND_WIN32 = 2,
} alia_host_backend;

typedef struct alia_host_window_options
{
    bool resizable;
    // When false, the continuous run loop paces frames to the monitor refresh
    // rate instead of using vsync.
    bool vsync;
    // Windows only, requires ALIA_ENABLE_VK_PRESENT at build time: present via
    // Vulkan swapchain after GL renders to an off-screen framebuffer.
    // Ignored for the Win32 backend.
    bool vulkan_present;
} alia_host_window_options;

typedef struct alia_host_open_config
{
    // Desktop: window title. Ignored on web.
    char const* title;
    // Desktop: initial/restored window state. Ignored on web.
    alia_window_state window_state;
    // Desktop: optional window hints. Ignored on web.
    alia_host_window_options const* window_options;
    // Desktop: which platform backend to open. AUTO selects Win32 on Windows
    // and GLFW elsewhere. GL shells should request GLFW explicitly.
    alia_host_backend backend;
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

// Install callbacks (if not already) and run until exit.
void
alia_host_run(alia_host* host, alia_host_run_config const* config);

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
// Non-null when the host was opened with the Win32 backend.
alia_win32_host*
alia_host_as_win32(alia_host* host);
#endif

ALIA_EXTERN_C_END

#endif /* ALIA_HOST_HOST_H */
