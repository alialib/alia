#pragma once

// TODO: Remove this.
#include <glad/glad.h>

// TODO: Remove this.
#include <alia/drawing.hpp>

#include <alia/internals/arena.hpp>

// TODO: Use forward declarations once those are sorted out.
#include <alia/system/object.hpp>

namespace alia {

struct display_list;

// TODO: Make this opaque.
struct gl_renderer
{
    alia_draw_system* system;
    GLuint vanilla_shader_program;
    GLuint vao, vbo;
    GLuint instance_vbo;
    GLint vanilla_matrix_location;
    alia_arena rect_instance_arena;
    GLuint clip_ubo;
    float* clip_ptr;
};

// Create a new OpenGL renderer.
// (This may compile shaders, etc.)
void
init_gl_renderer(alia_draw_system* system, gl_renderer* renderer);

// Destroy a renderer.
void
destroy_gl_renderer(gl_renderer* renderer);

void
render_box_command_list(void* user, alia_draw_bucket const* bucket);

// TODO: Move to a separate utility header.
GLuint
create_shader_program(
    const char* vertex_shader_source, const char* fragment_shader_source);

} // namespace alia
