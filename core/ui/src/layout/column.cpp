#include <alia/ui/layout/column.hpp>

#include <alia/ui/layout/utilities.hpp>

namespace alia {

void
begin_column(Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags)
{
    begin_container(ctx, scope, &column_vtable, flags);
}

void
end_column(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

struct ColumnScratch
{
    std::uint32_t child_count = 0;
    float max_width = 0;
    float total_height = 0, total_growth = 0;
    float baseline = 0;
};

HorizontalRequirements
column_measure_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& column = *reinterpret_cast<ColumnLayoutNode*>(node);
    auto& scratch = claim_scratch<ColumnScratch>(*ctx->scratch);

    for (LayoutNode* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        ++scratch.child_count;
    }
    VerticalRequirements* y_requirements
        = arena_array_alloc<VerticalRequirements>(
            *ctx->scratch, scratch.child_count);

    for (LayoutNode* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = measure_horizontal(ctx, child);
        scratch.max_width = (std::max)(scratch.max_width, child_x.min_size);
    }
    return HorizontalRequirements{
        .min_size = scratch.max_width,
        .growth_factor = resolve_growth_factor(column.flags)};
}

void
column_assign_widths(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& column = *reinterpret_cast<ColumnLayoutNode*>(node);
    auto& scratch = use_scratch<ColumnScratch>(*ctx->scratch);
    VerticalRequirements* y_requirements
        = arena_array_alloc<VerticalRequirements>(
            *ctx->scratch, scratch.child_count);
    auto const assignment = resolve_horizontal_assignment(
        column.flags, assigned_width, scratch.max_width);
    for (LayoutNode* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        assign_widths(ctx, child, assignment.size);
    }
}

VerticalRequirements
column_measure_vertical(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& column = *reinterpret_cast<ColumnLayoutNode*>(node);
    auto& scratch = use_scratch<ColumnScratch>(*ctx->scratch);
    VerticalRequirements* y_requirements
        = arena_array_alloc<VerticalRequirements>(
            *ctx->scratch, scratch.child_count);
    auto const assignment = resolve_horizontal_assignment(
        column.flags, assigned_width, scratch.max_width);
    VerticalRequirements* requirement_i = y_requirements;
    for (LayoutNode* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y = measure_vertical(ctx, child, assignment.size);
        *requirement_i++ = child_y;
        scratch.total_height += child_y.min_size;
        scratch.total_growth += child_y.growth_factor;
    }
    scratch.baseline = (scratch.child_count > 0) ? y_requirements->ascent : 0;
    return VerticalRequirements{
        .min_size = scratch.total_height,
        .growth_factor = resolve_growth_factor(column.flags),
        .ascent = scratch.baseline,
        .descent = scratch.total_height - scratch.baseline};
}

void
column_assign_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& column = *reinterpret_cast<ColumnLayoutNode*>(node);
    auto& scratch = use_scratch<ColumnScratch>(*ctx->scratch);
    VerticalRequirements* y_requirements
        = arena_array_alloc<VerticalRequirements>(
            *ctx->scratch, scratch.child_count);
    auto const assignment = resolve_assignment(
        column.flags,
        box.size,
        baseline,
        {scratch.max_width, scratch.total_height},
        scratch.baseline);
    float const total_extra_space
        = (std::max)(0.f, assignment.size.y - scratch.total_height);
    // TODO: Figure out how to handle 0 total growth.
    float const total_growth = (std::max)(0.00001f, scratch.total_growth);
    float current_y = box.pos.y + assignment.pos.y;
    for (LayoutNode* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y = *y_requirements++;
        float const extra_space
            = total_extra_space * child_y.growth_factor / total_growth;
        assign_boxes(
            ctx,
            child,
            Box{Vec2{box.pos.x + assignment.pos.x, current_y},
                Vec2{assignment.size.x, child_y.min_size + extra_space}},
            child_y.ascent);
        current_y += child_y.min_size + extra_space;
    }
}

LayoutNodeVtable column_vtable
    = {column_measure_horizontal,
       column_assign_widths,
       column_measure_vertical,
       column_assign_boxes,
       column_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
