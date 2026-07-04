#pragma once

#include <alia/abi/ui/system/api.h>

#include <alia/base/arena.h>

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

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
};
