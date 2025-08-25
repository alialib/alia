#pragma once

#include <GLFW/glfw3.h>

namespace alia {

struct System;

void
update_glfw_window_info(System& system, GLFWwindow* window);

} // namespace alia
