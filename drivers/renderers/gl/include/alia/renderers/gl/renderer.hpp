#pragma once

// TODO: Remove this.
#include <glad/glad.h>

// TODO: Remove this.
#include <alia/ui/drawing.hpp>

namespace alia {

struct infinite_arena;
struct system;
struct display_list;

// TODO: Make this opaque.
struct gl_renderer
{
    GLuint vanilla_shader_program;
    GLuint vao, vbo;
    GLuint instance_vbo;
    GLint vanilla_matrix_location;
    infinite_arena* rect_instance_arena;
    GLuint clip_ubo;
    float* clip_ptr;
};

// Create a new OpenGL renderer.
// (This may compile shaders, etc.)
void
init_gl_renderer(gl_renderer* renderer);

// Destroy a renderer.
void
destroy_gl_renderer(gl_renderer* renderer);

void
render_box_command_list(
    gl_renderer* renderer,
    system const& system,
    box_command_list const& boxes);

// TODO: Move to a separate utility header.
GLuint
create_shader_program(
    const char* vertex_shader_source, const char* fragment_shader_source);

} // namespace alia
