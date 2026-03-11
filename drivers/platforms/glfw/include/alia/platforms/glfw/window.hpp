#pragma once

#include <GLFW/glfw3.h>

extern "C" {

// TODO: This isn't technically correct.
typedef struct alia_ui_system alia_ui_system;

} // extern "C"

namespace alia {

void
update_glfw_window_info(alia_ui_system* ui, GLFWwindow* window);

} // namespace alia
