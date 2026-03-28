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

const char* primitive_vertex_shader_source = R"(
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 i_pos;
layout (location = 2) in vec2 i_size;
layout (location = 3) in vec4 i_color;
layout (location = 4) in int i_primitive_type;
layout (location = 5) in vec4 i_data_a;
layout (location = 6) in vec4 i_data_b;

uniform mat4 u_projection;

out vec4 v_color;
out vec2 v_pos;
flat out vec2 v_rect_center;
flat out vec2 v_rect_half_size;
flat out int v_primitive_type;
flat out vec4 v_data_a;
flat out vec4 v_data_b;
out vec2 v_uv_msdf;
flat out float v_msdf_sdf_scale;

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
    if (i_primitive_type == 3) {
        vec2 scaled = a_pos * i_size + i_pos;
        gl_Position = u_projection * vec4(scaled, 0.0, 1.0);

        v_color = srgba_to_linear(i_color);
        v_pos = vec2(0.0);
        v_rect_center = vec2(0.0);
        v_rect_half_size = vec2(0.0);

        v_primitive_type = i_primitive_type;
        v_data_a = i_data_a;
        v_data_b = i_data_b;

        vec2 uv_min = vec2(i_data_a[0], i_data_a[1]);
        vec2 uv_sz = vec2(i_data_a[2], i_data_a[3]);
        // y-down quad (a_pos.y=0 at top): match atlas to screen by flipping v.
        v_uv_msdf = vec2(
            uv_min.x + a_pos.x * uv_sz.x,
            uv_min.y + (1.0 - a_pos.y) * uv_sz.y);
        v_msdf_sdf_scale = i_data_b[0];
    } else {
        vec2 scaled = a_pos * (i_size + vec2(2.0f)) + i_pos - vec2(1.0f);
        gl_Position = u_projection * vec4(scaled, 0.0, 1.0);

        v_color = srgba_to_linear(i_color);
        v_pos = scaled;
        v_rect_center = i_pos + i_size * 0.5;
        v_rect_half_size = i_size * 0.5;

        v_primitive_type = i_primitive_type;
        v_data_a = i_data_a;
        v_data_b = i_data_b;

        v_uv_msdf = vec2(0.0);
        v_msdf_sdf_scale = 0.0;
    }
}
)";

const char* primitive_fragment_shader_source = R"(
in vec4 v_color;
in vec2 v_pos;
flat in vec2 v_rect_center;
flat in vec2 v_rect_half_size;
flat in int v_primitive_type;
flat in vec4 v_data_a;
flat in vec4 v_data_b;
in vec2 v_uv_msdf;
flat in float v_msdf_sdf_scale;

out vec4 frag_color;

uniform sampler2D u_msdf;

float median_msdf(vec3 v) {
    return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

vec3 srgb_to_linear(vec3 srgb) {
    vec3 low = srgb / 12.92;
    vec3 high = pow((srgb + 0.055) / 1.055, vec3(2.4));
    vec3 mask = step(vec3(0.04045), srgb);
    return mix(low, high, mask);
}

vec4 srgba_to_linear(vec4 srgba) {
    return vec4(srgb_to_linear(srgba.rgb) * srgba.a, srgba.a);
}

// CPU packs border_color bytes into a u32, reinterprets as float -> data_a.z.
vec4 unpack_border_color_linear(float packed_as_float) {
    uint packed_bits = floatBitsToUint(packed_as_float);
    vec4 border_srgba
        = vec4(float(packed_bits & 255u),
               float((packed_bits >> 8u) & 255u),
               float((packed_bits >> 16u) & 255u),
               float((packed_bits >> 24u) & 255u))
          / 255.0;
    return srgba_to_linear(border_srgba);
}

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

float sd_equilateral_triangle(vec2 p, float r)
{
    // adapted from: https://iquilezles.org/articles/distfunctions2d/
    const float k = sqrt(3.0);
    p.x = abs(p.x) - r;
    p.y = r / k - p.y;
    if (p.x + k * p.y > 0.0)
        p = vec2(p.x - k * p.y, -k * p.x - p.y) / 2.0;
    p.x -= clamp(p.x, -2.0 * r, 0.0);
    return -length(p) * sign(p.y);
}

float sd_squircle(vec2 p, float R)
{
    float x2 = p.x * p.x;
    float y2 = p.y * p.y;
    return sqrt(sqrt(x2 * x2 + y2 * y2)) - R;
}

vec3 linear_to_srgb(vec3 linear) {
    vec3 s1 = sqrt(linear);
    vec3 s2 = sqrt(s1);
    vec3 s3 = sqrt(s2);
    return 0.662002687 * s1 + 0.684122060 * s2 - 0.323583601 * s3 -
        0.0225411470 * linear;
}

vec4 apply_aa_and_postprocess(vec4 color, float aa_alpha) {
  #ifdef EMSCRIPTEN
    vec3 unpremultiplied = color.a > 0.0 ? color.rgb / color.a : vec3(0.0);
    float alpha = color.a * aa_alpha;
    return vec4(linear_to_srgb(unpremultiplied) * alpha, alpha);
  #else
    return color * aa_alpha;
  #endif
}

vec4 sample_primitive(vec2 p)
{
    vec2 local_p = v_pos - v_rect_center;

    // UI is always in physical pixels, so AA is 0.5px.
    float aa = 0.5;

    switch (v_primitive_type)
    {
        // ALIA_PRIMITIVE_BOX
        case 0:
        {
            float corner_radius = min(
                v_data_a.x, min(v_rect_half_size.x, v_rect_half_size.y));
            float border_width = v_data_a.y;
            vec4 border_color = unpack_border_color_linear(v_data_a.z);
            float d = sd_round_rect(local_p, v_rect_half_size, corner_radius);
            float alpha_inner = smoothstep(-aa, aa, d + border_width);
            float alpha_outer = smoothstep(aa, -aa, d);
            vec4 mix_color = mix(v_color, border_color, alpha_inner);
            return apply_aa_and_postprocess(mix_color, alpha_outer);
        }
        // ALIA_PRIMITIVE_EQUILATERAL_TRIANGLE
        case 1:
        {
            float R = min(v_rect_half_size.x, v_rect_half_size.y);
            float d = sd_equilateral_triangle(
                rotate_point_clockwise(local_p, -v_data_a.x), R);
            float alpha = smoothstep(aa, -aa, d);
            return apply_aa_and_postprocess(v_color, alpha);
        }
        // ALIA_PRIMITIVE_SQUIRCLE
        case 2:
        {
            float squircle_radius = v_data_a.x;
            float border_width = v_data_a.y;
            vec4 border_color = unpack_border_color_linear(v_data_a.z);
            float d = sd_squircle(local_p, squircle_radius);
            float alpha_inner = smoothstep(-aa, aa, d + border_width);
            float alpha_outer = smoothstep(aa, -aa, d);
            vec4 mix_color = mix(v_color, border_color, alpha_inner);
            return apply_aa_and_postprocess(mix_color, alpha_outer);
        }
        // ALIA_PRIMITIVE_MSDF_GLYPH
        case 3:
        {
            vec3 msd = texture(u_msdf, v_uv_msdf).rgb;
            float sd = median_msdf(msd) - 0.5;
            float screen_px_distance = v_msdf_sdf_scale * sd;
            float opacity = clamp(screen_px_distance + 0.5, 0.0, 1.0);
            return apply_aa_and_postprocess(v_color, opacity);
        }
        default:
            return vec4(0.0);
    }
}

void main()
{
    frag_color = sample_primitive(v_pos);
}
)";

struct primitive_instance
{
    alia_vec2f min;
    alia_vec2f size;
    alia_srgba8 color;
    int primitive_type;
    // Per-primitive-type layouts; see fragment shader `sample_pixel`.
    float data_a[4];
    float data_b[4];
};

GLuint
compile_shader(GLenum type, const char* source)
{
// 1. Define the header based on the platform
// Note: WebGL fragment shaders require explicit precision.
// It's safe to put this in vertex shaders too (it's the default there).
#ifdef __EMSCRIPTEN__
    const char* header
        = "#version 300 es\nprecision highp float;\n#define EMSCRIPTEN 1\n";
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

    GLuint primitive_shader_program = create_shader_program(
        primitive_vertex_shader_source, primitive_fragment_shader_source);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    GLint primitive_matrix_location
        = glGetUniformLocation(primitive_shader_program, "u_projection");

    GLint msdf_sampler_location
        = glGetUniformLocation(primitive_shader_program, "u_msdf");

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

    // data_a (location = 5)
    glVertexAttribPointer(
        5,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(primitive_instance),
        (void*) offsetof(primitive_instance, data_a));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    // data_b (location = 6)
    glVertexAttribPointer(
        6,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(primitive_instance),
        (void*) offsetof(primitive_instance, data_b));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    *renderer = {
        .system = system,
        .primitive_shader_program = primitive_shader_program,
        .vao = vao,
        .vbo = vbo,
        .instance_vbo = instance_vbo,
        .primitive_matrix_location = primitive_matrix_location,
        .msdf_sampler_location = msdf_sampler_location,
        .msdf_atlas_texture = 0,
    };

    initialize_lazy_commit_arena(&renderer->rect_instance_arena);
}

void
gl_renderer_upload_msdf_atlas(
    gl_renderer* renderer,
    unsigned char const* atlas_rgb,
    int width,
    int height)
{
    if (renderer->msdf_atlas_texture != 0)
    {
        glDeleteTextures(1, &renderer->msdf_atlas_texture);
        renderer->msdf_atlas_texture = 0;
    }

    glGenTextures(1, &renderer->msdf_atlas_texture);
    glBindTexture(GL_TEXTURE_2D, renderer->msdf_atlas_texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB8,
        width,
        height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        atlas_rgb);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void
destroy_gl_renderer(gl_renderer* renderer)
{
    if (renderer->msdf_atlas_texture != 0)
    {
        glDeleteTextures(1, &renderer->msdf_atlas_texture);
        renderer->msdf_atlas_texture = 0;
    }
    glDeleteProgram(renderer->primitive_shader_program);
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

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glViewport(
        clip_rect->min.x,
        surface_size.y - (clip_rect->min.y + clip_rect->size.y),
        clip_rect->size.x,
        clip_rect->size.y);

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

    glUseProgram(renderer->primitive_shader_program);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glUniformMatrix4fv(
        renderer->primitive_matrix_location, 1, GL_FALSE, ortho);

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
            std::memset(instance->data_a, 0, sizeof(instance->data_a));
            std::memset(instance->data_b, 0, sizeof(instance->data_b));

            switch (primitive_cmd->primitive_type)
            {
                case ALIA_PRIMITIVE_BOX: {
                    instance->data_a[0]
                        = primitive_cmd->payload.box.corner_radius;
                    instance->data_a[1]
                        = primitive_cmd->payload.box.border_width;
                    instance->data_a[2] = pack_border_color(
                        primitive_cmd->payload.box.border_color);
                    instance->data_a[3] = 0.0f;
                    break;
                }
                case ALIA_PRIMITIVE_EQUILATERAL_TRIANGLE: {
                    float const degrees_to_radians
                        = 3.14159265358979323846f / 180.0f;
                    instance->data_a[0]
                        = primitive_cmd->payload.triangle.rotation_degrees
                        * degrees_to_radians;
                    break;
                }
                case ALIA_PRIMITIVE_SQUIRCLE: {
                    instance->data_a[0]
                        = primitive_cmd->payload.squircle.radius;
                    instance->data_a[1]
                        = primitive_cmd->payload.squircle.border_width;
                    instance->data_a[2] = pack_border_color(
                        primitive_cmd->payload.squircle.border_color);
                    instance->data_a[3] = 0.0f;
                    break;
                }
                case ALIA_PRIMITIVE_MSDF_GLYPH: {
                    instance->data_a[0]
                        = primitive_cmd->payload.msdf_glyph.uv_rect[0];
                    instance->data_a[1]
                        = primitive_cmd->payload.msdf_glyph.uv_rect[1];
                    instance->data_a[2]
                        = primitive_cmd->payload.msdf_glyph.uv_rect[2];
                    instance->data_a[3]
                        = primitive_cmd->payload.msdf_glyph.uv_rect[3];
                    instance->data_b[0]
                        = primitive_cmd->payload.msdf_glyph.sdf_scale;
                    instance->data_b[1] = 0.0f;
                    instance->data_b[2] = 0.0f;
                    instance->data_b[3] = 0.0f;
                    break;
                }
            }
            ++instance;
        }
    }

    if (renderer->msdf_atlas_texture != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderer->msdf_atlas_texture);
        glUniform1i(renderer->msdf_sampler_location, 0);
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
