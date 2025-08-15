#include <alia/ui/layout/hyperflow.hpp>

#include <alia/ui/layout/utilities.hpp>

namespace alia {

void
begin_hyperflow(Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags)
{
    begin_container(ctx, scope, &hyperflow_vtable, flags);
}

void
end_hyperflow(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

struct ChildRequirements
{
    HorizontalRequirements x;
    VerticalRequirements y;
};

struct HyperflowScratch
{
    std::uint32_t child_count = 0;
    float total_height = 0, ascent = 0;
};

HorizontalRequirements
hyperflow_measure_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& hyperflow = *reinterpret_cast<HyperflowLayoutNode*>(node);
    auto& scratch = claim_scratch<HyperflowScratch>(*ctx->scratch);

    for (LayoutNode* child = hyperflow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        ++scratch.child_count;
    }

    auto* child_requirements = arena_array_alloc<ChildRequirements>(
        *ctx->scratch, scratch.child_count);

    float max_child_width = 0;
    for (LayoutNode* child = hyperflow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = measure_horizontal(ctx, child);
        max_child_width = (std::max)(max_child_width, child_x.min_size);
        child_requirements->x = child_x;
        ++child_requirements;
    }

    return HorizontalRequirements{
        .min_size = max_child_width,
        .growth_factor = resolve_growth_factor(hyperflow.flags)};
}

void
hyperflow_assign_widths(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    float assigned_width)
{
    // TODO
}

VerticalRequirements
hyperflow_measure_vertical(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    float assigned_width)
{
    auto& hyperflow = *reinterpret_cast<HyperflowLayoutNode*>(node);
    auto& scratch = use_scratch<HyperflowScratch>(*ctx->scratch);

    auto* child_requirements = arena_array_alloc<ChildRequirements>(
        *ctx->scratch, scratch.child_count);

    float line_height = 0, line_ascent = 0, line_descent = 0;
    float overall_height = 0, overall_ascent = 0;
    float current_x_offset = 0;
    bool wrapping_has_occurred = false;
    for (LayoutNode* child = hyperflow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y = measure_vertical(
            ctx, MAIN_AXIS_X, child, child_requirements->x.min_size);
        child_requirements->y = child_y;
        ++child_requirements;

        if (child_requirements->x.min_size + current_x_offset > assigned_width)
        {
            if (!wrapping_has_occurred)
            {
                overall_ascent = line_ascent;
                wrapping_has_occurred = true;
            }
            overall_height
                += (std::max)(line_height, line_ascent + line_descent);

            current_x_offset = 0;
            line_height = 0;
            line_ascent = 0;
            line_descent = 0;
        }

        line_height = (std::max)(line_height, child_y.min_size);
        line_ascent = (std::max)(line_ascent, child_y.ascent);
        line_descent = (std::max)(line_descent, child_y.descent);

        current_x_offset += child_requirements->x.min_size;
    }

    if (!wrapping_has_occurred)
        overall_ascent = line_ascent;
    overall_height += (std::max)(line_height, line_ascent + line_descent);

    scratch.total_height = overall_height;
    scratch.ascent = overall_ascent;

    return VerticalRequirements{
        .min_size = overall_height,
        .growth_factor = resolve_growth_factor(hyperflow.flags),
        .ascent = (hyperflow.flags & Y_ALIGNMENT_MASK) == BASELINE_Y
                    ? overall_ascent
                    : 0.0f,
        .descent = (hyperflow.flags & Y_ALIGNMENT_MASK) == BASELINE_Y
                     ? overall_height - overall_ascent
                     : 0.0f};
}

void
hyperflow_assign_boxes(
    PlacementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    Box box,
    float baseline)
{
    auto& hyperflow = *reinterpret_cast<HyperflowLayoutNode*>(node);
    auto& scratch = use_scratch<HyperflowScratch>(*ctx->scratch);

    auto* child_requirements = arena_array_alloc<ChildRequirements>(
        *ctx->scratch, scratch.child_count);

    if (scratch.child_count == 0)
        return;

    auto const placement = resolve_vertical_assignment(
        adjust_flags_for_main_axis(hyperflow.flags, main_axis),
        box.size.y,
        baseline,
        scratch.total_height,
        scratch.ascent);

    float line_height = 0, line_ascent = 0, line_descent = 0;

    float current_x = 0, current_y = box.pos.y + placement.offset;
    LayoutNode* line_start_child = hyperflow.first_child;
    int child_index = 0, line_start_index = 0;
    for (LayoutNode* child = hyperflow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const& requirements = child_requirements[child_index];

        if (current_x + requirements.x.min_size > box.size.x)
        {
            float x = box.pos.x;
            int i = line_start_index;
            for (LayoutNode* c = line_start_child; c != child;
                 c = c->next_sibling)
            {
                auto const& child_x = child_requirements[i].x;
                ++i;
                assign_boxes(
                    ctx,
                    MAIN_AXIS_X,
                    c,
                    Box{.pos = Vec2{x, current_y},
                        .size = Vec2{child_x.min_size, line_height}},
                    line_ascent);
                x += child_x.min_size;
            }

            current_x = 0;
            current_y += line_height;
            line_height = 0;
            line_ascent = 0;
            line_descent = 0;
            line_start_index = child_index;
            line_start_child = child;
        }

        line_height = (std::max)(line_height, requirements.y.min_size);
        line_ascent = (std::max)(line_ascent, requirements.y.ascent);
        line_descent = (std::max)(line_descent, requirements.y.descent);

        current_x += requirements.x.min_size;
        ++child_index;
    }

    if (line_start_child != nullptr)
    {
        float x = box.pos.x;
        int i = line_start_index;
        // TODO
    }
}

LayoutNodeVtable hyperflow_vtable = {
    hyperflow_measure_horizontal,
    hyperflow_assign_widths,
    hyperflow_measure_vertical,
    hyperflow_assign_boxes,
    hyperflow_measure_horizontal,
    default_measure_wrapped_vertical,
    nullptr,
};

} // namespace alia
