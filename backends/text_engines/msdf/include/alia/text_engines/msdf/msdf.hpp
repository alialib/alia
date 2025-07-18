#pragma once

#include <cstdint>
#include <utility>

#include <alia/ui/color.hpp>
#include <alia/ui/display_list.hpp>
#include <alia/ui/geometry.hpp>

// TODO: Split up this file.

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
    std::uint32_t unicode;
    float advance;
    bool visible;
    float plane_left, plane_bottom, plane_right, plane_top;
    float atlas_left, atlas_bottom, atlas_right, atlas_top;
};

struct MsdfKerningPair
{
    std::uint32_t left;
    std::uint32_t right;
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

// TODO: Don't assume that the atlas will be loaded from a file.
MsdfTextEngine*
create_msdf_text_engine(
    MsdfFontDescription const& font_description,
    char const* texture_atlas_path);

void
destroy_msdf_text_engine(MsdfTextEngine* engine);

MsdfFontMetrics const*
get_msdf_font_metrics(MsdfTextEngine* engine);

float
measure_text_width(
    MsdfTextEngine* engine, char const* text, size_t length, float font_size);

std::pair<size_t, float>
break_text(
    MsdfTextEngine* engine,
    char const* text,
    size_t start,
    size_t end,
    size_t buffer_length,
    float scale,
    float width,
    bool force_break);

struct MsdfDrawCommand
{
    MsdfTextEngine* engine;
    MsdfDrawCommand* next = nullptr;
    Vec2 position;
    float scale;
    Color color;
    size_t length;
    char text[];
};

void
draw_text(
    MsdfTextEngine* engine,
    DisplayListArena& arena,
    CommandList<MsdfDrawCommand>& commands,
    char const* text,
    size_t length,
    float scale,
    Vec2 position,
    Color color);

void
render_command_list(
    MsdfTextEngine* engine,
    CommandList<MsdfDrawCommand> const& commands,
    Vec2 framebuffer_size);

} // namespace alia
