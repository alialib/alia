#include <alia/layout/compositors/flow.hpp>

#include <alia/layout/utilities.hpp>

namespace alia {

void
begin_flow(context& ctx, layout_container_scope& scope, layout_flag_set flags)
{
    begin_container(ctx, scope, &flow_vtable, flags);
}

void
end_flow(context& ctx, layout_container_scope& scope)
{
    end_container(ctx, scope);
}

struct flow_scratch
{
    std::uint32_t child_count = 0;
    float total_height = 0, ascent = 0;
    wrapping_requirements* child_requirements = nullptr;
};

horizontal_requirements
flow_measure_horizontal(measurement_context* ctx, layout_node* node)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = claim_scratch<flow_scratch>(*ctx->scratch);

    float max_child_width = 0;
    for (layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_requirements = measure_wrapped_horizontal(ctx, child);
        max_child_width
            = (std::max) (max_child_width, child_requirements.min_size);
        ++scratch.child_count;
    }

    scratch.child_requirements = arena_array_alloc<wrapping_requirements>(
        *ctx->scratch, scratch.child_count);

    return horizontal_requirements{
        .min_size = max_child_width,
        .growth_factor = resolve_growth_factor(flow.flags)};
}

void
flow_assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
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
    //     assign_widths(scratch, child, child_x.min_size);
    // }
}

vertical_requirements
flow_measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = use_scratch<flow_scratch>(*ctx->scratch);

    auto* child_requirements = scratch.child_requirements;

    float line_height = 0, line_ascent = 0, line_descent = 0;
    float overall_height = 0, overall_ascent = 0;
    float current_x_offset = 0;
    bool wrapping_has_occurred = false;
    for (layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const requirements = measure_wrapped_vertical(
            ctx, ALIA_MAIN_AXIS_X, child, current_x_offset, assigned_width);
        *child_requirements++ = requirements;

        line_height = (std::max) (line_height, requirements.first_line.height);
        line_ascent = (std::max) (line_ascent, requirements.first_line.ascent);
        line_descent
            = (std::max) (line_descent, requirements.first_line.descent);

        if (has_wrapped_content(requirements))
        {
            if (!wrapping_has_occurred)
            {
                overall_ascent = line_ascent;
                wrapping_has_occurred = true;
            }
            overall_height
                += (std::max) (line_height, line_ascent + line_descent)
                 + requirements.interior_height;

            line_height = requirements.last_line.height;
            line_ascent = requirements.last_line.ascent;
            line_descent = requirements.last_line.descent;
        }

        current_x_offset = requirements.end_x;
    }

    arena_array_alloc<wrapping_requirements>(
        *ctx->scratch, scratch.child_count);

    if (!wrapping_has_occurred)
        overall_ascent = line_ascent;
    overall_height += (std::max) (line_height, line_ascent + line_descent);

    scratch.total_height = overall_height;
    scratch.ascent = overall_ascent;

    return vertical_requirements{
        .min_size = overall_height,
        .growth_factor = resolve_growth_factor(flow.flags),
        .ascent = (flow.flags & Y_ALIGNMENT_MASK) == BASELINE_Y
                    ? overall_ascent
                    : 0.0f,
        .descent = (flow.flags & Y_ALIGNMENT_MASK) == BASELINE_Y
                     ? overall_height - overall_ascent
                     : 0.0f};
}

void
flow_assign_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    box box,
    float baseline)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = use_scratch<flow_scratch>(*ctx->scratch);

    auto* child_requirements = scratch.child_requirements;

    if (scratch.child_count == 0)
        return;

    auto const placement = resolve_vertical_assignment(
        adjust_flags_for_main_axis(flow.flags, main_axis),
        box.size.y,
        baseline,
        scratch.total_height,
        scratch.ascent);

    float line_height = 0, line_ascent = 0, line_descent = 0;

    auto update_line_measures = [&](int i) {
        for (; i < scratch.child_count; ++i)
        {
            auto const requirements = child_requirements[i];
            if (!has_first_line_content(requirements))
                break;

            line_height
                = (std::max) (line_height, requirements.first_line.height);
            line_ascent
                = (std::max) (line_ascent, requirements.first_line.ascent);
            line_descent
                = (std::max) (line_descent, requirements.first_line.descent);

            if (has_wrapped_content(requirements))
                break;
        }
    };

    float current_x = 0, current_y = box.min.y + placement.offset;
    update_line_measures(0);
    line_height = (std::max) (line_height, line_ascent + line_descent);

    int child_index = 0;
    for (layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const& requirements = child_requirements[child_index];

        wrapping_assignment assignment;
        assignment.x_base = box.min.x;
        assignment.line_width = box.size.x;
        assignment.first_line_x_offset = current_x;

        // For the first line, we use the vertical requirements that existed
        // for the previous children.
        assignment.first_line = vertical_assignment{
            .line_height = line_height,
            .baseline_offset = assign_baseline(
                flow.flags, line_height, line_ascent, line_descent)};

        if (has_wrapped_content(requirements))
        {
            // For the last line, we need to reset to the child's requirements
            // and then incorporate any the requirements for any later children
            // that fit on that line.

            line_height = requirements.last_line.height;
            line_ascent = requirements.last_line.ascent;
            line_descent = requirements.last_line.descent;

            ++child_index;
            update_line_measures(child_index);
            line_height = (std::max) (line_height, line_ascent + line_descent);

            assignment.last_line = vertical_assignment{
                .line_height = line_height,
                .baseline_offset = assign_baseline(
                    flow.flags, line_height, line_ascent, line_descent)};

            if (requirements.interior_height == 0
                && !has_first_line_content(requirements))
            {
                // If the child wraps only once and does so immediately, then
                // we can just assign it using the non-wrapping interface.
                current_y += assignment.first_line.line_height;
                assign_boxes(
                    ctx,
                    ALIA_MAIN_AXIS_X,
                    child,
                    {.min = vec2{box.min.x, current_y},
                     .size = vec2{requirements.end_x, line_height}},
                    assign_baseline(
                        flow.flags, line_height, line_ascent, line_descent));
                current_x = requirements.end_x;
            }
            else
            {
                assignment.y_base = current_y;

                // Otherwise, there is real wrapping going on, so we need to do
                // an actual wrapped assignment.
                assign_wrapped_boxes(
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
            assign_boxes(
                ctx,
                ALIA_MAIN_AXIS_X,
                child,
                {.min = vec2{box.min.x + current_x, current_y},
                 .size = vec2{requirements.end_x - current_x, line_height}},
                assign_baseline(
                    flow.flags, line_height, line_ascent, line_descent));
            current_x = requirements.end_x;
            ++child_index;
        }
    }

    arena_array_alloc<wrapping_requirements>(
        *ctx->scratch, scratch.child_count);
}

wrapping_requirements
flow_measure_wrapped_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float current_x_offset,
    float line_width)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = use_scratch<flow_scratch>(*ctx->scratch);

    auto* child_requirements = scratch.child_requirements;

    float line_height = 0, line_ascent = 0, line_descent = 0;
    line_requirements first_line;
    bool wrapping_has_occurred = false;
    float interior_height = 0;
    for (layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const requirements = measure_wrapped_vertical(
            ctx, ALIA_MAIN_AXIS_X, child, current_x_offset, line_width);
        *child_requirements++ = requirements;

        line_height = (std::max) (line_height, requirements.first_line.height);
        line_ascent = (std::max) (line_ascent, requirements.first_line.ascent);
        line_descent
            = (std::max) (line_descent, requirements.first_line.descent);

        if (has_wrapped_content(requirements))
        {
            if (!wrapping_has_occurred)
            {
                first_line
                    = {.height = line_height,
                       .ascent = line_ascent,
                       .descent = line_descent};
                wrapping_has_occurred = true;
            }
            else
            {
                interior_height
                    += (std::max) (line_height, line_ascent + line_descent);
            }

            interior_height += requirements.interior_height;

            line_height = requirements.last_line.height;
            line_ascent = requirements.last_line.ascent;
            line_descent = requirements.last_line.descent;
        }

        current_x_offset = requirements.end_x;
    }

    line_requirements last_line;
    if (!wrapping_has_occurred)
    {
        first_line
            = {.height = line_height,
               .ascent = line_ascent,
               .descent = line_descent};
        last_line = {.height = 0, .ascent = 0, .descent = 0};
    }
    else
    {
        last_line
            = {.height = line_height,
               .ascent = line_ascent,
               .descent = line_descent};
    }

    arena_array_alloc<wrapping_requirements>(
        *ctx->scratch, scratch.child_count);

    scratch.total_height
        = (std::max) (first_line.height,
                      first_line.ascent + first_line.descent)
        + interior_height
        + (std::max) (last_line.height, last_line.ascent + last_line.descent);
    scratch.ascent = first_line.ascent;

    return wrapping_requirements{
        .first_line = first_line,
        .interior_height = interior_height,
        .last_line = last_line,
        .end_x = current_x_offset};
}

void
flow_assign_wrapped_boxes(
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    wrapping_assignment const* assignment)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = use_scratch<flow_scratch>(*ctx->scratch);

    auto* child_requirements = scratch.child_requirements;

    if (scratch.child_count == 0)
        return;

    float line_height = assignment->first_line.line_height,
          line_ascent = assignment->first_line.baseline_offset,
          line_descent = line_height - line_ascent;

    auto update_line_measures = [&](int i) {
        for (; i < scratch.child_count; ++i)
        {
            auto const requirements = child_requirements[i];
            if (!has_first_line_content(requirements))
                return;

            line_height
                = (std::max) (line_height, requirements.first_line.height);
            line_ascent
                = (std::max) (line_ascent, requirements.first_line.ascent);
            line_descent
                = (std::max) (line_descent, requirements.first_line.descent);

            if (has_wrapped_content(requirements))
                return;
        }

        line_height
            = (std::max) (line_height, assignment->last_line.line_height);
        line_ascent
            = (std::max) (line_ascent, assignment->last_line.baseline_offset);
        line_descent
            = (std::max) (line_descent,
                          assignment->last_line.line_height
                              - assignment->last_line.baseline_offset);
    };

    float current_x = assignment->first_line_x_offset;
    float current_y = assignment->y_base;
    update_line_measures(0);
    line_height = (std::max) (line_height, line_ascent + line_descent);

    int child_index = 0;
    for (layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const& requirements = child_requirements[child_index];

        wrapping_assignment child_assignment;
        child_assignment.x_base = assignment->x_base;
        child_assignment.line_width = assignment->line_width;
        child_assignment.first_line_x_offset = current_x;

        // For the first line, we use the vertical requirements that existed
        // for the previous children.
        child_assignment.first_line = vertical_assignment{
            .line_height = line_height,
            .baseline_offset = assign_baseline(
                flow.flags, line_height, line_ascent, line_descent)};

        if (has_wrapped_content(requirements))
        {
            // For the last line, we need to reset to the child's requirements
            // and then incorporate any the requirements for any later children
            // that fit on that line.

            line_height = requirements.last_line.height;
            line_ascent = requirements.last_line.ascent;
            line_descent = requirements.last_line.descent;

            ++child_index;
            update_line_measures(child_index);
            line_height = (std::max) (line_height, line_ascent + line_descent);

            child_assignment.last_line = vertical_assignment{
                .line_height = line_height,
                .baseline_offset = assign_baseline(
                    flow.flags, line_height, line_ascent, line_descent)};

            if (requirements.interior_height == 0
                && !has_first_line_content(requirements))
            {
                // If the child wraps only once and does so immediately, then
                // we can just assign it using the non-wrapping interface.
                current_y += child_assignment.first_line.line_height;
                assign_boxes(
                    ctx,
                    ALIA_MAIN_AXIS_X,
                    child,
                    box{.min = vec2{assignment->x_base, current_y},
                        .size = vec2{requirements.end_x, line_height}},
                    assign_baseline(
                        flow.flags, line_height, line_ascent, line_descent));
                current_x = requirements.end_x;
            }
            else
            {
                child_assignment.y_base = current_y;

                // Otherwise, there is real wrapping going on, so we need to do
                // an actual wrapped assignment.
                assign_wrapped_boxes(
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
            assign_boxes(
                ctx,
                ALIA_MAIN_AXIS_X,
                child,
                box{.min = vec2{assignment->x_base + current_x, current_y},
                    .size = vec2{requirements.end_x - current_x, line_height}},
                assign_baseline(
                    flow.flags, line_height, line_ascent, line_descent));
            current_x = requirements.end_x;
            ++child_index;
        }
    }

    arena_array_alloc<wrapping_requirements>(
        *ctx->scratch, scratch.child_count);
}

layout_node_vtable flow_vtable = {
    flow_measure_horizontal,
    flow_assign_widths,
    flow_measure_vertical,
    flow_assign_boxes,
    flow_measure_horizontal,
    flow_measure_wrapped_vertical,
    flow_assign_wrapped_boxes,
};

} // namespace alia
