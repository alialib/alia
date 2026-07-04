#ifndef ALIA_PLATFORMS_GLFW_UI_BINDING_H
#define ALIA_PLATFORMS_GLFW_UI_BINDING_H

#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;
typedef struct alia_glfw_host alia_glfw_host;

// POD for GLFW `glfwGetWindowUserPointer` when using the stock Alia GLFW
// callback installers in `input_glue.h`. The installers set
// `glfwSetWindowUserPointer(window, binding)` for you; both input and surface
// installers expect the same `alia_glfw_ui_binding*` so framebuffer and input
// callbacks share one user pointer.
typedef struct alia_glfw_ui_binding
{
    alia_ui_system* ui;
    alia_glfw_host* host;
} alia_glfw_ui_binding;

ALIA_EXTERN_C_END

#endif /* ALIA_PLATFORMS_GLFW_UI_BINDING_H */
