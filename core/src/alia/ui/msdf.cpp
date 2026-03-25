#include <alia/abi/ui/msdf.h>

#include <alia/abi/base/geometry/vec2.h>
#include <alia/abi/prelude.h>

#include <cassert>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace {

struct kerning_pair_index
{
    uint32_t left;
    uint32_t right;

    bool
    operator==(kerning_pair_index const& other) const
    {
        return left == other.left && right == other.right;
    }
};

struct kerning_pair_index_hash
{
    std::size_t
    operator()(kerning_pair_index const& pair) const
    {
        return (pair.left << 4) ^ pair.right;
    }
};

using kerning_lookup
    = std::unordered_map<kerning_pair_index, float, kerning_pair_index_hash>;

using glyph_lookup = std::unordered_map<int, alia_msdf_glyph>;

struct cached_glyph_data
{
    float uv_rect[4];
};

struct msdf_font_data
{
    alia_msdf_font_metrics metrics;
    glyph_lookup glyphs;
    std::unordered_map<int, cached_glyph_data> glyph_cache;
    kerning_lookup kerning;
};

static inline alia_msdf_glyph const&
require_glyph(msdf_font_data const& font, uint32_t unicode)
{
    alia_msdf_glyph const& glyph = font.glyphs.at(static_cast<int>(unicode));
    assert(glyph.unicode == unicode);
    return glyph;
}

static inline void
emit_glyph_draw_command(
    alia_context* ctx,
    alia_z_index z_index,
    alia_msdf_glyph const& glyph,
    cached_glyph_data const& cached_glyph,
    float sdf_scale,
    alia_vec2f cursor,
    float scale,
    alia_srgba8 color)
{
    alia_box box;
    box.min.x = cursor.x + glyph.plane_left * scale;
    box.min.y = cursor.y - glyph.plane_top * scale;
    box.size.x = (glyph.plane_right - glyph.plane_left) * scale;
    box.size.y = (glyph.plane_top - glyph.plane_bottom) * scale;

    auto* command = reinterpret_cast<alia_draw_primitive_command*>(
        alia_draw_command_alloc(
            ctx,
            z_index,
            ALIA_PRIMITIVE_MATERIAL_ID,
            ALIA_MIN_ALIGNED_SIZE(sizeof(alia_draw_primitive_command))));

    command->box = box;
    command->primitive_type = ALIA_PRIMITIVE_MSDF_GLYPH;
    command->color = color;
    std::memcpy(
        command->payload.msdf_glyph.uv_rect,
        cached_glyph.uv_rect,
        sizeof(cached_glyph.uv_rect));
    command->payload.msdf_glyph.sdf_scale = sdf_scale;
}

float
get_kerning(msdf_font_data const& font, uint32_t left, uint32_t right)
{
    auto it = font.kerning.find(kerning_pair_index{left, right});
    if (it != font.kerning.end())
        return it->second;
    return 0.0f;
}

} // namespace

struct alia_msdf_text_engine
{
    alia_msdf_atlas_description atlas;
    std::vector<msdf_font_data> fonts;
};

extern "C" void
alia_msdf_atlas_rle_decompress(
    uint8_t const* rle_r,
    size_t rle_r_size,
    uint8_t const* rle_g,
    size_t rle_g_size,
    uint8_t const* rle_b,
    size_t rle_b_size,
    uint8_t* out_rgb,
    size_t out_size)
{
    auto decode_channel
        = [](uint8_t const* rle,
             size_t rle_size,
             uint8_t* out,
             size_t out_count,
             int stride) {
              size_t out_pos = 0;
              size_t i = 0;
              while (i < rle_size && out_pos < out_count)
              {
                  uint8_t v = rle[i++];
                  if (v == 0x00 || v == 0xff)
                  {
                      if (i >= rle_size)
                          break;
                      uint8_t run = rle[i++];
                      for (uint8_t k = 0; k < run && out_pos < out_count;
                           ++k, ++out_pos)
                          out[out_pos * stride] = v;
                  }
                  else
                  {
                      out[out_pos * stride] = v;
                      ++out_pos;
                  }
              }
          };
    size_t plane_size = out_size / 3;
    decode_channel(rle_r, rle_r_size, out_rgb + 0, plane_size, 3);
    decode_channel(rle_g, rle_g_size, out_rgb + 1, plane_size, 3);
    decode_channel(rle_b, rle_b_size, out_rgb + 2, plane_size, 3);
}

extern "C" alia_msdf_text_engine*
alia_msdf_create_text_engine(
    alia_msdf_font_description const* font_descriptions, size_t font_count)
{
    auto* ctx = new alia_msdf_text_engine{};
    ctx->atlas = font_count > 0 ? font_descriptions[0].atlas
                                : alia_msdf_atlas_description{};

    ctx->fonts.reserve(font_count);
    for (size_t f = 0; f < font_count; ++f)
    {
        alia_msdf_font_description const& font = font_descriptions[f];
        msdf_font_data fd;
        fd.metrics = font.metrics;
        for (size_t i = 0; i < font.kerning_pair_count; ++i)
        {
            fd.kerning[kerning_pair_index{
                font.kerning_pairs[i].left, font.kerning_pairs[i].right}]
                = font.kerning_pairs[i].adjustment;
        }
        for (size_t i = 0; i < font.glyph_count; ++i)
        {
            fd.glyphs[font.glyphs[i].unicode] = font.glyphs[i];
        }
        for (size_t i = 0; i < font.glyph_count; ++i)
        {
            auto& cached_glyph = fd.glyph_cache[font.glyphs[i].unicode];
            cached_glyph.uv_rect[0]
                = font.glyphs[i].atlas_left / font.atlas.width;
            cached_glyph.uv_rect[1]
                = font.glyphs[i].atlas_bottom / font.atlas.height;
            cached_glyph.uv_rect[2]
                = (font.glyphs[i].atlas_right - font.glyphs[i].atlas_left)
                / font.atlas.width;
            cached_glyph.uv_rect[3]
                = (font.glyphs[i].atlas_top - font.glyphs[i].atlas_bottom)
                / font.atlas.height;
        }
        ctx->fonts.push_back(std::move(fd));
    }
    return ctx;
}

extern "C" void
alia_msdf_destroy_text_engine(alia_msdf_text_engine* ctx)
{
    delete ctx;
}

extern "C" alia_msdf_font_metrics const*
alia_msdf_get_font_metrics(alia_msdf_text_engine* ctx, size_t font_index)
{
    return &ctx->fonts[font_index].metrics;
}

extern "C" float
alia_msdf_measure_text_width(
    alia_msdf_text_engine* ctx,
    size_t font_index,
    char const* text,
    size_t length,
    float font_size)
{
    auto& glyphs = ctx->fonts[font_index].glyphs;
    float width = 0;
    for (size_t i = 0; i < length; ++i)
    {
        char const c = text[i];

        alia_msdf_glyph const& glyph = glyphs.at(static_cast<int>(c));
        assert(glyph.unicode == static_cast<uint32_t>(c));

        if (i + 1 < length)
        {
            width += (glyph.advance
                      + get_kerning(ctx->fonts[font_index], c, text[i + 1]))
                   * font_size;
        }
        else
        {
            width += glyph.advance * font_size;
        }
    }
    return width;
}

extern "C" float
alia_msdf_measure_codepoint_width(
    alia_msdf_text_engine* ctx,
    size_t font_index,
    uint32_t codepoint,
    float font_size)
{
    msdf_font_data const& font = ctx->fonts[font_index];
    alia_msdf_glyph const& glyph = require_glyph(font, codepoint);
    return glyph.advance * font_size;
}

extern "C" alia_msdf_break_result
alia_msdf_break_text(
    alia_msdf_text_engine* ctx,
    size_t font_index,
    char const* text,
    size_t start,
    size_t end,
    size_t buffer_length,
    float scale,
    float width,
    bool force_break)
{
    (void) buffer_length;
    auto& glyphs = ctx->fonts[font_index].glyphs;
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

        alia_msdf_glyph const& glyph = glyphs.at(static_cast<int>(c));
        assert(glyph.unicode == static_cast<uint32_t>(c));

        float kern = 0;
        if (i + 1 < end)
            kern = get_kerning(ctx->fonts[font_index], c, text[i + 1]);

        x += (glyph.advance + kern) * scale;
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

extern "C" void
alia_msdf_draw_text(
    alia_msdf_text_engine* fonts,
    alia_context* ctx,
    alia_z_index z_index,
    char const* text,
    size_t length,
    float scale,
    alia_vec2f position,
    alia_srgba8 color,
    size_t font_index)
{
    msdf_font_data& font = fonts->fonts[font_index];
    alia_msdf_atlas_description const& atlas = fonts->atlas;

    alia_vec2f cursor = alia_vec2f_add(
        alia_vec2f_add(position, ctx->geometry->offset),
        alia_vec2f{0, font.metrics.ascender * scale});

    float const sdf_scale = scale * atlas.distance_range / atlas.font_size;

    for (size_t i = 0; i < length; ++i)
    {
        char const c = text[i];
        if (c < 32)
            continue;

        uint32_t unicode = static_cast<uint32_t>(c);
        alia_msdf_glyph const& glyph = require_glyph(font, unicode);
        auto const& cached_glyph = font.glyph_cache.at(static_cast<int>(unicode));
        emit_glyph_draw_command(
            ctx, z_index, glyph, cached_glyph, sdf_scale, cursor, scale, color);

        cursor.x += glyph.advance * scale;
        if (i + 1 < length)
            cursor.x += get_kerning(font, c, text[i + 1]) * scale;
    }
}

extern "C" void
alia_msdf_draw_codepoint(
    alia_msdf_text_engine* fonts,
    alia_context* ctx,
    alia_z_index z_index,
    uint32_t codepoint,
    float scale,
    alia_vec2f position,
    alia_srgba8 color,
    size_t font_index)
{
    msdf_font_data& font = fonts->fonts[font_index];
    alia_msdf_atlas_description const& atlas = fonts->atlas;
    alia_msdf_glyph const& glyph = require_glyph(font, codepoint);
    auto const& cached_glyph = font.glyph_cache.at(static_cast<int>(codepoint));

    alia_vec2f cursor = alia_vec2f_add(
        alia_vec2f_add(position, ctx->geometry->offset),
        alia_vec2f{0, font.metrics.ascender * scale});
    float const sdf_scale = scale * atlas.distance_range / atlas.font_size;

    emit_glyph_draw_command(
        ctx, z_index, glyph, cached_glyph, sdf_scale, cursor, scale, color);
}
