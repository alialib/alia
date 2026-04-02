#pragma once

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

void
assign_text_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    // TODO: Implement
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
    float width = alia_msdf_measure_text_width(
        text.engine,
        text.font_index,
        text.text,
        strlen(text.text),
        text.font_size);

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

alia_horizontal_requirements
measure_text_wrapped_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    return alia_horizontal_requirements{0, 0};
}

alia_wrapping_requirements
measure_text_wrapped_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float current_x_offset,
    float line_width)
{
    auto& text = *reinterpret_cast<msdf_text_layout_node*>(node);
    auto const* metrics
        = alia_msdf_get_font_metrics(text.engine, text.font_index);

    size_t length = strlen(text.text);

    alia_wrapping_requirements requirements;

    auto first_break = alia_msdf_break_text(
        text.engine,
        text.font_index,
        text.text,
        0,
        length,
        length,
        text.font_size,
        line_width - current_x_offset - text.spacing * 2,
        current_x_offset == 0);

    // If there is content on the first line, then we need to assign the
    // vertical requirements for it.
    if (first_break.next != 0)
    {
        requirements.first_line = {
            .height = metrics->line_height * text.font_size + text.spacing * 2,
            .ascent = metrics->ascender * text.font_size + text.spacing,
            .descent = -metrics->descender * text.font_size + text.spacing};
    }
    else
    {
        requirements.first_line = {.height = 0, .ascent = 0, .descent = 0};
    }

    // If everything fits on the first line, then we're done.
    if (first_break.next == length)
    {
        requirements.interior_height = 0;
        requirements.last_line = {.height = 0, .ascent = 0, .descent = 0};
        requirements.end_x
            = current_x_offset + first_break.width + text.spacing * 2;
        return requirements;
    }

    // Otherwise, wrap the rest of the text...

    int wrap_count = 0;
    size_t index = first_break.next;
    float new_x = 0;
    while (index < length)
    {
        ++wrap_count;
        auto break_result = alia_msdf_break_text(
            text.engine,
            text.font_index,
            text.text,
            index,
            length,
            length,
            text.font_size,
            line_width - text.spacing * 2,
            true);
        index = break_result.next;
        new_x = break_result.width;
    }

    requirements.interior_height
        = (wrap_count - 1)
        * (metrics->line_height * text.font_size + text.spacing * 2);
    requirements.last_line
        = {.height = metrics->line_height * text.font_size + text.spacing * 2,
           .ascent = metrics->ascender * text.font_size + text.spacing,
           .descent = -metrics->descender * text.font_size + text.spacing};
    requirements.end_x = new_x + text.spacing * 2;

    return requirements;
}

void
assign_text_wrapped_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_wrapping_assignment const* assignment)
{
    auto& text = *reinterpret_cast<msdf_text_layout_node*>(node);
    auto const* metrics
        = alia_msdf_get_font_metrics(text.engine, text.font_index);

    size_t length = strlen(text.text);

    text_layout_placement_header* header
        = arena_alloc<text_layout_placement_header>(ctx->arena);
    header->fragment_count = 0;

    // TODO: This all feels a bit hacky, but it works for now, and it feels
    // like there is more significant restructuring to come anyway.

    float x = assignment->first_line_x_offset;
    float y = assignment->y_base + assignment->first_line.baseline_offset;
    float next_y = assignment->y_base + assignment->first_line.line_height
                 + metrics->ascender * text.font_size + text.spacing;

    size_t index = 0;
    while (index < length)
    {
        auto break_result = alia_msdf_break_text(
            text.engine,
            text.font_index,
            text.text,
            index,
            length,
            length,
            text.font_size,
            assignment->line_width - x - text.spacing * 2,
            x == 0);
        size_t const end_index = break_result.next;

        if (end_index == length)
        {
            y += assignment->last_line.baseline_offset
               - (metrics->ascender * text.font_size + text.spacing);
        }

        text_layout_placement_fragment* fragment
            = arena_alloc<text_layout_placement_fragment>(ctx->arena);
        fragment->position
            = {x + assignment->x_base + text.spacing,
               y - metrics->ascender * text.font_size};
        fragment->size
            = {assignment->line_width - x - text.spacing * 2,
               metrics->line_height * text.font_size};
        fragment->text = text.text + index;
        fragment->length = end_index - index;
        fragment->font_index = text.font_index;
        ++header->fragment_count;

        x = 0;
        y = next_y;
        next_y += metrics->line_height * text.font_size + text.spacing * 2;
        index = end_index;
    }
}

alia_layout_node_vtable text_layout_vtable
    = {measure_text_horizontal,
       assign_text_widths,
       measure_text_vertical,
       assign_text_boxes,
       measure_text_wrapped_horizontal,
       measure_text_wrapped_vertical,
       assign_text_wrapped_boxes};

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
            auto& text_placement = *arena_alloc<text_layout_placement_header>(
                *alia_layout_placement_arena(&ctx));
            for (int i = 0; i < text_placement.fragment_count; ++i)
            {
                auto& fragment = *arena_alloc<text_layout_placement_fragment>(
                    *alia_layout_placement_arena(&ctx));
                alia_box box
                    = {.min = fragment.position, .size = fragment.size};
                alia_element_box_region(&ctx, id, &box, ALIA_CURSOR_DEFAULT);
            }
            break;
        }
        case ALIA_CATEGORY_DRAWING: {
            auto& text_placement = *arena_alloc<text_layout_placement_header>(
                *alia_layout_placement_arena(&ctx));
            for (int i = 0; i < text_placement.fragment_count; ++i)
            {
                auto& fragment = *arena_alloc<text_layout_placement_fragment>(
                    *alia_layout_placement_arena(&ctx));
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
            auto& text_placement = *arena_alloc<text_layout_placement_header>(
                *alia_layout_placement_arena(&ctx));
            for (int i = 0; i < text_placement.fragment_count; ++i)
            {
                auto& fragment = *arena_alloc<text_layout_placement_fragment>(
                    *alia_layout_placement_arena(&ctx));
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
