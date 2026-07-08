#ifndef ALIA_PLATFORMS_GLFW_VK_PRESENT_H
#define ALIA_PLATFORMS_GLFW_VK_PRESENT_H

#include <alia/abi/prelude.h>

#include <glad/glad.h>

#include <stdbool.h>

struct GLFWwindow;

ALIA_EXTERN_C_BEGIN

typedef struct alia_glfw_vk_present alia_glfw_vk_present;

typedef struct alia_glfw_vk_gl_target
{
    GLuint framebuffer;
    int width;
    int height;
} alia_glfw_vk_gl_target;

bool
alia_glfw_vk_present_begin(
    alia_glfw_vk_present** out_present,
    GLFWwindow* window,
    bool vsync);

bool
alia_glfw_vk_present_complete(alia_glfw_vk_present* present);

bool
alia_glfw_vk_present_create(
    alia_glfw_vk_present** out_present,
    GLFWwindow* window,
    bool vsync);

void
alia_glfw_vk_present_destroy(alia_glfw_vk_present* present);

bool
alia_glfw_vk_present_resize(
    alia_glfw_vk_present* present, int width, int height);

bool
alia_glfw_vk_present_uses_gpu_interop(alia_glfw_vk_present* present);

// Returns the shared GL framebuffer used for off-screen rendering.
bool
alia_glfw_vk_present_get_gl_target(
    alia_glfw_vk_present* present,
    int width,
    int height,
    alia_glfw_vk_gl_target* out_target);

// Present the current GL target through Vulkan (GPU interop or readback).
bool
alia_glfw_vk_present_from_gl_target(alia_glfw_vk_present* present);

// When vsync is enabled, temporarily use a low-latency present mode during
// Win32 modal window moves/resizes (FIFO is restored when modal is false).
bool
alia_glfw_vk_present_set_modal_interaction(
    alia_glfw_vk_present* present, bool modal);

ALIA_EXTERN_C_END

#endif /* ALIA_PLATFORMS_GLFW_VK_PRESENT_H */
