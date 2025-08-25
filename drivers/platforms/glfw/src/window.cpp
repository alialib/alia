#include <alia/platforms/glfw/window.hpp>

#include <alia/ui/system.hpp>

namespace alia {

void
update_glfw_window_info(System& system, GLFWwindow* window)
{
    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    system.framebuffer_size
        = Vec2{float(framebuffer_width), float(framebuffer_height)};

    // TODO: Sort out scaling...

    // int window_width, window_height;
    // glfwGetWindowSize(window, &window_width, &window_height);

    // float xscale, yscale;
    // glfwGetWindowContentScale(window, &xscale, &yscale);
    // auto const ppi = Vec2{xscale * 96.f, yscale * 96.f};
}

} // namespace alia
