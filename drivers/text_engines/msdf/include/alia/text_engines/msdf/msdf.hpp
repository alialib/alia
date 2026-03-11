#pragma once

#include <cstdint>
#include <utility>

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/drawing.h>

// TODO: Split up this file.

namespace alia {

struct msdf_text_engine;

struct msdf_font_metrics
{
    float em_size;
    float line_height;
    float ascender;
    float descender;
    float underline_y;
    float underline_thickness;
};

struct msdf_atlas_description
{
    float distance_range;
    float distance_range_middle;
    float font_size;
    float width;
    float height;
};

struct msdf_glyph
{
    std::uint32_t unicode;
    float advance;
    bool visible;
    float plane_left, plane_bottom, plane_right, plane_top;
    float atlas_left, atlas_bottom, atlas_right, atlas_top;
};

struct msdf_kerning_pair
{
    std::uint32_t left;
    std::uint32_t right;
    float adjustment;
};

struct msdf_font_description
{
    msdf_font_metrics metrics;
    msdf_atlas_description atlas;

    msdf_glyph const* glyphs;
    size_t glyph_count;

    msdf_kerning_pair const* kerning_pairs;
    size_t kerning_pair_count;
};

// Creates engine from an in-memory RGB atlas.
msdf_text_engine*
create_msdf_text_engine(
    alia_ui_system* ui,
    // TODO: Don't have a separate draw system object.
    alia_draw_system* draw_system,
    msdf_font_description const& font_description,
    std::uint8_t const* atlas_rgb,
    int width,
    int height);

// Decompress per-channel RLE into interleaved RGB.
// NOTE:
// - In the this RLE format, only 0x00 and 0xff are run-length encoded as
//   `(value_byte, run_length_byte)`. Any other byte is a raw value.
// - `out_size` is the full size of the output buffer
//   (i.e., `width * height * 3`).
void
alia_msdf_atlas_rle_decompress(
    std::uint8_t const* rle_r,
    std::size_t rle_r_size,
    std::uint8_t const* rle_g,
    std::size_t rle_g_size,
    std::uint8_t const* rle_b,
    std::size_t rle_b_size,
    std::uint8_t* out_rgb,
    std::size_t out_size);

void
destroy_msdf_text_engine(msdf_text_engine* engine);

msdf_font_metrics const*
get_msdf_font_metrics(msdf_text_engine* engine);

float
measure_text_width(
    msdf_text_engine* engine,
    char const* text,
    size_t length,
    float font_size);

std::pair<size_t, float>
break_text(
    msdf_text_engine* engine,
    char const* text,
    size_t start,
    size_t end,
    size_t buffer_length,
    float scale,
    float width,
    bool force_break);

struct msdf_draw_command
{
    alia_draw_command base;
    msdf_text_engine* engine;
    alia_vec2f position;
    float scale;
    alia_rgba color;
    size_t length;
    char text[];
};

void
draw_text(
    msdf_text_engine* engine,
    alia_draw_context* ctx,
    alia_z_index z_index,
    char const* text,
    size_t length,
    float scale,
    alia_vec2f position,
    alia_rgba color);

} // namespace alia
