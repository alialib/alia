#pragma once

extern "C" {
typedef struct alia_ui_system alia_ui_system;
}

// POD for GLFW `glfwGetWindowUserPointer` when using the stock Alia GLFW
// callback installers in `input_glue.hpp`. The installers set
// `glfwSetWindowUserPointer(window, binding)` for you; both input and surface
// installers expect the same `alia_glfw_ui_binding*` so framebuffer and input
// callbacks share one user pointer.
typedef struct alia_glfw_ui_binding
{
    alia_ui_system* ui;
} alia_glfw_ui_binding;
