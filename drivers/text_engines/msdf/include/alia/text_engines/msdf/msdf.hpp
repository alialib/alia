#pragma once

#include <cstdint>
#include <utility>

#include <alia/abi/ui/drawing.h>
#include <alia/color.hpp>
#include <alia/geometry.hpp>

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

// TODO: Don't assume that the atlas will be loaded from a file.
msdf_text_engine*
create_msdf_text_engine(
    alia_draw_system* system,
    msdf_font_description const& font_description,
    char const* texture_atlas_path);

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
