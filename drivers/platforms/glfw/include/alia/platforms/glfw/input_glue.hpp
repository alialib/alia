#pragma once

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/input/constants.h>
#include <alia/platforms/glfw/ui_binding.hpp>

struct GLFWwindow;

// Map GLFW modifier bitmask to Alia kmods.
alia_kmods_t
alia_glfw_mods_to_kmods(int glfw_mods);

// Map window-coordinate cursor position to framebuffer pixels (HiDPI).
alia_vec2f
alia_glfw_cursor_window_to_framebuffer(GLFWwindow* window, double x, double y);

// Enqueue input using the same rules as the stock Alia GLFW examples.
void
alia_glfw_enqueue_mouse_button(
    alia_ui_system* ui,
    GLFWwindow* window,
    int button,
    int action,
    int mods);

void
alia_glfw_enqueue_cursor_pos(
    alia_ui_system* ui,
    GLFWwindow* window,
    double x,
    double y);

void
alia_glfw_enqueue_scroll(alia_ui_system* ui, double x, double y);

void
alia_glfw_enqueue_key(alia_ui_system* ui, int key, int action, int mods);

// Registers GLFW callbacks that forward to the enqueue helpers above.
// Sets `glfwSetWindowUserPointer(window, binding)`; use the same binding with
// `alia_glfw_install_surface_callbacks`.
void
alia_glfw_install_default_input_callbacks(
    GLFWwindow* window, alia_glfw_ui_binding* binding);

// Registers framebuffer and content-scale callbacks that only update
// `alia_ui_surface_set_size` / `alia_ui_surface_set_dpi` (never run a full
// UI frame from inside GLFW — safe during nested GLFW/OS callbacks).
// Uses the same `binding` user pointer as the input installer.
void
alia_glfw_install_surface_callbacks(
    GLFWwindow* window, alia_glfw_ui_binding* binding);
