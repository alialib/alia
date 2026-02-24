#include <alia/text_engines/msdf/msdf.hpp>

#include <alia/abi/base/color.h>
#include <alia/abi/ui/drawing.h>
#include <alia/base/arena.h>
#include <alia/renderers/gl/renderer.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <unordered_map>
#include <vector>

// TODO: Remove these.
#include <alia/impl/base/arena.hpp>
#include <alia/ui/drawing.h>
#include <iostream>

using namespace alia::operators;

namespace alia {

const char* msdf_vertex_shader_source = R"(
layout (location = 0) in vec2 a_vertex_pos;
layout (location = 1) in vec4 i_color;
layout (location = 2) in vec2 i_glyph_pos;
layout (location = 3) in float i_scale;
layout (location = 4) in vec4 i_uv_rect;    // .xy = pos, .zw = size
layout (location = 5) in vec4 i_plane_rect; // .xy = pos, .zw = size

flat out vec4 v_color;
flat out float v_scale;
out vec2 v_uv;

uniform mat4 u_projection;

void main() {
    vec2 xy_pos = i_plane_rect.xy;
    vec2 xy_size = i_plane_rect.zw;
    vec2 uv_pos = i_uv_rect.xy;
    vec2 uv_size = i_uv_rect.zw;

    vec2 xy = i_glyph_pos + (xy_pos + a_vertex_pos * xy_size) * i_scale;
    gl_Position = u_projection * vec4(xy, 0.0, 1.0);

    v_uv = uv_pos + a_vertex_pos * uv_size;
    v_color = i_color;
    v_scale = i_scale;
}
)";

const char* msdf_fragment_shader_source = R"(
flat in vec4 v_color;
flat in float v_scale;
in vec2 v_uv;

out vec4 out_color;

uniform sampler2D u_msdf;

float median(vec3 v) {
    return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

const float output_bias = 0.0f;

void main() {
    vec3 msd = texture(u_msdf, v_uv).rgb;
    float sd = median(msd) - 0.5;
    float screen_px_distance = v_scale / 12.0f * sd + output_bias;
    float opacity = clamp(screen_px_distance + 0.5, 0.0, 1.0);
    // float glow = smoothstep(-0.5, 0.0, sd); // 0.0 at glyph edge
    opacity *= smoothstep(0.05, 0.2, opacity);
    opacity = pow(opacity, 1.0 / 2.2);
    vec4 glyph_color = vec4(v_color.rgb, v_color.a * opacity);
    // vec4 glow_color = vec4(1, 0.2, 0.2, v_color.a * glow);
    // out_color = mix(glow_color, glyph_color, opacity);
    out_color = glyph_color;
}
)";

struct gpu_glyph_instance
{
    alia_rgba color;
    alia_vec2f position;
    float scale;
    float uv_rect[4];
    float plane_rect[4];
};

struct msdf_gpu_data
{
    GLuint shader_program;
    GLint matrix_location;

    GLuint texture;

    GLuint vao;
    GLuint quad_vbo;
    GLuint instance_vbo;

    // TODO: Don't use a vector here.
    std::vector<gpu_glyph_instance> glyph_instances;
};

struct kerning_pair_index
{
    uint32_t left;
    uint32_t right;

    bool
    operator==(kerning_pair_index const& other) const
        = default;
};

struct kerning_pair_index_hash
{
    std::size_t
    operator()(kerning_pair_index const& pair) const
    {
        return (pair.left << 4) ^ pair.right;
    }
};

using kerning_map
    = std::unordered_map<kerning_pair_index, float, kerning_pair_index_hash>;

using glyph_map = std::unordered_map<int, msdf_glyph>;

struct cached_glyph_data
{
    float uv_rect[4];
    float plane_rect[4];
};

struct msdf_text_engine
{
    alia_draw_system* system;

    msdf_gpu_data gpu;

    msdf_font_metrics metrics;
    msdf_atlas_description atlas;

    alia::glyph_map glyph_map;

    std::unordered_map<int, cached_glyph_data> glyph_cache;

    alia::kerning_map kerning_map;

    alia_draw_material_id material_id;
};

void
render_msdf_bucket(void* user, alia_draw_bucket const* bucket);

inline GLuint
create_quad_vbo()
{
    GLuint quad_vbo;
    glGenBuffers(1, &quad_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);

    float quad_vertices[] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    return quad_vbo;
}

inline GLuint
create_instance_vbo(uint32_t initial_capacity)
{
    GLuint instance_vbo;
    glGenBuffers(1, &instance_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);

    GLsizeiptr const buffer_size
        = sizeof(gpu_glyph_instance) * initial_capacity;
    glBufferData(GL_ARRAY_BUFFER, buffer_size, nullptr, GL_STREAM_DRAW);

    return instance_vbo;
}

inline GLuint
create_vertex_array(GLuint quad_vbo, GLuint instance_vbo)
{
    GLuint vao;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);

    // color (location = 1)
    glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(gpu_glyph_instance),
        (void*) offsetof(gpu_glyph_instance, color));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    // position (location = 2)
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(gpu_glyph_instance),
        (void*) offsetof(gpu_glyph_instance, position));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // scale (location = 3)
    glVertexAttribPointer(
        3,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(gpu_glyph_instance),
        (void*) offsetof(gpu_glyph_instance, scale));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // uv_rect (location = 4)
    glVertexAttribPointer(
        4,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(gpu_glyph_instance),
        (void*) offsetof(gpu_glyph_instance, uv_rect[0]));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // plane_rect (location = 5)
    glVertexAttribPointer(
        5,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(gpu_glyph_instance),
        (void*) offsetof(gpu_glyph_instance, plane_rect[0]));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    return vao;
}

GLuint
create_msdf_texture(char const* texture_atlas_path)
{
    GLuint texture;
    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);

    int atlas_width, atlas_height, channels;
    unsigned char* msdf_data = stbi_load(
        texture_atlas_path, &atlas_width, &atlas_height, &channels, 3);
    if (!msdf_data)
    {
        // TODO: Add proper error reporting.
        std::cerr << "load failed: " << stbi_failure_reason() << "\n";
        return 0;
    }

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB8,
        atlas_width,
        atlas_height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        msdf_data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(msdf_data);

    return texture;
}

msdf_text_engine*
create_msdf_text_engine(
    alia_draw_system* system,
    msdf_font_description const& font,
    char const* texture_atlas_path)
{
    GLuint texture = create_msdf_texture(texture_atlas_path);

    GLuint shader_program = create_shader_program(
        msdf_vertex_shader_source, msdf_fragment_shader_source);
    GLint matrix_location
        = glGetUniformLocation(shader_program, "u_projection");

    GLuint const quad_vbo = create_quad_vbo();
    uint32_t const initial_glyph_instance_capacity = 262'144; // 4096;
    GLuint const instance_vbo
        = create_instance_vbo(initial_glyph_instance_capacity);
    GLuint const vao = create_vertex_array(quad_vbo, instance_vbo);

    std::vector<gpu_glyph_instance> glyph_instances;
    glyph_instances.resize(initial_glyph_instance_capacity);

    kerning_map kerning_map;
    for (size_t i = 0; i < font.kerning_pair_count; ++i)
    {
        kerning_map[kerning_pair_index{
            font.kerning_pairs[i].left, font.kerning_pairs[i].right}]
            = font.kerning_pairs[i].adjustment;
    }

    glyph_map glyph_map;
    for (size_t i = 0; i < font.glyph_count; ++i)
    {
        glyph_map[font.glyphs[i].unicode] = font.glyphs[i];
    }

    // TODO: Don't use new here.
    msdf_text_engine* engine = new msdf_text_engine{
        .system = system,

        .gpu
        = {.shader_program = shader_program,
           .matrix_location = matrix_location,

           .texture = texture,

           .vao = vao,
           .quad_vbo = quad_vbo,
           .instance_vbo = instance_vbo,

           .glyph_instances = std::move(glyph_instances)},

        .metrics = font.metrics,
        .atlas = font.atlas,
        .glyph_map = std::move(glyph_map),
        .kerning_map = std::move(kerning_map),

        .material_id = 0};

    for (size_t i = 0; i < font.glyph_count; ++i)
    {
        auto& cached_glyph = engine->glyph_cache[font.glyphs[i].unicode];
        cached_glyph.uv_rect[0] = font.glyphs[i].atlas_left / font.atlas.width;
        cached_glyph.uv_rect[1]
            = 1.0f - (font.glyphs[i].atlas_top / font.atlas.height);
        cached_glyph.uv_rect[2]
            = (font.glyphs[i].atlas_right - font.glyphs[i].atlas_left)
            / font.atlas.width;
        cached_glyph.uv_rect[3]
            = (font.glyphs[i].atlas_top - font.glyphs[i].atlas_bottom)
            / font.atlas.height;
        cached_glyph.plane_rect[0] = font.glyphs[i].plane_left;
        cached_glyph.plane_rect[1] = -font.glyphs[i].plane_top;
        cached_glyph.plane_rect[2]
            = font.glyphs[i].plane_right - font.glyphs[i].plane_left;
        cached_glyph.plane_rect[3]
            = -font.glyphs[i].plane_bottom + font.glyphs[i].plane_top;
    }

    engine->material_id = alia_material_alloc_ids(system, 1);
    alia_material_register(
        system,
        engine->material_id,
        alia_material_vtable{
            .draw_bucket = render_msdf_bucket,
        },
        engine);

    return engine;
}

void
destroy_msdf_text_engine(msdf_text_engine* engine)
{
    glDeleteTextures(1, &engine->gpu.texture);
    glDeleteProgram(engine->gpu.shader_program);
    glDeleteBuffers(1, &engine->gpu.quad_vbo);
    glDeleteBuffers(1, &engine->gpu.instance_vbo);
    glDeleteVertexArrays(1, &engine->gpu.vao);

    delete engine;
}

msdf_font_metrics const*
get_msdf_font_metrics(msdf_text_engine* engine)
{
    return &engine->metrics;
}

float
get_kerning(msdf_text_engine* engine, uint32_t left, uint32_t right)
{
    auto it = engine->kerning_map.find(kerning_pair_index{left, right});
    if (it != engine->kerning_map.end())
    {
        return it->second;
    }
    return 0.0f;
}

float
measure_text_width(
    msdf_text_engine* engine, char const* text, size_t length, float font_size)
{
    float width = 0;
    for (size_t i = 0; i < length; ++i)
    {
        char const c = text[i];

        const msdf_glyph& glyph = engine->glyph_map[c];
        assert(glyph.unicode == c);

        if (i + 1 < length)
        {
            width += (glyph.advance + get_kerning(engine, c, text[i + 1]))
                   * font_size;
        }
        else
        {
            width += glyph.advance * font_size;
        }
    }
    return width;
}

void
draw_text(
    msdf_text_engine* engine,
    alia_draw_context* ctx,
    alia_z_index z_index,
    char const* text,
    size_t length,
    float scale,
    alia_vec2f position,
    alia_rgba color)
{
    auto* command = downcast<msdf_draw_command>(alia_draw_command_alloc(
        ctx,
        z_index,
        engine->material_id,
        alia_min_aligned_size(sizeof(msdf_draw_command) + length)));
    command->engine = engine;
    command->position
        = position + alia_vec2f{0, engine->metrics.ascender * scale};
    command->scale = scale;
    command->color = color;
    command->length = length;
    memcpy(command->text, text, length);
}

std::pair<size_t, float>
break_text(
    msdf_text_engine* engine,
    char const* text,
    size_t start,
    size_t end,
    size_t buffer_length,
    float scale,
    float width,
    bool force_break)
{
    size_t last_space = start;
    float x = 0;
    for (size_t i = start; i < end; ++i)
    {
        char const c = text[i];

        switch (c)
        {
            case '\r':
                continue;
            case '\n':
                return {i + 1, x};
            case ' ':
                last_space = i + 1;
                break;
        }

        const msdf_glyph& glyph = engine->glyph_map[c];
        assert(glyph.unicode == c);

        x += (glyph.advance + get_kerning(engine, c, text[i + 1])) * scale;
        if (x > width)
        {
            return {
                last_space == start && force_break ? (i == start ? i + 1 : i)
                                                   : last_space,
                x};
        }
    }
    return {end, x};
}

void
render_command(
    msdf_text_engine* engine,
    size_t& glyph_instance_count,
    msdf_draw_command const& command)
{
    // TODO: Culling.
    alia_vec2f position = command.position;
    float scale = command.scale;
    for (size_t i = 0; i < command.length; ++i)
    {
        char const c = command.text[i];
        if (c < 32)
            continue;
        uint32_t unicode = c;

        const msdf_glyph& glyph = engine->glyph_map[unicode];
        assert(glyph.unicode == unicode);

        auto& cached_glyph = engine->glyph_cache[unicode];

        engine->gpu.glyph_instances[glyph_instance_count] = {
            alia_rgba{0.9f, 0.9f, 0.9f, 1.0f},
            position,
            scale,
            cached_glyph.uv_rect[0],
            cached_glyph.uv_rect[1],
            cached_glyph.uv_rect[2],
            cached_glyph.uv_rect[3],
            cached_glyph.plane_rect[0],
            cached_glyph.plane_rect[1],
            cached_glyph.plane_rect[2],
            cached_glyph.plane_rect[3],
        };
        glyph_instance_count++;

        position.x += glyph.advance * scale;

        if (i + 1 < command.length)
            position.x += get_kerning(engine, c, command.text[i + 1]) * scale;
    }
}

void
render_msdf_bucket(void* user, alia_draw_bucket const* bucket)
{
    msdf_text_engine* engine = (msdf_text_engine*) user;
    alia_vec2f surface_size
        = {engine->system->surface_size.x, engine->system->surface_size.y};

    size_t glyph_instance_count = 0;
    for (alia_draw_command* cmd = bucket->head; cmd; cmd = cmd->next)
    {
        auto const* msdf_cmd = downcast<msdf_draw_command>(cmd);
        render_command(engine, glyph_instance_count, *msdf_cmd);
    }

    glBindBuffer(GL_ARRAY_BUFFER, engine->gpu.instance_vbo);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        glyph_instance_count * sizeof(gpu_glyph_instance),
        engine->gpu.glyph_instances.data());

    glBindTexture(GL_TEXTURE_2D, engine->gpu.texture);

    glUseProgram(engine->gpu.shader_program);

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

    glUniformMatrix4fv(engine->gpu.matrix_location, 1, GL_FALSE, ortho);

    glBindVertexArray(engine->gpu.vao);

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, glyph_instance_count);
}

} // namespace alia
