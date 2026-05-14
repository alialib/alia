#pragma once

#include <GLFW/glfw3.h>

extern "C" {

typedef struct alia_ui_system alia_ui_system;

} // extern "C"

namespace alia {

// Polls GLFW for framebuffer size and updates `alia_ui_surface_set_size`.
// If you use `alia_glfw_install_surface_callbacks`, you typically do not need
// to call this every frame (the callbacks already keep the surface size in
// sync).
void
update_glfw_window_info(alia_ui_system* ui, GLFWwindow* window);

} // namespace alia
