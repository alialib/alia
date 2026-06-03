#include <alia/abi/ui/style.h>
#include <alia/impl/ui/layout.hpp>

using namespace alia::operators;

namespace alia {

using flow_layout_node = alia_layout_container;

struct flow_scratch
{
    int fragment_count = 0;
    float max_fragment_width = 0;
    float overall_height = 0, overall_ascent = 0;
    alia_arena_marker scratch_end = {};
};

alia_horizontal_requirements
flow_measure_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = claim_scratch<flow_scratch>(ctx->scratch);

    int fragment_count = 0;
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        fragment_count += alia_count_flow_fragments(ctx, child);
    }
    scratch.fragment_count = fragment_count;

    alia_flow_fragment* fragments
        = arena_alloc_array<alia_flow_fragment>(ctx->scratch, fragment_count);
    // Reserve space for the fragment placements.
    arena_alloc_array<alia_flow_fragment_placement>(
        ctx->scratch, fragment_count);

    alia_flow_fragment_emitter emitter = {.fragments = fragments, .index = 0};
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        alia_emit_flow_fragments(ctx, child, &emitter);
    }

    scratch.scratch_end = alia_arena_mark(&ctx->scratch);

    float max_fragment_width = 0;
    for (int i = 0; i < fragment_count; ++i)
    {
        max_fragment_width
            = (std::max) (max_fragment_width, fragments[i].width);
    }
    scratch.max_fragment_width = max_fragment_width;

    return alia_horizontal_requirements{
        .min_size = max_fragment_width,
        .growth_factor = alia_resolve_growth_factor(flow.flags)};
}

void
flow_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    // TODO
    // auto& flow = *reinterpret_cast<FlowLayoutNode*>(node);
    // auto& scratch = use_scratch<FlowScratch>(*scratch);
    // HorizontalRequirements* x_requirements
    //     = reinterpret_cast<HorizontalRequirements*>(scratch->allocate(
    //         flow.child_count * sizeof(HorizontalRequirements),
    //         alignof(HorizontalRequirements)));
    // VerticalRequirements* y_requirements
    //     = reinterpret_cast<VerticalRequirements*>(scratch->allocate(
    //         flow.child_count * sizeof(VerticalRequirements),
    //         alignof(VerticalRequirements)));
    // for (LayoutNode* child = flow.first_child; child != nullptr;
    //      child = child->next_sibling)
    // {
    //     auto const child_x = *x_requirements++;
    //     alia_assign_widths(scratch, child, child_x.min_size);
    // }
}

alia_vertical_requirements
flow_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = use_scratch<flow_scratch>(ctx->scratch);

    alia_flow_fragment* fragments = arena_alloc_array<alia_flow_fragment>(
        ctx->scratch, scratch.fragment_count);

    // We don't actually invoke the children at all during this step, so we
    // need to jump over the scratch space that they would have used.
    // (Note that we are also jumping over the fragment placements since we
    // don't need them here either.)
    alia_arena_jump(&ctx->scratch, scratch.scratch_end);

    auto const assignment = alia_resolve_container_x(
        alia_fold_in_cross_axis_flags(flow.flags, main_axis),
        assigned_width,
        scratch.max_fragment_width);

    alia_line_requirements line = {0};
    float overall_height = 0, overall_ascent = 0;
    float current_x_offset = 0;
    bool wrapping_has_occurred = false;
    for (int i = 0; i < scratch.fragment_count; ++i)
    {
        alia_layout_line_fold_in_fragment(line, fragments[i]);
        if (current_x_offset + fragments[i].width > assignment.size)
        {
            alia_layout_line_finalize_height(line);
            if (!wrapping_has_occurred)
            {
                overall_ascent = line.ascent;
                wrapping_has_occurred = true;
            }
            overall_height += line.height;

            current_x_offset = 0;
        }
        current_x_offset += fragments[i].width;
    }

    if (!wrapping_has_occurred)
        overall_ascent = line.ascent;
    overall_height += alia_layout_line_final_height(line);

    scratch.overall_height = overall_height;
    scratch.overall_ascent = overall_ascent;

    return alia_vertical_requirements{
        .min_size = overall_height,
        .growth_factor = alia_resolve_growth_factor(flow.flags),
        .ascent = (flow.flags & ALIA_Y_ALIGNMENT_MASK) == ALIA_BASELINE_Y
                    ? overall_ascent
                    : 0.0f,
        .descent = (flow.flags & ALIA_Y_ALIGNMENT_MASK) == ALIA_BASELINE_Y
                     ? overall_height - overall_ascent
                     : 0.0f};
}

void
flow_assign_line_boxes(
    alia_placement_context* ctx,
    alia_layout_flags_t flow_flags,
    alia_vec2f position,
    float gap,
    alia_line_requirements& line,
    int fragment_count,
    alia_flow_fragment* fragments,
    alia_flow_fragment_placement* placements)
{
    alia_layout_line_finalize_height(line);
    float const baseline = alia_resolve_baseline(
        flow_flags, line.height, line.ascent, line.descent);
    for (int i = 0; i < fragment_count; ++i)
    {
        placements[i] = {.position = position, .baseline = baseline};
        position.x += fragments[i].width + gap;
    }
}

void
flow_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = use_scratch<flow_scratch>(ctx->scratch);

    alia_flow_fragment* fragments = arena_alloc_array<alia_flow_fragment>(
        ctx->scratch, scratch.fragment_count);
    alia_flow_fragment_placement* placements
        = arena_alloc_array<alia_flow_fragment_placement>(
            ctx->scratch, scratch.fragment_count);

    // TODO: Is this correct/useful?
    if (scratch.fragment_count == 0)
        return;

    auto const placement = alia_resolve_container_box(
        alia_fold_in_cross_axis_flags(flow.flags, main_axis),
        box.size,
        baseline,
        {scratch.max_fragment_width, scratch.overall_height},
        scratch.overall_ascent);

    if ((flow.flags & ALIA_PROVIDE_BOX) != 0)
    {
        alia_box* provided_box = arena_alloc<alia_box>(ctx->arena);
        provided_box->min = box.min + placement.min;
        provided_box->size = placement.size;
    }

    alia_line_requirements line = {0};
    float x_assignment_base = box.min.x + placement.min.x;
    float current_x = 0, current_y = box.min.y + placement.min.y;
    int line_start_index = 0;
    for (int i = 0; i < scratch.fragment_count; ++i)
    {
        alia_layout_line_fold_in_fragment(line, fragments[i]);
        if (current_x + fragments[i].width > placement.size.x)
        {
            alia_layout_line_finalize_height(line);

            auto const spacing = alia_layout_justify_line(
                flow.flags,
                placement.size.x - current_x,
                i - line_start_index);
            flow_assign_line_boxes(
                ctx,
                flow.flags,
                {x_assignment_base + spacing.leading, current_y},
                spacing.gap,
                line,
                i - line_start_index,
                fragments + line_start_index,
                placements + line_start_index);

            current_y += line.height;

            current_x = 0;
            line_start_index = i;
        }
        current_x += fragments[i].width;
    }

    alia_layout_line_finalize_height(line);
    auto const spacing = alia_layout_justify_line(
        flow.flags,
        placement.size.x - current_x,
        scratch.fragment_count - line_start_index);
    flow_assign_line_boxes(
        ctx,
        flow.flags,
        {x_assignment_base + spacing.leading, current_y},
        spacing.gap,
        line,
        scratch.fragment_count - line_start_index,
        fragments + line_start_index,
        placements + line_start_index);

    alia_flow_fragment_reader reader
        = {.fragments = fragments, .placements = placements, .index = 0};
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        alia_layout_read_fragment_placements(ctx, child, &reader);
    }
}

int
flow_count_flow_fragments(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    int fragment_count = 0;
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        fragment_count += alia_count_flow_fragments(ctx, child);
    }
    return fragment_count;
}

void
flow_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        alia_emit_flow_fragments(ctx, child, emitter);
    }
}

void
flow_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        alia_layout_read_fragment_placements(ctx, child, reader);
    }
}

alia_layout_node_vtable flow_vtable = {
    flow_measure_horizontal,
    flow_assign_widths,
    flow_measure_vertical,
    flow_assign_boxes,
    flow_count_flow_fragments,
    flow_emit_flow_fragments,
    flow_read_fragment_placements,
};

} // namespace alia

extern "C" {

void
alia_layout_flow_begin(alia_context* ctx, alia_layout_flags_t flags)
{
    alia_layout_container_simple_begin(ctx, &alia::flow_vtable, flags);
}

void
alia_layout_flow_end(alia_context* ctx)
{
    alia_layout_container_simple_end(ctx);
}

} // extern "C"
