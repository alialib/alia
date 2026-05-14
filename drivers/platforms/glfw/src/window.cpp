#include <alia/platforms/glfw/window.h>

#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/api.h>

#include <GLFW/glfw3.h>

extern "C" {

void
alia_glfw_update_window_info(alia_ui_system* ui, GLFWwindow* window)
{
    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    alia_vec2i const size
        = ALIA_BRACED_INIT(alia_vec2i, framebuffer_width, framebuffer_height);
    alia_ui_surface_set_size(ui, size);

    // TODO: Sort out scaling...

    // int window_width, window_height;
    // glfwGetWindowSize(window, &window_width, &window_height);

    // float xscale, yscale;
    // glfwGetWindowContentScale(window, &xscale, &yscale);
    // auto const dpi = Vec2{xscale * 96.f, yscale * 96.f};
}

} // extern "C"
