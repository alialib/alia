#ifndef ALIA_ABI_UI_MSDF_H
#define ALIA_ABI_UI_MSDF_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_msdf_text_engine alia_msdf_text_engine;

typedef struct alia_msdf_font_metrics
{
    float em_size;
    float line_height;
    float ascender;
    float descender;
    float underline_y;
    float underline_thickness;
} alia_msdf_font_metrics;

typedef struct alia_msdf_atlas_description
{
    float distance_range;
    float distance_range_middle;
    float font_size;
    float width;
    float height;
} alia_msdf_atlas_description;

typedef struct alia_msdf_glyph
{
    uint32_t unicode;
    float advance;
    bool visible;
    float plane_left, plane_bottom, plane_right, plane_top;
    float atlas_left, atlas_bottom, atlas_right, atlas_top;
} alia_msdf_glyph;

typedef struct alia_msdf_kerning_pair
{
    uint32_t left;
    uint32_t right;
    float adjustment;
} alia_msdf_kerning_pair;

typedef struct alia_msdf_font_description
{
    alia_msdf_font_metrics metrics;
    alia_msdf_atlas_description atlas;

    alia_msdf_glyph const* glyphs;
    size_t glyph_count;

    alia_msdf_kerning_pair const* kerning_pairs;
    size_t kerning_pair_count;
} alia_msdf_font_description;

typedef struct alia_msdf_break_result
{
    size_t next;
    float width;
} alia_msdf_break_result;

void
alia_msdf_atlas_rle_decompress(
    uint8_t const* rle_r,
    size_t rle_r_size,
    uint8_t const* rle_g,
    size_t rle_g_size,
    uint8_t const* rle_b,
    size_t rle_b_size,
    uint8_t* out_rgb,
    size_t out_size);

alia_msdf_text_engine*
alia_msdf_create_text_engine(
    alia_msdf_font_description const* font_descriptions, size_t font_count);

void
alia_msdf_destroy_text_engine(alia_msdf_text_engine* ctx);

alia_msdf_font_metrics const*
alia_msdf_get_font_metrics(alia_msdf_text_engine* ctx, size_t font_index);

float
alia_msdf_measure_text_width(
    alia_msdf_text_engine* ctx,
    size_t font_index,
    char const* text,
    size_t length,
    float font_size);

float
alia_msdf_measure_codepoint_width(
    alia_msdf_text_engine* ctx,
    size_t font_index,
    uint32_t codepoint,
    float font_size);

alia_msdf_break_result
alia_msdf_break_text(
    alia_msdf_text_engine* ctx,
    size_t font_index,
    char const* text,
    size_t start,
    size_t end,
    size_t buffer_length,
    float scale,
    float width,
    bool force_break);

void
alia_msdf_draw_text(
    alia_msdf_text_engine* fonts,
    alia_context* ctx,
    alia_z_index z_index,
    char const* text,
    size_t length,
    float scale,
    alia_vec2f position,
    alia_srgba8 color,
    size_t font_index);

void
alia_msdf_draw_codepoint(
    alia_msdf_text_engine* fonts,
    alia_context* ctx,
    alia_z_index z_index,
    uint32_t codepoint,
    float scale,
    alia_vec2f position,
    alia_srgba8 color,
    size_t font_index);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_MSDF_H */
