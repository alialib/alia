#include <alia/ui/layout/flow.hpp>
#include <alia/ui/layout/scratch.hpp>

namespace alia {

struct FlowScratch
{
    float total_height = 0, ascent = 0;
};

HorizontalRequirements
measure_flow_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& flow = *reinterpret_cast<FlowLayoutNode*>(node);
    auto& flow_scratch = claim_scratch<FlowScratch>(*ctx->scratch);

    WrappingRequirements* child_requirements
        = reinterpret_cast<WrappingRequirements*>(ctx->scratch->allocate(
            flow.child_count * sizeof(WrappingRequirements),
            alignof(WrappingRequirements)));

    float max_child_width = 0;
    for (LayoutNode* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_requirements = measure_wrapped_horizontal(ctx, child);
        max_child_width
            = (std::max)(max_child_width, child_requirements.min_size);
    }
    return HorizontalRequirements{
        .min_size = max_child_width,
        .growth_factor = float(flow.props.growth_factor)};
}

void
assign_flow_widths(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
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
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& flow = *reinterpret_cast<FlowLayoutNode*>(node);
    auto& flow_scratch = use_scratch<FlowScratch>(*ctx->scratch);

    WrappingRequirements* child_requirements
        = reinterpret_cast<WrappingRequirements*>(ctx->scratch->allocate(
            flow.child_count * sizeof(WrappingRequirements),
            alignof(WrappingRequirements)));

    float line_height = 0, line_ascent = 0, line_descent = 0;
    float overall_height = 0, overall_ascent = 0;
    float current_x_offset = 0;
    bool wrapping_has_occurred = false;
    for (LayoutNode* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const requirements = measure_wrapped_vertical(
            ctx, child, current_x_offset, assigned_width);
        *child_requirements++ = requirements;

        line_height = (std::max)(line_height, requirements.first_line.height);
        line_ascent = (std::max)(line_ascent, requirements.first_line.ascent);
        line_descent
            = (std::max)(line_descent, requirements.first_line.descent);

        if (has_wrapped_content(requirements))
        {
            if (!wrapping_has_occurred)
            {
                overall_ascent = line_ascent;
                wrapping_has_occurred = true;
            }
            overall_height
                += (std::max)(line_height, line_ascent + line_descent)
                 + requirements.interior_height;

            line_height = requirements.last_line.height;
            line_ascent = requirements.last_line.ascent;
            line_descent = requirements.last_line.descent;
        }

        current_x_offset = requirements.end_x;
    }

    if (!wrapping_has_occurred)
        overall_ascent = line_ascent;
    overall_height += (std::max)(line_height, line_ascent + line_descent);

    flow_scratch.total_height = overall_height;
    flow_scratch.ascent = overall_ascent;

    return VerticalRequirements{
        .min_size = overall_height,
        .growth_factor = float(flow.props.growth_factor),
        .ascent = flow.props.y_alignment == LayoutAlignment::Baseline
                    ? overall_ascent
                    : 0.0f,
        .descent = flow.props.y_alignment == LayoutAlignment::Baseline
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
            if (!has_first_line_content(requirements))
                break;

            line_height
                = (std::max)(line_height, requirements.first_line.height);
            line_ascent
                = (std::max)(line_ascent, requirements.first_line.ascent);
            line_descent
                = (std::max)(line_descent, requirements.first_line.descent);

            if (has_wrapped_content(requirements))
                break;
        }
    };

    auto const placement = resolve_axis_assignment(
        flow.props.y_alignment,
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

        // For the first line, we use the vertical requirements that existed
        // for the previous children.
        assignment.first_line = VerticalAssignment{
            .line_height = (std::max)(line_height, line_ascent + line_descent),
            .baseline_offset = line_ascent};

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

            assignment.last_line = VerticalAssignment{
                .line_height
                = (std::max)(line_height, line_ascent + line_descent),
                .baseline_offset = line_ascent};

            if (requirements.interior_height == 0
                && !has_first_line_content(requirements))
            {
                // If the child wraps only once and does so immediately, then
                // we can just assign it using the non-wrapping interface.
                current_y += assignment.first_line.line_height;
                assign_boxes(
                    ctx,
                    child,
                    Box{.pos = Vec2{box.pos.x, current_y},
                        .size = Vec2{requirements.end_x, line_height}},
                    line_ascent);
                current_x = requirements.end_x;
            }
            else
            {
                assignment.y_base = current_y;

                // Otherwise, there is real wrapping going on, so we need to do
                // an actual wrapped assignment.
                assign_wrapped_boxes(ctx, child, &assignment);

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
                child,
                Box{.pos = Vec2{box.pos.x + current_x, current_y},
                    .size = Vec2{requirements.end_x - current_x, line_height}},
                line_ascent);
            current_x = requirements.end_x;
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
