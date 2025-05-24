#pragma once

// TODO: Remove this.
#include <glad/glad.h>

namespace alia {

struct Arena;
struct System;
struct DisplayList;

// TODO: Make this opaque.
struct GlRenderer
{
    GLuint vanilla_shader_program;
    GLuint vao, vbo;
    GLuint instance_vbo;
    GLint vanilla_matrix_location;
    Arena* rect_instance_arena;
};

// Create a new OpenGL renderer.
// (This may compile shaders, etc.)
void
init_gl_renderer(GlRenderer* renderer);

// Destroy a renderer.
void
destroy_gl_renderer(GlRenderer* renderer);

// Submit the display list and render to framebuffer
void
render_display_list(
    GlRenderer* renderer,
    System const& system,
    DisplayList const& display_list);

// TODO: Move to a separate utility header.
GLuint
create_shader_program(
    const char* vertex_shader_source, const char* fragment_shader_source);

} // namespace alia
