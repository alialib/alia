#pragma once

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/system/host_window.h>

struct GLFWwindow;

// Saved windowed rect and mode for borderless fullscreen toggle.
struct glfw_borderless_fullscreen_state
{
    bool is_fullscreen = false;
    alia_vec2i windowed_position{};
    alia_vec2i windowed_size{};
};

struct glfw_fullscreen_host_binding
{
    GLFWwindow* window;
    glfw_borderless_fullscreen_state* state;
};

void
glfw_fullscreen_host_toggle(void* user);

static inline alia_host_window_ops
glfw_make_fullscreen_host_ops(glfw_fullscreen_host_binding* binding)
{
    return alia_host_window_ops{
        .toggle_fullscreen = glfw_fullscreen_host_toggle,
        .user = binding,
    };
}
