#include <alia/renderers/gl/renderer.h>
#include <alia/renderers/gl/shaders.h>

#include "renderer_object.h"

#include <alia/abi/base/arena.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/effects.h>
#include <alia/abi/ui/msdf.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/system/renderer.h>
#include <alia/abi/ui/viewport.h>
#include <alia/impl/base/arena.hpp>
#include <alia/prelude.hpp>

// TODO: Remove these.
#include <alia/ui/drawing.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <vector>

namespace {

char const* primitive_vertex_shader_source = R"(
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

char const* primitive_fragment_shader_source = R"(
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
    float data_a[4];
    float data_b[4];
};

void
check_gl_errors()
{
}

void
renderer_setup_blend()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void
renderer_init_gpu(alia_ui_system* system, alia_gl_renderer* renderer)
{
    float quad_vertices[] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

    GLuint primitive_shader_program = alia_gl_create_shader_program(
        primitive_vertex_shader_source, primitive_fragment_shader_source);

    check_gl_errors();

    GLint primitive_matrix_location
        = glGetUniformLocation(primitive_shader_program, "u_projection");

    GLint msdf_sampler_location
        = glGetUniformLocation(primitive_shader_program, "u_msdf");

    check_gl_errors();

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

    check_gl_errors();

    GLuint instance_vbo;
    glGenBuffers(1, &instance_vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);

    check_gl_errors();

    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(primitive_instance),
        (void*) offsetof(primitive_instance, min));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(primitive_instance),
        (void*) offsetof(primitive_instance, size));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glVertexAttribPointer(
        3,
        4,
        GL_UNSIGNED_BYTE,
        GL_TRUE,
        sizeof(primitive_instance),
        (void*) offsetof(primitive_instance, color));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glVertexAttribIPointer(
        4,
        1,
        GL_INT,
        sizeof(primitive_instance),
        (void*) offsetof(primitive_instance, primitive_type));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    glVertexAttribPointer(
        5,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(primitive_instance),
        (void*) offsetof(primitive_instance, data_a));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    glVertexAttribPointer(
        6,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(primitive_instance),
        (void*) offsetof(primitive_instance, data_b));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);

    check_gl_errors();

    renderer->system = system;
    renderer->primitive_shader_program = primitive_shader_program;
    renderer->vao = vao;
    renderer->vbo = vbo;
    renderer->instance_vbo = instance_vbo;
    renderer->primitive_matrix_location = primitive_matrix_location;
    renderer->msdf_sampler_location = msdf_sampler_location;
    renderer->msdf_atlas_texture = 0;

    alia::initialize_lazy_commit_arena(&renderer->rect_instance_arena);
}

void
render_primitive_command_list(void* user, alia_draw_bucket const* bucket)
{
    alia_gl_renderer* renderer = static_cast<alia_gl_renderer*>(user);
    if (bucket->count == 0)
        return;

    alia_draw_bucket const& boxes = *bucket;

    alia_box const* clip_rect = bucket->clip_rect;

    alia_vec2f const surface_size
        = alia_vec2i_to_vec2f(alia_ui_surface_get_size(renderer->system));

    check_gl_errors();

    glViewport(
        clip_rect->min.x,
        surface_size.y - (clip_rect->min.y + clip_rect->size.y),
        clip_rect->size.x,
        clip_rect->size.y);

    check_gl_errors();

    float l = clip_rect->min.x;
    float r = clip_rect->min.x + clip_rect->size.x;
    float t = (float) clip_rect->min.y;
    float b = clip_rect->min.y + clip_rect->size.y;
    float n = -1.f;
    float f = 1.f;

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

    check_gl_errors();

    glUniformMatrix4fv(
        renderer->primitive_matrix_location, 1, GL_FALSE, ortho);

    check_gl_errors();

    alia_bump_allocator rect_alloc;
    alia_bump_allocator_init(&rect_alloc, &renderer->rect_instance_arena);
    primitive_instance* primitive_instances
        = alia::arena_alloc_array<primitive_instance>(rect_alloc, boxes.count);

    auto pack_border_color = [](alia_srgba8 c) -> float {
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
                = alia::downcast<alia_draw_primitive_command>(cmd);
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

    check_gl_errors();

    glBindVertexArray(renderer->vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, boxes.count);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    if (renderer->msdf_atlas_texture != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

char const* effect_vertex_shader_source = R"(
layout(location = 0) in vec2 position;
void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

static size_t
effect_params_ubo_bytes(size_t params_size)
{
    if (params_size == 0)
        return 16;
    return (params_size + 15u) & ~size_t(15u);
}

bool
ensure_effect_geometry(alia_gl_renderer* renderer)
{
    if (renderer->effect_vao != 0 && renderer->effect_vbo != 0
        && renderer->effect_frame_ubo != 0)
        return true;

    float vertices[] = {
        -1.f,
        -1.f,
        1.f,
        -1.f,
        1.f,
        1.f,
        -1.f,
        1.f,
        -1.f,
        -1.f,
        1.f,
        1.f,
    };

    if (renderer->effect_vao == 0 || renderer->effect_vbo == 0)
    {
        glGenVertexArrays(1, &renderer->effect_vao);
        glGenBuffers(1, &renderer->effect_vbo);
        glBindVertexArray(renderer->effect_vao);
        glBindBuffer(GL_ARRAY_BUFFER, renderer->effect_vbo);
        glBufferData(
            GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (renderer->effect_frame_ubo == 0)
    {
        glGenBuffers(1, &renderer->effect_frame_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, renderer->effect_frame_ubo);
        glBufferData(GL_UNIFORM_BUFFER, 32, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    return renderer->effect_vao != 0 && renderer->effect_vbo != 0
        && renderer->effect_frame_ubo != 0;
}

void
bind_effect_uniform_blocks(
    GLuint program,
    size_t params_size,
    size_t ubo_bytes,
    gl_effect_slot* slot)
{
    GLint block_count = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &block_count);
    for (GLint i = 0; i < block_count; ++i)
    {
        GLint data_size = 0;
        glGetActiveUniformBlockiv(
            program, GLuint(i), GL_UNIFORM_BLOCK_DATA_SIZE, &data_size);
        if (data_size == 32 && slot->frame_block_index == GL_INVALID_INDEX)
            slot->frame_block_index = GLuint(i);
        else if (
            params_size > 0 && size_t(data_size) == ubo_bytes
            && slot->params_block_index == GL_INVALID_INDEX)
            slot->params_block_index = GLuint(i);
    }

    // Legacy hand-written GLSL: named `Effect` UBO + frame uniforms.
    if (slot->frame_block_index == GL_INVALID_INDEX)
    {
        slot->loc_region = glGetUniformLocation(program, "alia_effect_region");
        slot->loc_surface
            = glGetUniformLocation(program, "alia_effect_surface");
    }
    if (slot->params_block_index == GL_INVALID_INDEX && params_size > 0)
    {
        GLuint named = glGetUniformBlockIndex(program, "Effect");
        if (named != GL_INVALID_INDEX)
            slot->params_block_index = named;
    }

    if (slot->frame_block_index != GL_INVALID_INDEX)
        glUniformBlockBinding(program, slot->frame_block_index, 0);
    if (slot->params_block_index != GL_INVALID_INDEX)
        glUniformBlockBinding(program, slot->params_block_index, 1);
}

void
render_effect_command_list(void* user, alia_draw_bucket const* bucket)
{
    auto* slot = static_cast<gl_effect_slot*>(user);
    if (!slot || !slot->renderer || !bucket || bucket->count == 0)
        return;

    alia_gl_renderer* renderer = slot->renderer;
    if (!renderer->system || renderer->effect_vao == 0 || slot->program == 0)
        return;

    alia_vec2f const surface_size
        = alia_vec2i_to_vec2f(alia_ui_surface_get_size(renderer->system));

    glDisable(GL_BLEND);
    glBindVertexArray(renderer->effect_vao);
    glUseProgram(slot->program);

    for (alia_draw_command const* walk = bucket->head; walk != nullptr;
         walk = walk->next)
    {
        auto const* effect_cmd
            = alia::downcast<alia_effect_draw_command>(walk);

        alia_box const& r = effect_cmd->region;
        if (r.size.x <= 0.f || r.size.y <= 0.f)
            continue;

        alia_gl_viewport const viewport
            = alia_viewport_region_to_gl_viewport(r, surface_size);
        glViewport(viewport.x, viewport.y, viewport.width, viewport.height);

        if (slot->frame_block_index != GL_INVALID_INDEX
            && renderer->effect_frame_ubo != 0)
        {
            float frame[8] = {
                r.min.x,
                r.min.y,
                r.size.x,
                r.size.y,
                surface_size.x,
                surface_size.y,
                0.f,
                0.f};
            glBindBuffer(GL_UNIFORM_BUFFER, renderer->effect_frame_ubo);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(frame), frame);
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, renderer->effect_frame_ubo);
        }
        else
        {
            if (slot->loc_region >= 0)
                glUniform4f(
                    slot->loc_region, r.min.x, r.min.y, r.size.x, r.size.y);
            if (slot->loc_surface >= 0)
                glUniform4f(
                    slot->loc_surface,
                    surface_size.x,
                    surface_size.y,
                    0.f,
                    0.f);
        }

        if (slot->params_ubo != 0 && slot->ubo_bytes > 0)
        {
            std::vector<unsigned char> blob(slot->ubo_bytes, 0);
            if (effect_cmd->params_size > 0 && effect_cmd->params)
            {
                size_t const copy_n
                    = effect_cmd->params_size < slot->params_size
                        ? effect_cmd->params_size
                        : slot->params_size;
                std::memcpy(blob.data(), effect_cmd->params, copy_n);
            }
            glBindBuffer(GL_UNIFORM_BUFFER, slot->params_ubo);
            glBufferSubData(
                GL_UNIFORM_BUFFER,
                0,
                GLsizeiptr(slot->ubo_bytes),
                blob.data());
            GLuint const params_binding
                = slot->frame_block_index != GL_INVALID_INDEX ? 1u : 0u;
            glBindBufferBase(
                GL_UNIFORM_BUFFER, params_binding, slot->params_ubo);
        }

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glEnable(GL_BLEND);
}

int
register_effect_blob(
    alia_gl_renderer* renderer,
    alia_effect_desc const* desc,
    alia_draw_material_id* out_material_id)
{
    ALIA_ASSERT(renderer);
    ALIA_ASSERT(renderer->system);
    ALIA_ASSERT(desc);
    ALIA_ASSERT(out_material_id);

    if (desc->shader.format != ALIA_SHADER_FORMAT_GLSL_ES
        || !desc->shader.data || desc->shader.size == 0)
    {
        std::fprintf(
            stderr, "[alia gl] effect requires ALIA_SHADER_FORMAT_GLSL_ES\n");
        return -1;
    }

    if (!ensure_effect_geometry(renderer))
    {
        std::fprintf(stderr, "[alia gl] effect geometry init failed\n");
        return -1;
    }

    // glShaderSource expects a NUL-terminated string when length is null.
    std::string source(
        static_cast<char const*>(desc->shader.data), desc->shader.size);

    GLuint program = alia_gl_create_shader_program(
        effect_vertex_shader_source, source.c_str());
    if (program == 0)
    {
        std::fprintf(stderr, "[alia gl] effect program creation failed\n");
        return -1;
    }

    auto slot = std::make_unique<gl_effect_slot>();
    slot->renderer = renderer;
    slot->program = program;
    slot->params_size = desc->params_size;
    slot->ubo_bytes = effect_params_ubo_bytes(desc->params_size);

    bind_effect_uniform_blocks(
        program, desc->params_size, slot->ubo_bytes, slot.get());

    if (desc->params_size > 0)
    {
        if (slot->params_block_index == GL_INVALID_INDEX)
        {
            std::fprintf(
                stderr, "[alia gl] effect params uniform block missing\n");
            glDeleteProgram(program);
            return -1;
        }
        glGenBuffers(1, &slot->params_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, slot->params_ubo);
        glBufferData(
            GL_UNIFORM_BUFFER,
            GLsizeiptr(slot->ubo_bytes),
            nullptr,
            GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    alia_draw_material_id const material_id
        = alia_material_alloc_ids(renderer->system, 1);
    alia_material_register(
        renderer->system,
        material_id,
        alia_material_vtable{.draw_bucket = render_effect_command_list},
        slot.get());

    renderer->effects.push_back(std::move(slot));
    *out_material_id = material_id;
    return 0;
}

} // namespace

extern "C" {

alia_struct_spec
alia_gl_renderer_object_spec(void)
{
    return alia_struct_spec{
        .size = sizeof(alia_gl_renderer), .align = alignof(alia_gl_renderer)};
}

alia_gl_renderer*
alia_gl_renderer_init(void* object_storage)
{
    ALIA_ASSERT(object_storage);
    return new (object_storage) alia_gl_renderer{};
}

void
alia_gl_renderer_attach(alia_gl_renderer* renderer, alia_ui_system* ui)
{
    ALIA_ASSERT(ui);
    ALIA_ASSERT(renderer);

    renderer_init_gpu(ui, renderer);
    alia_material_register(
        ui,
        ALIA_PRIMITIVE_MATERIAL_ID,
        alia_material_vtable{
            .draw_bucket = render_primitive_command_list,
        },
        renderer);
    renderer_setup_blend();

    alia_renderer_ops const ops = {
        .upload_msdf_atlas =
            [](void* user, alia_msdf_atlas_image const* image) {
                alia_gl_renderer_upload_msdf_atlas(
                    static_cast<alia_gl_renderer*>(user), image);
            },
        .register_effect =
            [](void* user,
               alia_effect_desc const* desc,
               alia_draw_material_id* out_material_id) {
                return register_effect_blob(
                    static_cast<alia_gl_renderer*>(user),
                    desc,
                    out_material_id);
            },
        .user = renderer,
    };
    alia_ui_system_set_renderer_ops(ui, &ops);
}

int
alia_gl_effect_register(
    alia_gl_renderer* renderer,
    alia_gl_effect_desc const* desc,
    alia_draw_material_id* out_material_id)
{
    ALIA_ASSERT(renderer);
    ALIA_ASSERT(desc);
    ALIA_ASSERT(desc->fragment_shader_source);
    ALIA_ASSERT(out_material_id);

    alia_effect_desc const portable = {
        .shader =
            {
                .format = ALIA_SHADER_FORMAT_GLSL_ES,
                .data = desc->fragment_shader_source,
                .size = std::strlen(desc->fragment_shader_source),
            },
        .params_size = desc->params_size,
    };
    return register_effect_blob(renderer, &portable, out_material_id);
}

void
alia_gl_renderer_upload_msdf_atlas(
    alia_gl_renderer* renderer, alia_msdf_atlas_image const* image)
{
    ALIA_ASSERT(renderer);
    ALIA_ASSERT(image);
    ALIA_ASSERT(image->rgb);

    int const width = image->width;
    int const height = image->height;
    unsigned char const* const atlas_rgb = image->rgb;

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
alia_gl_renderer_destroy(alia_gl_renderer* renderer)
{
    ALIA_ASSERT(renderer);

    for (auto& slot : renderer->effects)
    {
        if (!slot)
            continue;
        if (slot->program != 0)
            glDeleteProgram(slot->program);
        if (slot->params_ubo != 0)
            glDeleteBuffers(1, &slot->params_ubo);
    }
    renderer->effects.clear();
    if (renderer->effect_vbo != 0)
    {
        glDeleteBuffers(1, &renderer->effect_vbo);
        renderer->effect_vbo = 0;
    }
    if (renderer->effect_vao != 0)
    {
        glDeleteVertexArrays(1, &renderer->effect_vao);
        renderer->effect_vao = 0;
    }
    if (renderer->effect_frame_ubo != 0)
    {
        glDeleteBuffers(1, &renderer->effect_frame_ubo);
        renderer->effect_frame_ubo = 0;
    }

    if (renderer->msdf_atlas_texture != 0)
    {
        glDeleteTextures(1, &renderer->msdf_atlas_texture);
        renderer->msdf_atlas_texture = 0;
    }
    if (renderer->primitive_shader_program != 0)
    {
        glDeleteProgram(renderer->primitive_shader_program);
        renderer->primitive_shader_program = 0;
    }
    if (renderer->vbo != 0)
    {
        glDeleteBuffers(1, &renderer->vbo);
        renderer->vbo = 0;
    }
    if (renderer->instance_vbo != 0)
    {
        glDeleteBuffers(1, &renderer->instance_vbo);
        renderer->instance_vbo = 0;
    }
    if (renderer->vao != 0)
    {
        glDeleteVertexArrays(1, &renderer->vao);
        renderer->vao = 0;
    }
}

} // extern "C"
