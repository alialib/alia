#pragma once

#include <GLFW/glfw3.h>

namespace alia {

struct ui_system;

void
update_glfw_window_info(ui_system& system, GLFWwindow* window);

} // namespace alia
