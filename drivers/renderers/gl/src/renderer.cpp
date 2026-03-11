#include <alia/renderers/gl/renderer.hpp>

#include <alia/abi/base/arena.h>
#include <alia/abi/base/color.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/system/api.h>
#include <alia/impl/base/arena.hpp>

// TODO: Remove these.
#include <alia/ui/drawing.h>
#include <iostream>

// extern "C" {

// struct gl_renderer
// {
//     alia_draw_system* system;
//     GLuint vanilla_shader_program;
//     GLuint vao, vbo;
//     GLuint instance_vbo;
//     GLint vanilla_matrix_location;
//     alia_arena rect_instance_arena;
// };

// } // extern "C"

namespace alia {

const char* vanilla_vertex_shader_source = R"(
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 i_pos;
layout (location = 2) in vec2 i_size;
layout (location = 3) in vec4 i_fill_color;
layout (location = 4) in float i_corner_radius;
layout (location = 5) in float i_border_width;
layout (location = 6) in vec4 i_border_color;

uniform mat4 u_projection;

out vec4 v_fill_color;
out vec2 v_pos;
flat out vec2 v_rect_center;
flat out vec2 v_rect_half_size;
flat out float v_corner_radius;
flat out float v_border_width;
flat out vec4 v_border_color;

void main() {
    vec2 scaled = a_pos * (i_size + vec2(2.0f)) + i_pos - vec2(1.0f);
    gl_Position = u_projection * vec4(scaled, 0.0, 1.0);
    v_fill_color = i_fill_color;
    v_pos = scaled;
    v_rect_center = i_pos + i_size * 0.5;
    v_rect_half_size = i_size * 0.5;
    v_corner_radius =
        min(i_corner_radius, min(v_rect_half_size.x, v_rect_half_size.y));
    v_border_width = i_border_width;
    v_border_color = i_border_color;
}
)";

const char* vanilla_fragment_shader_source = R"(
in vec4 v_fill_color;
in vec2 v_pos;
flat in vec2 v_rect_center;
flat in vec2 v_rect_half_size;
flat in float v_corner_radius;
flat in float v_border_width;
flat in vec4 v_border_color;
out vec4 frag_color;

float sd_round_rect(vec2 p, vec2 b, float r)
{
    // `q` is how far we are outside the inner rectangle (with corners cut out)
    vec2 q = abs(p) - (b - vec2(r));

    // Outside distance: length of positive part
    float outside = length(max(q, 0.0));

    // Inside distance: if we're inside, q's max component is <= 0
    float inside = min(max(q.x, q.y), 0.0);

    return outside + inside - r; // negative inside, 0 on boundary
}

vec4 sample_pixel(vec2 p) {
    float d =
        sd_round_rect(
            v_pos - v_rect_center,
            v_rect_half_size,
            v_corner_radius);

    // TODO: Take into account UI scaling if applicable.
    float aa = 0.5;
    float alpha_inner = smoothstep(-aa, aa, d + v_border_width);
    float alpha_outer = smoothstep(aa, -aa, d);

    vec4 mix_color = mix(v_fill_color, v_border_color, alpha_inner);
    return mix_color * alpha_outer;
}

void main() {
    frag_color = sample_pixel(v_pos);
}
)";

struct rect_instance
{
    alia_vec2f min;
    alia_vec2f size;
    alia_rgba fill_color;
    alia_rgba border_color;
    float corner_radius;
    float border_width;
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
init_gl_renderer(alia_ui_system* system, gl_renderer* renderer)
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
        (void*) offsetof(rect_instance, fill_color));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // corner radius (location = 4)
    glVertexAttribPointer(
        4,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(rect_instance),
        (void*) offsetof(rect_instance, corner_radius));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // border width (location = 5)
    glVertexAttribPointer(
        5,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(rect_instance),
        (void*) offsetof(rect_instance, border_width));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    // border color (location = 6)
    glVertexAttribPointer(
        6,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(rect_instance),
        (void*) offsetof(rect_instance, border_color));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);

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
    alia_draw_bucket const& boxes = *bucket;

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    alia_vec2f surface_size
        = alia_ui_system_get_surface_size(renderer->system);

    float l = 0.f; // left
    float r = surface_size.x; // right
    float t = 0.f; // top
    float b = surface_size.y; // bottom
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

    alia_bump_allocator rect_alloc;
    alia_bump_allocator_init(&rect_alloc, &renderer->rect_instance_arena);
    rect_instance* rect_instances
        = arena_alloc_array<rect_instance>(rect_alloc, boxes.count);
    {
        rect_instance* instance = rect_instances;
        for (auto const* cmd = boxes.head; cmd; cmd = cmd->next)
        {
            auto const* box_cmd = downcast<alia_draw_box_command>(cmd);
            instance->min = box_cmd->box.min;
            instance->size = box_cmd->box.size;
            instance->fill_color = box_cmd->paint.fill_color;
            instance->border_color = box_cmd->paint.border_color;
            instance->corner_radius = box_cmd->paint.corner_radius;
            instance->border_width = box_cmd->paint.border_width;
            ++instance;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, renderer->instance_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(rect_instance) * boxes.count,
        rect_instances,
        GL_STREAM_DRAW);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glBindVertexArray(renderer->vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, boxes.count);
}

} // namespace alia
