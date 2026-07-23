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
    // Legacy hand-written GLSL: frame uniforms instead of a frame UBO.
    GLint loc_region = -1;
    GLint loc_surface = -1;
    GLuint frame_block_index = GL_INVALID_INDEX;
    GLuint params_block_index = GL_INVALID_INDEX;
    size_t params_size = 0;
    size_t ubo_bytes = 0;
};

struct gl_linear_target
{
    GLuint fbo = 0;
    GLuint color_tex = 0;
    int width = 0;
    int height = 0;
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
    // Shared AliaEffectFrame UBO (binding 0) for Slang-baked effects.
    GLuint effect_frame_ubo = 0;
    std::vector<std::unique_ptr<gl_effect_slot>> effects;
    // Linear-blend primary surface - Shaders output linear premultiplied color
    // into RGBA8. At draw_pass_end a present pass encodes linear->sRGB into
    // the host framebuffer (WebGL has no sRGB backbuffer like D3D's RTV).
    gl_linear_target primary_target{};
    GLuint present_program = 0;
    GLint present_sampler_location = -1;
    // draw-framebuffer binding captured at draw_pass_begin (usually 0)
    GLint pass_draw_fbo = 0;
};
