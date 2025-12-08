#include <alia/platforms/glfw/window.hpp>

#include <alia/system/object.hpp>

namespace alia {

void
update_glfw_window_info(ui_system& system, GLFWwindow* window)
{
    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    system.surface_size
        = vec2{float(framebuffer_width), float(framebuffer_height)};

    // TODO: Sort out scaling...

    // int window_width, window_height;
    // glfwGetWindowSize(window, &window_width, &window_height);

    // float xscale, yscale;
    // glfwGetWindowContentScale(window, &xscale, &yscale);
    // auto const ppi = Vec2{xscale * 96.f, yscale * 96.f};
}

} // namespace alia
