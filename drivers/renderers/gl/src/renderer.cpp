#include <alia/renderers/gl/renderer.hpp>

#include <alia/arena.hpp>
#include <alia/drawing.hpp>
#include <alia/system/object.hpp>

// TODO: Remove this.
#include <alia/internals/drawing.hpp>

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

const char* vanilla_vertex_shader_source = R"(
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 i_pos;
layout (location = 2) in vec2 i_size;
layout (location = 3) in vec4 i_color;
layout (location = 4) in float i_radius;
layout (location = 5) in vec4 i_clip_rect;

uniform mat4 u_projection;

out vec4 v_color;
out vec2 v_pos;
out vec2 v_rect_center;
out vec2 v_rect_half_size;
flat out float v_radius;
flat out vec4 v_clip_rect;

void main() {
    vec2 scaled = a_pos * i_size + i_pos;
    gl_Position = u_projection * vec4(scaled, 0.0, 1.0);
    v_color = i_color;
    v_pos = scaled;
    v_rect_center = i_pos + i_size * 0.5;
    v_rect_half_size = i_size * 0.5;
    v_radius = i_radius;
    v_clip_rect = i_clip_rect;
}
)";

const char* vanilla_fragment_shader_source = R"(
in vec4 v_color;
in vec2 v_pos;
in vec2 v_rect_center;
in vec2 v_rect_half_size;
flat in float v_radius;
flat in vec4 v_clip_rect;
out vec4 frag_color;

float sd_round_rect(vec2 p, vec2 b, float r)
{
    // Clamp radius so it can't exceed the half-size in either axis.
    r = min(r, min(b.x, b.y));

    // q is how far we are outside the inner rectangle (with corners cut out)
    vec2 q = abs(p) - (b - vec2(r));

    // Outside distance: length of positive part
    float outside = length(max(q, 0.0));

    // Inside distance: if we're inside, q's max component is <= 0
    float inside = min(max(q.x, q.y), 0.0);

    return outside + inside - r; // negative inside, 0 on boundary
}

void main() {
    if (v_pos.x < v_clip_rect.x || v_pos.y < v_clip_rect.y ||
        v_pos.x > v_clip_rect.z || v_pos.y > v_clip_rect.w)
    {
        discard;
    }

    float d = sd_round_rect(v_pos - v_rect_center, v_rect_half_size, v_radius);

    float aa = fwidth(d);                    // ~ size of one pixel in distance units
    float alpha = smoothstep(0.0, -aa, d);   // 1 inside, 0 outside

    frag_color = vec4(v_color.rgb, v_color.a * alpha);
}
)";

struct rect_instance
{
    vec2f min;
    vec2f size;
    alia::color color;
    float radius;
    float clip_rect[4];
};

GLuint
compile_shader(GLenum type, const char* source)
{
// 1. Define the header based on the platform
// Note: WebGL fragment shaders require explicit precision.
// It's safe to put this in vertex shaders too (it's the default there).
#ifdef __EMSCRIPTEN__
    const char* header = "#version 300 es\nprecision highp float;\n";
#else
    const char* header = "#version 330 core\n";
#endif
    const char* sources[] = {header, source};
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 2, sources, nullptr);
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
init_gl_renderer(alia_draw_system* system, gl_renderer* renderer)
{
    float quad_vertices[] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

    GLuint vanilla_shader_program = create_shader_program(
        vanilla_vertex_shader_source, vanilla_fragment_shader_source);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    GLint vanilla_matrix_location
        = glGetUniformLocation(vanilla_shader_program, "u_projection");

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
        sizeof(rect_instance),
        (void*) offsetof(rect_instance, min));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    // size (location = 2)
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(rect_instance),
        (void*) offsetof(rect_instance, size));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // color (location = 3)
    glVertexAttribPointer(
        3,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(rect_instance),
        (void*) offsetof(rect_instance, color));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // radius (location = 4)
    glVertexAttribPointer(
        4,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(rect_instance),
        (void*) offsetof(rect_instance, radius));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // clip rect (location = 5)
    glVertexAttribPointer(
        5,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(rect_instance),
        (void*) offsetof(rect_instance, clip_rect));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    *renderer = {
        .system = system,
        .vanilla_shader_program = vanilla_shader_program,
        .vao = vao,
        .vbo = vbo,
        .instance_vbo = instance_vbo,
        .vanilla_matrix_location = vanilla_matrix_location,
    };

    initialize_lazy_commit_arena(&renderer->rect_instance_arena);
}

void
destroy_gl_renderer(gl_renderer* renderer)
{
    glDeleteProgram(renderer->vanilla_shader_program);
    glDeleteBuffers(1, &renderer->vbo);
    glDeleteBuffers(1, &renderer->instance_vbo);
}

struct Vertex
{
    float x, y, u, v;
};

void
render_box_command_list(void* user, alia_draw_bucket const* bucket)
{
    gl_renderer* renderer = static_cast<gl_renderer*>(user);
    auto const& system = *renderer->system;
    alia_draw_bucket const& boxes = *bucket;

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    float l = 0.f; // left
    float r = system.surface_size.x; // right
    float t = 0.f; // top
    float b = system.surface_size.y; // bottom
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

    float clip_rect[4]
        = {0.f, 0.f, system.surface_size.x, system.surface_size.y};

    alia_arena_reset(alia_arena_get_view(&renderer->rect_instance_arena));
    rect_instance* rect_instances = arena_alloc_array<rect_instance>(
        *alia_arena_get_view(&renderer->rect_instance_arena), boxes.count);
    {
        rect_instance* instance = rect_instances;
        for (auto const* cmd = boxes.head; cmd; cmd = cmd->next)
        {
            auto const* box_cmd = downcast<box_draw_command>(cmd);
            instance->min = box_cmd->box.min;
            instance->size = box_cmd->box.size;
            instance->color = box_cmd->color;
            instance->radius = box_cmd->radius;
            instance->clip_rect[0] = clip_rect[0];
            instance->clip_rect[1] = clip_rect[1];
            instance->clip_rect[2] = clip_rect[2];
            instance->clip_rect[3] = clip_rect[3];
            ++instance;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, renderer->instance_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(rect_instance) * boxes.count,
        rect_instances,
        GL_STATIC_DRAW);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glBindVertexArray(renderer->vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, boxes.count);
}

} // namespace alia
