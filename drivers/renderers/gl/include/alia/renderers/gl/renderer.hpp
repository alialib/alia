#pragma once

// TODO: Refactor this.
#ifdef __EMSCRIPTEN__
#define GLFW_INCLUDE_ES3
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <emscripten.h>
#else
#include <glad/glad.h>

#include <GLFW/glfw3.h>
#endif

#include <alia/abi/base/arena.h>
#include <alia/abi/ui/drawing.h>

#include <alia/base/arena.h>

namespace alia {

struct gl_renderer
{
    alia_ui_system* system;
    GLuint vanilla_shader_program;
    GLuint vao, vbo;
    GLuint instance_vbo;
    GLint vanilla_matrix_location;
    alia_arena rect_instance_arena;
};

struct display_list;

// Create a new OpenGL renderer.
// (This may compile shaders, etc.)
void
init_gl_renderer(alia_ui_system* system, gl_renderer* renderer);

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
