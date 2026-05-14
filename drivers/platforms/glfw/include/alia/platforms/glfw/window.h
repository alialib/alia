#ifndef ALIA_PLATFORMS_GLFW_WINDOW_H
#define ALIA_PLATFORMS_GLFW_WINDOW_H

#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;
typedef struct GLFWwindow GLFWwindow;

// Polls GLFW for framebuffer size and updates `alia_ui_surface_set_size`.
// If you use `alia_glfw_install_surface_callbacks`, you typically do not need
// to call this every frame (the callbacks already keep the surface size in
// sync).
ALIA_API void
alia_glfw_update_window_info(alia_ui_system* ui, GLFWwindow* window);

ALIA_EXTERN_C_END

#endif /* ALIA_PLATFORMS_GLFW_WINDOW_H */
