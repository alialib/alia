#pragma once

#include <alia/ui/geometry.hpp>

#include <vector>

namespace alia {

struct MsdfTextEngine;

struct MsdfFontMetrics
{
    float em_size;
    float line_height;
    float ascender;
    float descender;
    float underline_y;
    float underline_thickness;
};

struct MsdfAtlasDescription
{
    float distance_range;
    float distance_range_middle;
    float font_size;
    float width;
    float height;
};

struct MsdfGlyph
{
    uint32_t unicode;
    float advance;
    bool visible;
    float plane_left, plane_bottom, plane_right, plane_top;
    float atlas_left, atlas_bottom, atlas_right, atlas_top;
};

struct MsdfKerningPair
{
    uint32_t left;
    uint32_t right;
    float adjustment;
};

struct MsdfFontDescription
{
    MsdfFontMetrics metrics;
    MsdfAtlasDescription atlas;

    MsdfGlyph const* glyphs;
    size_t glyph_count;

    MsdfKerningPair const* kerning_pairs;
    size_t kerning_pair_count;
};

// TODO: Support multiple fonts within a single engine.
// TODO: Don't assume that the atlas will be loaded from a file.
MsdfTextEngine*
create_msdf_text_engine(
    MsdfFontDescription const& font_description,
    char const* texture_atlas_path);

void
destroy_msdf_text_engine(MsdfTextEngine* engine);

float
render_text(
    MsdfTextEngine* engine,
    char const* text,
    size_t start,
    size_t end,
    size_t length,
    float scale,
    float x,
    float y);

float
render_wrapped_text(
    MsdfTextEngine* engine,
    char const* text,
    float scale,
    float x,
    float y,
    float width);

void
start_render_pass(MsdfTextEngine* engine);

void
end_render_pass(MsdfTextEngine* engine, Vec2 framebuffer_size);

} // namespace alia
