#ifndef ALIA_PLATFORMS_GLFW_BORDERLESS_FULLSCREEN_H
#define ALIA_PLATFORMS_GLFW_BORDERLESS_FULLSCREEN_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/host_window.h>

ALIA_EXTERN_C_BEGIN

typedef struct GLFWwindow GLFWwindow;

// Saved windowed rect and mode for borderless fullscreen toggle.
// Zero-initialize (e.g. `static` storage or `{0}`) before use.
typedef struct alia_glfw_borderless_fullscreen_state
{
    bool is_fullscreen;
    alia_vec2i windowed_position;
    alia_vec2i windowed_size;
} alia_glfw_borderless_fullscreen_state;

typedef struct alia_glfw_fullscreen_host_binding
{
    GLFWwindow* window;
    alia_glfw_borderless_fullscreen_state* state;
} alia_glfw_fullscreen_host_binding;

ALIA_API void
alia_glfw_fullscreen_host_toggle(void* user);

static inline alia_host_window_ops
alia_glfw_make_fullscreen_host_ops(
    alia_glfw_fullscreen_host_binding* binding)
{
    alia_host_window_ops ops;
    ops.toggle_fullscreen = alia_glfw_fullscreen_host_toggle;
    ops.user = binding;
    return ops;
}

ALIA_EXTERN_C_END

#endif /* ALIA_PLATFORMS_GLFW_BORDERLESS_FULLSCREEN_H */
