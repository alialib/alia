#include <alia/renderers/gl/renderer.hpp>

#include <alia/abi/base/arena.h>
#include <alia/abi/base/color.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/system/api.h>
#include <alia/impl/base/arena.hpp>

// TODO: Remove these.
#include <alia/ui/drawing.h>
#include <cstring>
#include <iostream>

namespace alia {

const char* vanilla_vertex_shader_source = R"(
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 i_pos;
layout (location = 2) in vec2 i_size;
layout (location = 3) in vec4 i_color;
layout (location = 4) in int i_primitive_type;
layout (location = 5) in vec4 i_payload;

uniform mat4 u_projection;

out vec4 v_color;
out vec2 v_pos;
flat out vec2 v_rect_center;
flat out vec2 v_rect_half_size;
flat out int v_primitive_type;
flat out float v_triangle_rotation_radians;
flat out float v_corner_radius;
flat out float v_border_width;
flat out vec4 v_border_color;

vec3 srgb_to_linear(vec3 srgb) {
    vec3 low = srgb / 12.92;
    vec3 high = pow((srgb + 0.055) / 1.055, vec3(2.4));
    vec3 mask = step(vec3(0.04045), srgb);
    return mix(low, high, mask);
}

vec4 srgba_to_linear(vec4 srgba) {
    return vec4(srgb_to_linear(srgba.rgb) * srgba.a, srgba.a);
}

void main() {
    vec2 scaled = a_pos * (i_size + vec2(2.0f)) + i_pos - vec2(1.0f);
    gl_Position = u_projection * vec4(scaled, 0.0, 1.0);

    v_color = srgba_to_linear(i_color);
    v_pos = scaled;
    v_rect_center = i_pos + i_size * 0.5;
    v_rect_half_size = i_size * 0.5;

    v_primitive_type = i_primitive_type;
    // Generic payload layout:
    // - payload.x: triangle rotation (radians) OR box corner_radius
    // - payload.y: box border_width
    // - payload.z: packed border_color bits (srgba8) reinterpreted as float
    v_triangle_rotation_radians = i_payload.x;

    v_corner_radius =
        min(i_payload.x, min(v_rect_half_size.x, v_rect_half_size.y));
    v_border_width = i_payload.y;

    // Pack/unpack strategy:
    // CPU stores border_color as raw 32-bit bits (r,g,b,a each 8-bit),
    // reinterprets those bits as a float, and uploads it as payload.z.
    // Here we reverse that.
    uint packed_bits = floatBitsToUint(i_payload.z);
    vec4 border_srgba
        = vec4(float(packed_bits & 255u),
               float((packed_bits >> 8u) & 255u),
               float((packed_bits >> 16u) & 255u),
               float((packed_bits >> 24u) & 255u))
          / 255.0;
    v_border_color = srgba_to_linear(border_srgba);
}
)";

const char* vanilla_fragment_shader_source = R"(
in vec4 v_color;
in vec2 v_pos;
flat in vec2 v_rect_center;
flat in vec2 v_rect_half_size;
flat in int v_primitive_type;
flat in float v_triangle_rotation_radians;
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

vec2 rotate_point_clockwise(vec2 p, float radians)
{
    float cs = cos(radians);
    float sn = sin(radians);
    // Positive rotation is clockwise in screen space (y-down).
    return vec2(cs * p.x - sn * p.y, sn * p.x + cs * p.y);
}

float cross2(vec2 u, vec2 v)
{
    return u.x * v.y - u.y * v.x;
}

float sd_segment(vec2 p, vec2 a, vec2 b)
{
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

float sd_equilateral_triangle(vec2 p, float R, float rotation_radians)
{
    // Base (unrotated) triangle with circumradius R:
    // - one vertex at (0, -R)
    // - other two at (+sqrt(3)/2*R, +R/2) and (-sqrt(3)/2*R, +R/2)
    float k = sqrt(3.0);
    vec2 a = vec2(0.0, -R);
    vec2 b = vec2(k * 0.5 * R, 0.5 * R);
    vec2 c = vec2(-k * 0.5 * R, 0.5 * R);

    // Rotate the triangle (not the point) to match the CPU-provided convention.
    a = rotate_point_clockwise(a, rotation_radians);
    b = rotate_point_clockwise(b, rotation_radians);
    c = rotate_point_clockwise(c, rotation_radians);

    float d = min(
        sd_segment(p, a, b),
        min(sd_segment(p, b, c), sd_segment(p, c, a)));

    // Sign: point-in-triangle test using edge cross products.
    float c1 = cross2(b - a, p - a);
    float c2 = cross2(c - b, p - b);
    float c3 = cross2(a - c, p - c);
    float cmin = min(c1, min(c2, c3));
    float cmax = max(c1, max(c2, c3));
    float sign = (cmin >= 0.0 || cmax <= 0.0) ? -1.0 : 1.0;
    return d * sign;
}

vec4 sample_pixel(vec2 p)
{
    vec2 local_p = v_pos - v_rect_center;

    // TODO: Take into account UI scaling if applicable.
    float aa = 0.5;

    switch (v_primitive_type)
    {
        // Box
        case 0:
        {
            float d = sd_round_rect(local_p, v_rect_half_size, v_corner_radius);
            float alpha_inner = smoothstep(-aa, aa, d + v_border_width);
            float alpha_outer = smoothstep(aa, -aa, d);
            vec4 mix_color = mix(v_color, v_border_color, alpha_inner);
            return mix_color * alpha_outer;
        }
        // Equilateral triangle
        case 1:
        {
            float R = min(v_rect_half_size.x, v_rect_half_size.y);
            float d = sd_equilateral_triangle(local_p, R, v_triangle_rotation_radians);
            float alpha_outer = smoothstep(aa, -aa, d);
            return v_color * alpha_outer;
        }
        default:
            return vec4(0.0);
    }
}

void main()
{
    frag_color = sample_pixel(v_pos);
}
)";

struct primitive_instance
{
    alia_vec2f min;
    alia_vec2f size;
    alia_srgba8 color;
    int primitive_type;
    // Generic per-primitive payload:
    // See `vanilla_vertex_shader_source` for the decoding rules.
    float payload[4];
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
        sizeof(primitive_instance),
        (void*) offsetof(primitive_instance, min));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    // size (location = 2)
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(primitive_instance),
        (void*) offsetof(primitive_instance, size));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // color (location = 3)
    glVertexAttribPointer(
        3,
        4,
        GL_UNSIGNED_BYTE,
        GL_TRUE,
        sizeof(primitive_instance),
        (void*) offsetof(primitive_instance, color));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // primitive type (location = 4)
    glVertexAttribIPointer(
        4,
        1,
        GL_INT,
        sizeof(primitive_instance),
        (void*) offsetof(primitive_instance, primitive_type));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // payload (location = 5)
    glVertexAttribPointer(
        5,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(primitive_instance),
        (void*) offsetof(primitive_instance, payload));
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
render_primitive_command_list(void* user, alia_draw_bucket const* bucket)
{
    gl_renderer* renderer = static_cast<gl_renderer*>(user);
    alia_draw_bucket const& boxes = *bucket;

    alia_box const* clip_rect = bucket->clip_rect;

    alia_vec2f const surface_size
        = alia_vec2i_to_vec2f(alia_ui_surface_get_size(renderer->system));

    glViewport(
        clip_rect->min.x,
        surface_size.y - (clip_rect->min.y + clip_rect->size.y),
        clip_rect->size.x,
        clip_rect->size.y);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    float l = clip_rect->min.x; // left
    float r = clip_rect->min.x + clip_rect->size.x; // right
    float t = (float) clip_rect->min.y; // top
    float b = clip_rect->min.y + clip_rect->size.y; // bottom
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
    primitive_instance* primitive_instances
        = arena_alloc_array<primitive_instance>(rect_alloc, boxes.count);

    auto pack_border_color = [](alia_srgba8 c) -> float {
        // Pack raw bytes into a u32 and reinterpret the u32 bits as float.
        uint32_t packed = uint32_t(c.r) | (uint32_t(c.g) << 8u)
                        | (uint32_t(c.b) << 16u) | (uint32_t(c.a) << 24u);
        float f;
        std::memcpy(&f, &packed, sizeof(f));
        return f;
    };

    {
        primitive_instance* instance = primitive_instances;
        for (auto const* cmd = boxes.head; cmd; cmd = cmd->next)
        {
            auto const* primitive_cmd
                = downcast<alia_draw_primitive_command>(cmd);
            instance->min = primitive_cmd->box.min;
            instance->size = primitive_cmd->box.size;
            instance->color = primitive_cmd->color;
            instance->primitive_type = int(primitive_cmd->primitive_type);
            instance->payload[0] = 0.0f;
            instance->payload[1] = 0.0f;
            instance->payload[2] = 0.0f;
            instance->payload[3] = 0.0f;

            if (primitive_cmd->primitive_type == ALIA_PRIMITIVE_BOX)
            {
                instance->payload[0]
                    = primitive_cmd->payload.box.corner_radius;
                instance->payload[1] = primitive_cmd->payload.box.border_width;
                instance->payload[2] = pack_border_color(
                    primitive_cmd->payload.box.border_color);
                instance->payload[3] = 0.0f;
            }
            else if (
                primitive_cmd->primitive_type
                == ALIA_PRIMITIVE_EQUILATERAL_TRIANGLE)
            {
                float const degrees_to_radians
                    = 3.14159265358979323846f / 180.0f;
                instance->payload[0]
                    = primitive_cmd->payload.triangle.rotation_degrees
                    * degrees_to_radians;
            }
            ++instance;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, renderer->instance_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(primitive_instance) * boxes.count,
        primitive_instances,
        GL_STREAM_DRAW);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glBindVertexArray(renderer->vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, boxes.count);
}

} // namespace alia
