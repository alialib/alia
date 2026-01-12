#pragma once

#include <GLFW/glfw3.h>

// TODO: Use forward declarations once those are sorted out.
#include <alia/system/object.hpp>

namespace alia {

void
update_glfw_window_info(ui_system& system, GLFWwindow* window);

} // namespace alia
