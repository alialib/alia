#include <alia/ui/layout/flow.hpp>
#include <alia/ui/layout/scratch.hpp>

namespace alia {

struct FlowScratch
{
    float total_height = 0, ascent = 0;
};

HorizontalRequirements
measure_flow_horizontal(LayoutScratchArena* scratch, LayoutNode* node)
{
    auto& flow = *reinterpret_cast<FlowLayoutNode*>(node);
    auto& flow_scratch = claim_scratch<FlowScratch>(*scratch);

    WrappingRequirements* child_requirements
        = reinterpret_cast<WrappingRequirements*>(scratch->allocate(
            flow.child_count * sizeof(WrappingRequirements),
            alignof(WrappingRequirements)));

    float max_child_width = 0;
    for (LayoutNode* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_requirements
            = measure_wrapped_horizontal(scratch, child);
        max_child_width
            = (std::max)(max_child_width, child_requirements.min_size);
    }
    return HorizontalRequirements{
        .min_size = max_child_width, .growth_factor = node->growth_factor};
}

void
assign_flow_widths(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    // TODO
    // auto& flow = *reinterpret_cast<FlowLayoutNode*>(node);
    // auto& flow_scratch = use_scratch<FlowScratch>(*scratch);
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

VerticalRequirements
measure_flow_vertical(
    LayoutScratchArena* scratch, LayoutNode* node, float assigned_width)
{
    auto& flow = *reinterpret_cast<FlowLayoutNode*>(node);
    auto& flow_scratch = use_scratch<FlowScratch>(*scratch);

    WrappingRequirements* child_requirements
        = reinterpret_cast<WrappingRequirements*>(scratch->allocate(
            flow.child_count * sizeof(WrappingRequirements),
            alignof(WrappingRequirements)));

    float line_height = 0, line_ascent = 0, line_descent = 0;
    float overall_height = 0, overall_ascent = 0;
    float current_x_offset = 0;
    bool first_wrap = true;
    for (LayoutNode* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const requirements = measure_wrapped_vertical(
            scratch, child, current_x_offset, assigned_width);
        *child_requirements++ = requirements;

        if (!requirements.wrapped_immediately)
        {
            line_height = (std::max)(line_height, requirements.line_height);
            line_ascent = (std::max)(line_ascent, requirements.ascent);
            line_descent = (std::max)(line_descent, requirements.descent);
        }

        // TODO: Optimize this for larger wrap counts.
        for (int i = 0; i < requirements.wrap_count; ++i)
        {
            if (first_wrap)
            {
                overall_ascent = line_ascent;
                first_wrap = false;
            }
            overall_height
                += (std::max)(line_height, line_ascent + line_descent);

            line_height = requirements.line_height;
            line_ascent = requirements.ascent;
            line_descent = requirements.descent;
        }

        current_x_offset = requirements.new_x_offset;
    }

    overall_height += (std::max)(line_height, line_ascent + line_descent);

    flow_scratch.total_height = overall_height;
    flow_scratch.ascent = overall_ascent;

    return VerticalRequirements{
        .min_size = overall_height,
        .growth_factor = node->growth_factor,
        .ascent
        = node->alignment == LayoutAlignment::Baseline ? overall_ascent : 0.0f,
        .descent = node->alignment == LayoutAlignment::Baseline
                     ? overall_height - overall_ascent
                     : 0.0f};
}

void
assign_flow_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& flow = *reinterpret_cast<FlowLayoutNode*>(node);
    auto& flow_scratch = use_scratch<FlowScratch>(*ctx->scratch);

    WrappingRequirements* child_requirements
        = reinterpret_cast<WrappingRequirements*>(ctx->scratch->allocate(
            flow.child_count * sizeof(WrappingRequirements),
            alignof(WrappingRequirements)));

    if (flow.child_count == 0)
        return;

    float line_height = 0, line_ascent = 0, line_descent = 0;

    auto update_line_measures = [&](int i) {
        for (; i < flow.child_count; ++i)
        {
            auto const requirements = child_requirements[i];
            if (requirements.wrapped_immediately)
                break;

            line_height = (std::max)(line_height, requirements.line_height);
            line_ascent = (std::max)(line_ascent, requirements.ascent);
            line_descent = (std::max)(line_descent, requirements.descent);

            if (requirements.wrap_count > 0)
                break;
        }
    };

    auto const placement = resolve_axis_assignment(
        node->alignment,
        box.size.y,
        baseline,
        flow_scratch.total_height,
        flow_scratch.ascent);

    float current_x = 0, current_y = box.pos.y + placement.offset;
    update_line_measures(0);

    int child_index = 0;
    for (LayoutNode* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const& requirements = child_requirements[child_index];

        WrappingAssignment assignment;
        assignment.x_base = box.pos.x;
        assignment.line_width = box.size.x;
        assignment.first_line_x_offset = current_x;
        assignment.y_base = current_y;

        // For the first line, we use the vertical requirements that existed
        // for the previous children.
        assignment.first_line = VerticalAssignment{
            .line_height = (std::max)(line_height, line_ascent + line_descent),
            .baseline_offset = line_ascent};

        if (requirements.wrap_count > 0)
        {
            // If the child wraps, then the middle line measurements will be
            // determined purely from the child's own requirements.
            assignment.middle_lines = VerticalAssignment{
                .line_height = requirements.line_height,
                .baseline_offset = requirements.ascent};

            // And for the last line, we need to reset to the child's
            // requirements and then incorporate any the requirements for any
            // later children that fit on that line.

            line_height = requirements.line_height;
            line_ascent = requirements.ascent;
            line_descent = requirements.descent;

            ++child_index;
            update_line_measures(child_index);

            assignment.last_line = VerticalAssignment{
                .line_height
                = (std::max)(line_height, line_ascent + line_descent),
                .baseline_offset = line_ascent};

            if (requirements.wrap_count == 1
                && requirements.wrapped_immediately)
            {
                // If the child wraps only once and does so immediately, then
                // we can just assign it using the non-wrapping interface.
                current_y += assignment.first_line.line_height;
                assign_boxes(
                    ctx,
                    child,
                    Box{.pos = Vec2{box.pos.x, current_y},
                        .size = Vec2{requirements.new_x_offset, line_height}},
                    line_ascent);
                current_x = requirements.new_x_offset;
            }
            else
            {
                // Otherwise, there is real wrapping going on, so we need to do
                // an actual wrapped assignment.
                assign_wrapped_boxes(ctx, child, &assignment);

                // Update our X and Y positions.
                current_y += assignment.first_line.line_height
                           + assignment.middle_lines.line_height
                                 * (requirements.wrap_count - 1);
                current_x = requirements.new_x_offset;
            }
        }
        else
        {
            // If the child doesn't wrap, then we can just assign it using
            // the non-wrapping interface.
            assign_boxes(
                ctx,
                child,
                Box{.pos = Vec2{box.pos.x + current_x, current_y},
                    .size
                    = Vec2{requirements.new_x_offset - current_x, line_height}},
                line_ascent);
            current_x = requirements.new_x_offset;
            ++child_index;
        }
    }
}

LayoutNodeVtable flow_vtable = {
    measure_flow_horizontal,
    assign_flow_widths,
    measure_flow_vertical,
    assign_flow_boxes,
    default_measure_wrapped_horizontal,
    default_measure_wrapped_vertical,
    nullptr,
};

} // namespace alia
