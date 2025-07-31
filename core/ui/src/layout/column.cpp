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
    float max_width = 0;
    float total_height = 0, total_growth = 0;
    float baseline = 0;
};

HorizontalRequirements
column_measure_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& column = *reinterpret_cast<ColumnLayoutNode*>(node);
    auto& column_scratch = claim_scratch<ColumnScratch>(*ctx->scratch);
    VerticalRequirements* y_requirements
        = arena_array_alloc<VerticalRequirements>(
            *ctx->scratch, column.child_count);
    for (LayoutNode* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = measure_horizontal(ctx, child);
        column_scratch.max_width
            = (std::max)(column_scratch.max_width, child_x.min_size);
    }
    return HorizontalRequirements{
        .min_size = column_scratch.max_width,
        .growth_factor = resolve_growth_factor(column.flags)};
}

void
column_assign_widths(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& column = *reinterpret_cast<ColumnLayoutNode*>(node);
    auto& column_scratch = use_scratch<ColumnScratch>(*ctx->scratch);
    VerticalRequirements* y_requirements
        = arena_array_alloc<VerticalRequirements>(
            *ctx->scratch, column.child_count);
    auto const assignment = resolve_horizontal_assignment(
        column.flags, assigned_width, column_scratch.max_width);
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
    auto& column_scratch = use_scratch<ColumnScratch>(*ctx->scratch);
    VerticalRequirements* y_requirements
        = arena_array_alloc<VerticalRequirements>(
            *ctx->scratch, column.child_count);
    auto const assignment = resolve_horizontal_assignment(
        column.flags, assigned_width, column_scratch.max_width);
    VerticalRequirements* requirement_i = y_requirements;
    for (LayoutNode* child = column.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y = measure_vertical(ctx, child, assignment.size);
        *requirement_i++ = child_y;
        column_scratch.total_height += child_y.min_size;
        column_scratch.total_growth += child_y.growth_factor;
    }
    column_scratch.baseline
        = (column.child_count > 0) ? y_requirements->ascent : 0;
    return VerticalRequirements{
        .min_size = column_scratch.total_height,
        .growth_factor = resolve_growth_factor(column.flags),
        .ascent = column_scratch.baseline,
        .descent = column_scratch.total_height - column_scratch.baseline};
}

void
column_assign_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& column = *reinterpret_cast<ColumnLayoutNode*>(node);
    auto& column_scratch = use_scratch<ColumnScratch>(*ctx->scratch);
    VerticalRequirements* y_requirements
        = arena_array_alloc<VerticalRequirements>(
            *ctx->scratch, column.child_count);
    auto const assignment = resolve_assignment(
        column.flags,
        box.size,
        baseline,
        {column_scratch.max_width, column_scratch.total_height},
        column_scratch.baseline);
    float const total_extra_space
        = (std::max)(0.f, assignment.size.y - column_scratch.total_height);
    // TODO: Figure out how to handle 0 total growth.
    float const total_growth
        = (std::max)(0.00001f, column_scratch.total_growth);
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
