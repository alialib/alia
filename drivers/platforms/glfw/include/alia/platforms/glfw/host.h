#ifndef ALIA_PLATFORMS_GLFW_HOST_H
#define ALIA_PLATFORMS_GLFW_HOST_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/api.h>
#include <alia/host/frame.h>
#include <alia/host/window_state.h>

#include <stdbool.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;
typedef struct GLFWwindow GLFWwindow;

typedef struct alia_glfw_window_options
{
    bool resizable;
    // When false, the continuous run loop paces frames to the monitor refresh
    // rate instead of using glfwSwapInterval.
    bool vsync;
    // Windows prototype: present via Vulkan swapchain after GL renders to an
    // off-screen framebuffer.
    bool vulkan_present;
} alia_glfw_window_options;

// Opaque host object — the app owns instances (stack, heap, or file-scope).
typedef struct alia_glfw_host alia_glfw_host;

typedef struct alia_glfw_host_config
{
    alia_ui_system* ui;
    alia_host_frame_handler frame;
    // When false, the loop waits on `glfwWaitEventsTimeout` until input, a due
    // timer, or `alia_ui_needs_tick` says to wake. When true, polls every
    // iteration (for continuous animation).
    bool continuous;
    alia_window_state_handler on_window_state_changed;
} alia_glfw_host_config;

// `glfwInit` and stock OpenGL 3.3 core window hints. Safe to call once per
// process before opening a window through the host.
bool
alia_glfw_platform_init(void);

void
alia_glfw_platform_shutdown(void);

alia_glfw_host*
alia_glfw_host_create(void);

void
alia_glfw_host_destroy(alia_glfw_host* host);

// Create a window, make its context current, load GLAD, enable sRGB, and apply
// all fields of `state` (including borderless fullscreen). Seeds the host's
// canonical window state. `options` may be null (resizable and vsync default to
// true). Returns false on failure.
bool
alia_glfw_host_open(
    alia_glfw_host* host,
    char const* title,
    alia_window_state state,
    alia_glfw_window_options const* options);

// Use an existing `GLFWwindow*` with the host. Window state is read from GLFW
// on first install. Does not load GLAD or change the current context.
void
alia_glfw_host_attach_window(alia_glfw_host* host, GLFWwindow* window);

// Update the UI surface's size and DPI from the host window.
void
alia_glfw_host_sync_surface(alia_glfw_host* host, alia_ui_system* ui);

// Register GLFW callbacks on the host window.
void
alia_glfw_host_install(
    alia_glfw_host* host, alia_glfw_host_config const* config);

// Install callbacks (if not already) and run until the window is closed.
void
alia_glfw_host_run(alia_glfw_host* host, alia_glfw_host_config const* config);

void
alia_glfw_host_toggle_fullscreen(alia_glfw_host* host);

ALIA_EXTERN_C_END

#endif /* ALIA_PLATFORMS_GLFW_HOST_H */
