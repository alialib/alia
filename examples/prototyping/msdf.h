#pragma once

// Prototype MSDF text layout (`do_text`). The live path is `alia_text` in
// <alia/abi/ui/text.h>. Kept for optional layout experiments / shader gallery.

#include <alia/abi/ui/layout/utilities/flow.h>

struct text_layout_placement_header
{
    int fragment_count;
};

struct text_layout_placement_fragment
{
    alia_vec2f position;
    alia_vec2f size;
    char const* text;
    size_t length;
    size_t font_index;
};

struct msdf_text_layout_node
{
    alia_layout_node base;
    alia_layout_flags_t flags;
    float spacing;
    alia_msdf_text_engine* engine;
    size_t font_index;
    char const* text;
    float font_size;
};

static text_layout_placement_header&
msdf_consume_text_placement_header(context& ctx)
{
    return *arena_alloc<text_layout_placement_header>(
        *alia_layout_placement_arena(&ctx));
}

static text_layout_placement_fragment&
msdf_consume_text_placement_fragment(context& ctx)
{
    return *arena_alloc<text_layout_placement_fragment>(
        *alia_layout_placement_arena(&ctx));
}

alia_horizontal_requirements
measure_text_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& text = *reinterpret_cast<msdf_text_layout_node*>(node);
    float width = alia_msdf_measure_text_width(
        text.engine,
        text.font_index,
        text.text,
        strlen(text.text),
        text.font_size);
    return alia_horizontal_requirements{
        .min_size = width + text.spacing * 2, .growth_factor = 0};
}

alia_vertical_requirements
measure_text_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& text = *reinterpret_cast<msdf_text_layout_node*>(node);
    auto const* metrics
        = alia_msdf_get_font_metrics(text.engine, text.font_index);
    return alia_vertical_requirements{
        .min_size = metrics->line_height * text.font_size + text.spacing * 2,
        .growth_factor = 0,
        .ascent = (text.flags & ALIA_Y_ALIGNMENT_MASK) == ALIA_BASELINE_Y
                    ? metrics->ascender * text.font_size + text.spacing
                    : 0.0f,
        .descent = (text.flags & ALIA_Y_ALIGNMENT_MASK) == ALIA_BASELINE_Y
                     ? -metrics->descender * text.font_size + text.spacing
                     : 0.0f};
}

struct text_line_metrics
{
    float height;
    float ascent;
    float descent;
};

enum class text_token_kind
{
    content,
    spacer,
    break_token,
};

static bool
text_is_space(char c)
{
    return c == ' ' || c == '\t';
}

static text_line_metrics
text_font_line_metrics(
    msdf_text_layout_node const& text, alia_msdf_font_metrics const* metrics)
{
    return {
        .height = metrics->line_height * text.font_size,
        .ascent = metrics->ascender * text.font_size,
        .descent = -metrics->descender * text.font_size};
}

static alia_line_requirements
text_line_requirements(
    msdf_text_layout_node const& text, alia_msdf_font_metrics const* metrics)
{
    auto const line = text_font_line_metrics(text, metrics);
    return {
        .height = line.height, .ascent = line.ascent, .descent = line.descent};
}

static alia_line_requirements
text_emit_line_requirements(
    msdf_text_layout_node const& text,
    alia_msdf_font_metrics const* metrics,
    alia_flow_fragment_emitter const* emitter)
{
    return alia_layout_line_requirements_with_run_offsets(
        emitter, text_line_requirements(text, metrics));
}

template<class F>
static void
text_for_each_token(char const* text, size_t length, F&& f)
{
    size_t i = 0;
    while (i < length)
    {
        if (text[i] == '\r')
        {
            f(text_token_kind::break_token, i, 0);
            ++i;
            if (i < length && text[i] == '\n')
                ++i;
            continue;
        }
        if (text[i] == '\n')
        {
            f(text_token_kind::break_token, i, 0);
            ++i;
            continue;
        }

        if (text_is_space(text[i]))
        {
            size_t const start = i;
            while (i < length && text_is_space(text[i]))
                ++i;
            f(text_token_kind::spacer, start, i - start);
            continue;
        }

        size_t const start = i;
        while (i < length && !text_is_space(text[i]) && text[i] != '\n'
               && text[i] != '\r')
            ++i;
        f(text_token_kind::content, start, i - start);
    }
}

static int
text_count_tokens(char const* text, size_t length)
{
    int count = 0;
    text_for_each_token(
        text, length, [&](text_token_kind, size_t, size_t) { ++count; });
    return count;
}

static float
text_measure_substring_width(
    msdf_text_layout_node const& text, size_t start, size_t length)
{
    return alia_msdf_measure_text_width(
        text.engine,
        text.font_index,
        text.text + start,
        length,
        text.font_size);
}

static void
text_emit_token(
    msdf_text_layout_node const& text,
    alia_msdf_font_metrics const* metrics,
    alia_flow_fragment_emitter* emitter,
    text_token_kind kind,
    size_t start,
    size_t length)
{
    auto const line = text_emit_line_requirements(text, metrics, emitter);
    switch (kind)
    {
        case text_token_kind::content:
            alia_layout_emit_flow_fragment_raw(
                emitter,
                alia_flow_fragment{
                    .flags = 0,
                    .kind = ALIA_FLOW_FRAGMENT_KIND_CONTENT,
                    .content
                    = {.size = alia_vec2f_make(
                           text_measure_substring_width(
                               text, start, length),
                           line.height),
                       .ascent = line.ascent,
                       .descent = line.descent}});
            break;
        case text_token_kind::spacer: {
            float const spacer_width
                = text_measure_substring_width(text, start, length);
            alia_layout_emit_flow_fragment_raw(
                emitter,
                alia_flow_fragment{
                    .flags = ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_START
                           | ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_END,
                    .kind = ALIA_FLOW_FRAGMENT_KIND_CONTENT,
                    .content
                    = {.size = alia_vec2f_make(spacer_width, line.height),
                       .ascent = line.ascent,
                       .descent = line.descent}});
            break;
        }
        case text_token_kind::break_token:
            alia_layout_emit_flow_fragment_raw(
                emitter,
                alia_flow_fragment{
                    .flags = ALIA_FLOW_FRAGMENT_BREAK_AFTER
                           | ALIA_FLOW_FRAGMENT_OMIT_FROM_BOUNDS,
                    .kind = ALIA_FLOW_FRAGMENT_KIND_CONTENT,
                    .content
                    = {.size = alia_vec2f_make(0.f, line.height),
                       .ascent = line.ascent,
                       .descent = line.descent}});
            break;
    }
}

static void
text_write_placement_fragment(
    alia_placement_context* ctx,
    msdf_text_layout_node const& text,
    alia_msdf_font_metrics const* metrics,
    alia_flow_fragment_placement const* placement,
    alia_flow_fragment const* spec,
    size_t start,
    size_t length,
    text_layout_placement_fragment* fragment)
{
    (void) ctx;
    auto const base = text_line_requirements(text, metrics);
    fragment->position
        = {placement->position.x,
           placement->position.y + placement->baseline
               - metrics->ascender * text.font_size};
    fragment->size = {alia_flow_fragment_content(spec)->size.x, base.height};
    fragment->text = text.text + start;
    fragment->length = length;
    fragment->font_index = text.font_index;
}

static void
text_place_whole_string(
    alia_placement_context* ctx,
    msdf_text_layout_node& text,
    alia_msdf_font_metrics const* metrics,
    alia_main_axis_index main_axis,
    alia_box box,
    float baseline,
    float width)
{
    auto const placement = alia_resolve_leaf_box(
        alia_fold_in_cross_axis_flags(text.flags, main_axis),
        box.size,
        baseline,
        alia_vec2f{width, metrics->line_height * text.font_size},
        metrics->ascender * text.font_size,
        {text.spacing, text.spacing});

    text_layout_placement_header* header
        = arena_alloc<text_layout_placement_header>(ctx->arena);
    header->fragment_count = 1;

    text_layout_placement_fragment* fragment
        = arena_alloc<text_layout_placement_fragment>(ctx->arena);
    fragment->position = box.min + placement.min;
    fragment->size = placement.size;
    fragment->text = text.text;
    fragment->length = strlen(text.text);
    fragment->font_index = text.font_index;
}

void
assign_text_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& text = *reinterpret_cast<msdf_text_layout_node*>(node);
    auto const* metrics
        = alia_msdf_get_font_metrics(text.engine, text.font_index);

    // TODO: Don't repeatedly measure the text width.
    float const width = alia_msdf_measure_text_width(
        text.engine,
        text.font_index,
        text.text,
        strlen(text.text),
        text.font_size);

    text_place_whole_string(
        ctx, text, metrics, main_axis, box, baseline, width);
}

static int
text_count_fragments(char const* text, size_t length)
{
    return text_count_tokens(text, length);
}

alia_flow_emission_counts
text_count_flow_emissions(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& text = *reinterpret_cast<msdf_text_layout_node*>(node);
    (void) ctx;
    return alia_flow_emission_counts_with_run_scope({
        .fragment_count = text_count_fragments(text.text, strlen(text.text))});
}

void
text_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter)
{
    auto& text = *reinterpret_cast<msdf_text_layout_node*>(node);
    (void) ctx;
    auto const* metrics
        = alia_msdf_get_font_metrics(text.engine, text.font_index);
    alia_flow_emit_run_push_control(
        emitter,
        alia_edge_offsets{.left = text.spacing, .right = text.spacing});
    size_t const length = strlen(text.text);
    text_for_each_token(
        text.text,
        length,
        [&](text_token_kind kind, size_t start, size_t len) {
            text_emit_token(text, metrics, emitter, kind, start, len);
        });
    alia_flow_emit_run_pop_control(emitter);
}

static void
text_advance_flow_fragment(alia_flow_fragment_reader* reader)
{
    (void) alia_layout_read_fragment_spec(reader);
    (void) alia_layout_read_fragment_placement(reader);
    alia_layout_advance_fragment(reader);
}

void
text_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader)
{
    auto& text = *reinterpret_cast<msdf_text_layout_node*>(node);
    auto const* metrics
        = alia_msdf_get_font_metrics(text.engine, text.font_index);

    alia_layout_skip_flow_run_push_fragment(reader);

    text_layout_placement_header* header
        = arena_alloc<text_layout_placement_header>(ctx->arena);
    header->fragment_count = 0;

    size_t const length = strlen(text.text);
    text_for_each_token(
        text.text,
        length,
        [&](text_token_kind kind, size_t start, size_t token_length) {
            switch (kind)
            {
                case text_token_kind::content: {
                    auto const* spec = alia_layout_read_fragment_spec(reader);
                    auto const* placement
                        = alia_layout_read_fragment_placement(reader);
                    alia_layout_advance_fragment(reader);
                    if (!(placement->flags
                          & ALIA_FLOW_FRAGMENT_PLACEMENT_SUPPRESSED))
                    {
                        text_layout_placement_fragment* fragment
                            = arena_alloc<text_layout_placement_fragment>(
                                ctx->arena);
                        text_write_placement_fragment(
                            ctx,
                            text,
                            metrics,
                            placement,
                            spec,
                            start,
                            token_length,
                            fragment);
                        ++header->fragment_count;
                    }
                    break;
                }
                case text_token_kind::spacer:
                case text_token_kind::break_token:
                    text_advance_flow_fragment(reader);
                    break;
            }
        });

    alia_layout_skip_flow_run_pop_fragment(reader);
}

alia_layout_node_vtable text_layout_vtable
    = {measure_text_horizontal,
       measure_text_vertical,
       assign_text_boxes,
       text_count_flow_emissions,
       text_emit_flow_fragments,
       text_read_fragment_placements};

// `scale` is logical font size in design pixels; `do_text` applies
// ctx.geometry->scale once.
bool
do_text(
    context& ctx,
    alia_z_index z_index,
    alia_srgba8 color,
    float scale,
    char const* text,
    layout_flag_set flags = NO_FLAGS,
    size_t font_index = 0)
{
    bool result = false;
    // TODO: Should this have its own ID?
    alia_element_id id = alia_element_get_identity(&ctx);
    switch (get_event_category(ctx))
    {
        case ALIA_CATEGORY_REFRESH: {
            auto& emission = ctx.layout->emission;
            msdf_text_layout_node* new_node
                = arena_alloc<msdf_text_layout_node>(emission.arena);
            new_node->base.vtable = &text_layout_vtable;
            new_node->base.next_sibling = nullptr;
            new_node->flags = raw_code(flags);
            new_node->text = text;
            new_node->font_size = scale * ctx.geometry->scale;
            new_node->engine = the_msdf_text_engine;
            new_node->font_index = font_index;
            new_node->spacing = ctx.style->spacing;
            *emission.next_ptr = &new_node->base;
            emission.next_ptr = &new_node->base.next_sibling;
            break;
        }
        case ALIA_CATEGORY_SPATIAL: {
            auto& text_placement = msdf_consume_text_placement_header(ctx);
            for (int i = 0; i < text_placement.fragment_count; ++i)
            {
                auto& fragment = msdf_consume_text_placement_fragment(ctx);
                alia_box box
                    = {.min = fragment.position, .size = fragment.size};
                alia_element_box_region(
                    &ctx, id, &box, ALIA_CURSOR_DEFAULT, ALIA_HIT_TEST_MOUSE);
            }
            break;
        }
        case ALIA_CATEGORY_DRAWING: {
            auto& text_placement = msdf_consume_text_placement_header(ctx);
            for (int i = 0; i < text_placement.fragment_count; ++i)
            {
                auto& fragment = msdf_consume_text_placement_fragment(ctx);
                alia_msdf_draw_text(
                    the_msdf_text_engine,
                    &ctx,
                    z_index,
                    fragment.text,
                    fragment.length,
                    scale * ctx.geometry->scale,
                    fragment.position,
                    color,
                    fragment.font_index);
            }
            break;
        }
        default:
        case ALIA_CATEGORY_INPUT: {
            auto& text_placement = msdf_consume_text_placement_header(ctx);
            for (int i = 0; i < text_placement.fragment_count; ++i)
            {
                auto& fragment = msdf_consume_text_placement_fragment(ctx);
                alia_box box
                    = {.min = fragment.position, .size = fragment.size};
                if (alia_element_detect_click(&ctx, id, ALIA_BUTTON_LEFT))
                {
                    // TODO: Perform action, abort traversal.
                    result = true;
                }
            }
            break;
        }
    }
    return result;
}
