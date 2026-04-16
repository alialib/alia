#include <alia/abi/ui/style.h>
#include <alia/impl/ui/layout.hpp>

using namespace alia::operators;

namespace alia {

using flow_layout_node = alia_layout_container;

struct flow_scratch
{
    std::uint32_t child_count = 0;
    float total_height = 0, ascent = 0;
    alia_wrapping_requirements* child_requirements = nullptr;
};

alia_horizontal_requirements
flow_measure_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = claim_scratch<flow_scratch>(ctx->scratch);

    float max_child_width = 0;
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_requirements
            = alia_measure_wrapped_horizontal(ctx, child);
        max_child_width
            = (std::max) (max_child_width, child_requirements.min_size);
        ++scratch.child_count;
    }

    scratch.child_requirements = arena_alloc_array<alia_wrapping_requirements>(
        ctx->scratch, scratch.child_count);

    return alia_horizontal_requirements{
        .min_size = max_child_width,
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

    auto* child_requirements = scratch.child_requirements;

    alia_line_requirements line = {0};
    float overall_height = 0, overall_ascent = 0;
    float current_x_offset = 0;
    bool wrapping_has_occurred = false;
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const requirements = alia_measure_wrapped_vertical(
            ctx, ALIA_MAIN_AXIS_X, child, current_x_offset, assigned_width);
        *child_requirements++ = requirements;

        alia_layout_line_fold_in_line(line, requirements.first_line);

        if (alia_layout_wrapping_has_wrapped_content(requirements))
        {
            if (!wrapping_has_occurred)
            {
                overall_ascent = line.ascent;
                wrapping_has_occurred = true;
            }
            overall_height
                += (std::max) (line.height, line.ascent + line.descent)
                 + requirements.interior_height;

            alia_layout_line_fold_in_line(line, requirements.last_line);
        }

        current_x_offset = requirements.end_x;
    }

    arena_alloc_array<alia_wrapping_requirements>(
        ctx->scratch, scratch.child_count);

    if (!wrapping_has_occurred)
        overall_ascent = line.ascent;
    overall_height += alia_layout_line_final_height(line);

    scratch.total_height = overall_height;
    scratch.ascent = overall_ascent;

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
flow_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = use_scratch<flow_scratch>(ctx->scratch);

    auto* child_requirements = scratch.child_requirements;

    if (scratch.child_count == 0)
        return;

    auto const placement = alia_resolve_container_y(
        alia_fold_in_cross_axis_flags(flow.flags, main_axis),
        box.size.y,
        baseline,
        scratch.total_height,
        scratch.ascent);

    if ((flow.flags & ALIA_PROVIDE_BOX) != 0)
    {
        alia_box* provided_box = arena_alloc<alia_box>(ctx->arena);
        provided_box->min = {box.min.x, box.min.y + placement.offset};
        provided_box->size = {box.size.x, placement.size};
    }

    alia_line_requirements line = {0};

    auto update_line_measures = [&](int i) {
        for (; i < scratch.child_count; ++i)
        {
            auto const requirements = child_requirements[i];
            if (!alia_layout_wrapping_has_first_line_content(requirements))
                break;

            alia_layout_line_fold_in_line(line, requirements.first_line);

            if (alia_layout_wrapping_has_wrapped_content(requirements))
                break;
        }
    };

    float current_x = 0, current_y = box.min.y + placement.offset;
    update_line_measures(0);
    alia_layout_line_finalize_height(line);

    int child_index = 0;
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const& requirements = child_requirements[child_index];

        alia_wrapping_assignment assignment;
        assignment.x_base = box.min.x;
        assignment.line_width = box.size.x;
        assignment.first_line_x_offset = current_x;

        // For the first line, we use the vertical requirements that existed
        // for the previous children.
        assignment.first_line = alia_vertical_assignment{
            .line_height = line.height,
            .baseline_offset = alia_resolve_baseline(
                flow.flags, line.height, line.ascent, line.descent)};

        if (alia_layout_wrapping_has_wrapped_content(requirements))
        {
            // For the last line, we need to reset to the child's requirements
            // and then incorporate any the requirements for any later children
            // that fit on that line.

            alia_layout_line_fold_in_line(line, requirements.last_line);

            ++child_index;
            update_line_measures(child_index);
            alia_layout_line_finalize_height(line);

            assignment.last_line = alia_vertical_assignment{
                .line_height = line.height,
                .baseline_offset = alia_resolve_baseline(
                    flow.flags, line.height, line.ascent, line.descent)};

            if (requirements.interior_height == 0
                && !alia_layout_wrapping_has_first_line_content(requirements))
            {
                // If the child wraps only once and does so immediately, then
                // we can just assign it using the non-wrapping interface.
                current_y += assignment.first_line.line_height;
                alia_assign_boxes(
                    ctx,
                    ALIA_MAIN_AXIS_X,
                    child,
                    {.min = {box.min.x, current_y},
                     .size = {requirements.end_x, line.height}},
                    alia_resolve_baseline(
                        flow.flags, line.height, line.ascent, line.descent));
                current_x = requirements.end_x;
            }
            else
            {
                assignment.y_base = current_y;

                // Otherwise, there is real wrapping going on, so we need to do
                // an actual wrapped assignment.
                alia_assign_wrapped_boxes(
                    ctx, ALIA_MAIN_AXIS_X, child, &assignment);

                // Update our X and Y positions.
                current_y += assignment.first_line.line_height
                           + requirements.interior_height;
                current_x = requirements.end_x;
            }
        }
        else
        {
            // If the child doesn't wrap, then we can just assign it using
            // the non-wrapping interface.
            alia_assign_boxes(
                ctx,
                ALIA_MAIN_AXIS_X,
                child,
                {.min = {box.min.x + current_x, current_y},
                 .size = {requirements.end_x - current_x, line.height}},
                alia_resolve_baseline(
                    flow.flags, line.height, line.ascent, line.descent));
            current_x = requirements.end_x;
            ++child_index;
        }
    }

    arena_alloc_array<alia_wrapping_requirements>(
        ctx->scratch, scratch.child_count);
}

alia_wrapping_requirements
flow_measure_wrapped_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float current_x_offset,
    float line_width)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = use_scratch<flow_scratch>(ctx->scratch);

    auto* child_requirements = scratch.child_requirements;

    alia_line_requirements first_line;
    bool wrapping_has_occurred = false;
    float interior_height = 0;
    alia_line_requirements line = {0};
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const requirements = alia_measure_wrapped_vertical(
            ctx, ALIA_MAIN_AXIS_X, child, current_x_offset, line_width);
        *child_requirements++ = requirements;

        alia_layout_line_fold_in_line(line, requirements.first_line);

        if (alia_layout_wrapping_has_wrapped_content(requirements))
        {
            if (!wrapping_has_occurred)
            {
                first_line = line;
                wrapping_has_occurred = true;
            }
            else
            {
                interior_height += alia_layout_line_final_height(line);
            }

            interior_height += requirements.interior_height;

            alia_layout_line_fold_in_line(line, requirements.last_line);
        }

        current_x_offset = requirements.end_x;
    }

    alia_line_requirements last_line;
    if (!wrapping_has_occurred)
    {
        first_line = line;
        last_line = {.height = 0, .ascent = 0, .descent = 0};
    }
    else
    {
        last_line = line;
    }

    arena_alloc_array<alia_wrapping_requirements>(
        ctx->scratch, scratch.child_count);

    scratch.total_height
        = alia_layout_line_final_height(first_line) + interior_height
        + alia_layout_line_final_height(last_line);
    scratch.ascent = first_line.ascent;

    return alia_wrapping_requirements{
        .first_line = first_line,
        .interior_height = interior_height,
        .last_line = last_line,
        .end_x = current_x_offset};
}

void
flow_assign_wrapped_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_wrapping_assignment const* assignment)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = use_scratch<flow_scratch>(ctx->scratch);

    auto* child_requirements = scratch.child_requirements;

    if (scratch.child_count == 0)
        return;

    alia_line_requirements line
        = alia_layout_line_from_assignment(assignment->first_line);

    auto update_line_measures = [&](int i) {
        for (; i < scratch.child_count; ++i)
        {
            auto const requirements = child_requirements[i];
            if (!alia_layout_wrapping_has_first_line_content(requirements))
                return;

            alia_layout_line_fold_in_line(line, requirements.first_line);

            if (alia_layout_wrapping_has_wrapped_content(requirements))
                return;
        }

        alia_layout_line_fold_in_assignment(line, assignment->last_line);
    };

    float current_x = assignment->first_line_x_offset;
    float current_y = assignment->y_base;
    update_line_measures(0);
    alia_layout_line_finalize_height(line);

    int child_index = 0;
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const& requirements = child_requirements[child_index];

        alia_wrapping_assignment child_assignment;
        child_assignment.x_base = assignment->x_base;
        child_assignment.line_width = assignment->line_width;
        child_assignment.first_line_x_offset = current_x;

        // For the first line, we use the vertical requirements that existed
        // for the previous children.
        child_assignment.first_line = alia_vertical_assignment{
            .line_height = line.height,
            .baseline_offset = alia_resolve_baseline(
                flow.flags, line.height, line.ascent, line.descent)};

        if (alia_layout_wrapping_has_wrapped_content(requirements))
        {
            // For the last line, we need to reset to the child's requirements
            // and then incorporate any the requirements for any later children
            // that fit on that line.

            alia_layout_line_fold_in_line(line, requirements.last_line);

            ++child_index;
            update_line_measures(child_index);
            alia_layout_line_finalize_height(line);

            child_assignment.last_line = alia_vertical_assignment{
                .line_height = line.height,
                .baseline_offset = alia_resolve_baseline(
                    flow.flags, line.height, line.ascent, line.descent)};

            if (requirements.interior_height == 0
                && !alia_layout_wrapping_has_first_line_content(requirements))
            {
                // If the child wraps only once and does so immediately, then
                // we can just assign it using the non-wrapping interface.
                current_y += child_assignment.first_line.line_height;
                alia_assign_boxes(
                    ctx,
                    ALIA_MAIN_AXIS_X,
                    child,
                    {.min = {assignment->x_base, current_y},
                     .size = {requirements.end_x, line.height}},
                    alia_resolve_baseline(
                        flow.flags, line.height, line.ascent, line.descent));
                current_x = requirements.end_x;
            }
            else
            {
                child_assignment.y_base = current_y;

                // Otherwise, there is real wrapping going on, so we need to do
                // an actual wrapped assignment.
                alia_assign_wrapped_boxes(
                    ctx, ALIA_MAIN_AXIS_X, child, &child_assignment);

                // Update our X and Y positions.
                current_y += child_assignment.first_line.line_height
                           + requirements.interior_height;
                current_x = requirements.end_x;
            }
        }
        else
        {
            // If the child doesn't wrap, then we can just assign it using
            // the non-wrapping interface.
            alia_assign_boxes(
                ctx,
                ALIA_MAIN_AXIS_X,
                child,
                alia_box{
                    .min = {assignment->x_base + current_x, current_y},
                    .size = {requirements.end_x - current_x, line.height}},
                alia_resolve_baseline(
                    flow.flags, line.height, line.ascent, line.descent));
            current_x = requirements.end_x;
            ++child_index;
        }
    }

    arena_alloc_array<alia_wrapping_requirements>(
        ctx->scratch, scratch.child_count);
}

alia_layout_node_vtable flow_vtable = {
    flow_measure_horizontal,
    flow_assign_widths,
    flow_measure_vertical,
    flow_assign_boxes,
    flow_measure_horizontal,
    flow_measure_wrapped_vertical,
    flow_assign_wrapped_boxes,
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
