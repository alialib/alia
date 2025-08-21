#include <alia/renderers/gl/renderer.hpp>

#include <alia/kernel/infinite_arena.hpp>
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

const char* vanilla_vertex_shader_source = R"(
#version 330 core
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 i_pos;
layout (location = 2) in vec2 i_size;
layout (location = 3) in vec4 i_color;
layout (location = 4) in uint i_clip_index;

uniform mat4 u_projection;

out vec4 v_color;
flat out uint v_clip_index;

void main() {
    vec2 scaled = a_pos * i_size + i_pos;
    gl_Position = u_projection * vec4(scaled, 0.0, 1.0);
    v_color = i_color;
    v_clip_index = i_clip_index;
}
)";

const char* vanilla_fragment_shader_source = R"(
#version 330 core

layout (std140) uniform ClipUBO {
    vec4 clips[1024]; // TODO: Make this dynamic and adjust for GL limits.
};

in vec4 v_color;
flat in uint v_clip_index;

out vec4 frag_color;

void main() {
    vec4 r = clips[v_clip_index];
    vec2 p = vec2(gl_FragCoord.x, gl_FragCoord.y);
    bvec4 inside = bvec4(p.x >= r.x,
                         p.y >= r.y,
                         p.x <  r.x + r.z,
                         p.y <  r.y + r.w);
    if (!(inside.x && inside.y && inside.z && inside.w)) discard;

    frag_color = v_color;
}
)";

struct RectInstance
{
    Vec2 pos;
    Vec2 size;
    Color color;
    GLuint clip_index;
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

    constexpr GLsizeiptr max_clips = 1024;
    constexpr GLsizeiptr bytes_per_clip = sizeof(float) * 4; // vec4
    constexpr GLsizeiptr ubo_size = max_clips * bytes_per_clip;

    GLuint clip_ubo = 0;
    glCreateBuffers(1, &clip_ubo);

    glNamedBufferStorage(
        clip_ubo,
        ubo_size,
        nullptr,
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    GLuint clip_block
        = glGetUniformBlockIndex(vanilla_shader_program, "ClipUBO");
    glBindBufferBase(GL_UNIFORM_BUFFER, clip_block, clip_ubo);

    InfiniteArena* rect_instance_arena = new InfiniteArena; // TODO
    rect_instance_arena->initialize();

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

    glVertexAttribIPointer(
        4,
        1,
        GL_UNSIGNED_INT,
        sizeof(RectInstance),
        (void*) offsetof(RectInstance, clip_index));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    float* clip_ptr = (float*) glMapNamedBufferRange(
        clip_ubo,
        0,
        ubo_size,
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    *renderer = {
        .vanilla_shader_program = vanilla_shader_program,
        .vao = vao,
        .vbo = vbo,
        .instance_vbo = instance_vbo,
        .vanilla_matrix_location = vanilla_matrix_location,
        .rect_instance_arena = rect_instance_arena,
        .clip_ubo = clip_ubo,
        .clip_ptr = clip_ptr,
    };
}

void
destroy_gl_renderer(GlRenderer* renderer)
{
    glDeleteProgram(renderer->vanilla_shader_program);
    glDeleteBuffers(1, &renderer->vbo);
    glDeleteBuffers(1, &renderer->instance_vbo);
    glDeleteBuffers(1, &renderer->clip_ubo);
}

struct Vertex
{
    float x, y, u, v;
};

void
render_box_command_list(
    GlRenderer* renderer, System const& system, BoxCommandList const& boxes)
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

    renderer->clip_ptr[0] = 0.f;
    renderer->clip_ptr[1] = 0.f;
    renderer->clip_ptr[2] = system.framebuffer_size.x;
    renderer->clip_ptr[3] = system.framebuffer_size.y;

    renderer->rect_instance_arena->reset();
    RectInstance* rect_instances = arena_array_alloc<RectInstance>(
        *renderer->rect_instance_arena, boxes.count);
    {
        RectInstance* instance = rect_instances;
        for (auto const* cmd = boxes.head; cmd; cmd = cmd->next)
        {
            instance->pos = cmd->box.pos;
            instance->size = cmd->box.size;
            instance->color = cmd->color;
            instance->clip_index = 0; // TODO: Add clip index.
            ++instance;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, renderer->instance_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(RectInstance) * boxes.count,
        rect_instances,
        GL_STATIC_DRAW);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glBindVertexArray(renderer->vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, boxes.count);
}

} // namespace alia
