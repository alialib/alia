#pragma once

#include <alia/abi/ui/system/api.h>

#include <alia/base/arena.h>

#include <memory>
#include <vector>

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

struct alia_gl_renderer;

struct gl_effect_slot
{
    alia_gl_renderer* renderer = nullptr;
    GLuint program = 0;
    GLuint params_ubo = 0;
    GLint loc_region = -1;
    GLint loc_surface = -1;
    size_t params_size = 0;
    size_t ubo_bytes = 0;
};

struct alia_gl_renderer
{
    alia_ui_system* system = nullptr;
    GLuint primitive_shader_program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint instance_vbo = 0;
    GLint primitive_matrix_location = 0;
    GLint msdf_sampler_location = 0;
    GLuint msdf_atlas_texture = 0;
    alia_arena rect_instance_arena{};
    // shared effect geometry - Each registered effect is its own material.
    GLuint effect_vao = 0;
    GLuint effect_vbo = 0;
    std::vector<std::unique_ptr<gl_effect_slot>> effects;
};
