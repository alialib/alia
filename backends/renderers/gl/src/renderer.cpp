#include <alia/renderers/gl/renderer.hpp>

#include <alia/foundation/arena.hpp>
#include <alia/ui/display_list.hpp>
#include <alia/ui/system.hpp>

// TODO: Remove this.
#include <iostream>

namespace alia {

// TODO: Move this to foundation.

static void*
default_alloc(void*, size_t size, size_t alignment)
{
    void* ptr = nullptr;
#ifdef _MSC_VER
    ptr = _aligned_malloc(size, alignment);
#else
    posix_memalign(&ptr, alignment, size);
#endif
    return ptr;
}

static void
default_dealloc(void*, void* ptr)
{
#ifdef _MSC_VER
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

const char* instanced_vertex_shader_source = R"(
#version 330 core
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 i_pos;
layout (location = 2) in vec2 i_size;
layout (location = 3) in vec4 i_color;

uniform mat4 u_projection;

out vec4 v_color;

void main() {
    vec2 scaled = a_pos * i_size + i_pos;
    gl_Position = u_projection * vec4(scaled, 0.0, 1.0);
    v_color = i_color;
}
)";

const char* vanilla_fragment_shader_source = R"(
#version 330 core
in vec4 v_color;
out vec4 frag_color;
void main() {
    frag_color = v_color;
}
)";

const char* msdf_vertex_shader_source = R"(
#version 330 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;

out vec2 v_uv;

uniform mat4 u_projection;

void main() {
    v_uv = a_uv;
    gl_Position = u_projection * vec4(a_position, 0.0, 1.0);
}
)";

const char* msdf_fragment_shader_source = R"(
#version 330 core

in vec2 v_uv;
out vec4 out_color;

uniform sampler2D u_msdf;
uniform vec4 u_color;

float median(vec3 v) {
    return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

void main() {
    vec3 msd = texture(u_msdf, v_uv).rgb;
    float dist = median(msd) - 0.5;
    float alpha = clamp(dist / fwidth(dist) + 0.5, 0.0, 1.0);
    out_color = vec4(u_color.rgb * alpha, u_color.a * alpha);
}
)";

struct RectInstance
{
    Vec2 pos;
    Vec2 size;
    Color color;
};

GLuint
compile_shader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        std::cerr << "Shader compilation error: " << info_log << std::endl;
    }
    return shader;
}

GLuint
create_shader_program(
    const char* vertex_shader_source, const char* fragment_shader_source)
{
    GLuint vertex_shader
        = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader
        = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return shader_program;
}

void
init_gl_renderer(GlRenderer* renderer)
{
    float quad_vertices[] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

    GLuint vanilla_shader_program = create_shader_program(
        instanced_vertex_shader_source, vanilla_fragment_shader_source);
    GLuint msdf_shader_program = create_shader_program(
        msdf_vertex_shader_source, msdf_fragment_shader_source);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    ArenaAllocator alloc{default_alloc, default_dealloc, nullptr};
    Arena* rect_instance_arena
        = create_arena(alloc, sizeof(RectInstance) * 4096); // TODO

    GLuint instance_vbo;
    glGenBuffers(1, &instance_vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    // position (location = 1)
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(RectInstance),
        (void*) offsetof(RectInstance, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    // size (location = 2)
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(RectInstance),
        (void*) offsetof(RectInstance, size));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // color (location = 3)
    glVertexAttribPointer(
        3,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(RectInstance),
        (void*) offsetof(RectInstance, color));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    GLint vanilla_matrix_location
        = glGetUniformLocation(vanilla_shader_program, "u_projection");
    GLint msdf_matrix_location
        = glGetUniformLocation(msdf_shader_program, "u_projection");
    GLint msdf_color_location
        = glGetUniformLocation(msdf_shader_program, "u_color");

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    *renderer = {
        .vanilla_shader_program = vanilla_shader_program,
        .msdf_shader_program = msdf_shader_program,
        .vao = vao,
        .vbo = vbo,
        .instance_vbo = instance_vbo,
        .vanilla_matrix_location = vanilla_matrix_location,
        .msdf_matrix_location = msdf_matrix_location,
        .msdf_color_location = msdf_color_location,
        .rect_instance_arena = rect_instance_arena,
    };
}

void
destroy_gl_renderer(GlRenderer* renderer)
{
    glDeleteProgram(renderer->vanilla_shader_program);
    glDeleteProgram(renderer->msdf_shader_program);
    glDeleteBuffers(1, &renderer->vbo);
    glDeleteBuffers(1, &renderer->instance_vbo);
}

struct Vertex
{
    float x, y, u, v;
};

// Submit the display list and render to framebuffer
void
render_display_list(
    GlRenderer* renderer,
    System const& system,
    DisplayList const& display_list)
{
    glViewport(0, 0, system.framebuffer_size.x, system.framebuffer_size.y);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    float l = 0.f; // left
    float r = system.framebuffer_size.x; // right
    float t = 0.f; // top
    float b = system.framebuffer_size.y; // bottom
    float n = -1.f; // near
    float f = 1.f; // far

    float ortho[16] = {
        2.f / (r - l),
        0.f,
        0.f,
        0.f,
        0.f,
        2.f / (t - b),
        0.f,
        0.f,
        0.f,
        0.f,
        -2.f / (f - n),
        0.f,
        -(r + l) / (r - l),
        -(t + b) / (t - b),
        -(f + n) / (f - n),
        1.f};

    glUseProgram(renderer->vanilla_shader_program);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glUniformMatrix4fv(renderer->vanilla_matrix_location, 1, GL_FALSE, ortho);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    reset_arena(renderer->rect_instance_arena);
    RectInstance* rect_instances = (RectInstance*) arena_alloc(
        renderer->rect_instance_arena,
        sizeof(RectInstance) * display_list.count);
    for (size_t i = 0; i < display_list.count; ++i)
    {
        RectInstance* rect_instance = &rect_instances[i];
        rect_instance->pos = display_list.commands[i].box.pos;
        rect_instance->size = display_list.commands[i].box.size;
        rect_instance->color = display_list.commands[i].color;
    }

    glBindBuffer(GL_ARRAY_BUFFER, renderer->instance_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(RectInstance) * display_list.count,
        rect_instances,
        GL_STATIC_DRAW);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glBindVertexArray(renderer->vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, display_list.count);

    glUseProgram(renderer->msdf_shader_program);
    glUniformMatrix4fv(renderer->msdf_matrix_location, 1, GL_FALSE, ortho);
    glUniform4f(renderer->msdf_color_location, 1, 1, 1, 1); // white
}

} // namespace alia
