#include <alia/platforms/glfw/window.hpp>

#include <alia/abi/ui/system/api.h>

namespace alia {

void
update_glfw_window_info(alia_ui_system* ui, GLFWwindow* window)
{
    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    alia_ui_surface_set_size(ui, {framebuffer_width, framebuffer_height});

    // TODO: Sort out scaling...

    // int window_width, window_height;
    // glfwGetWindowSize(window, &window_width, &window_height);

    // float xscale, yscale;
    // glfwGetWindowContentScale(window, &xscale, &yscale);
    // auto const dpi = Vec2{xscale * 96.f, yscale * 96.f};
}

} // namespace alia
