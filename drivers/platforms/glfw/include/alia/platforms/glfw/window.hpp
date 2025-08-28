#pragma once

#include <GLFW/glfw3.h>

namespace alia {

struct system;

void
update_glfw_window_info(system& system, GLFWwindow* window);

} // namespace alia
