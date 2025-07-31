#include <alia/ui/layout/row.hpp>

#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

void
begin_row(Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags)
{
    begin_container(ctx, scope, &row_vtable, flags);
}

void
end_row(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

struct RowScratch
{
    float total_width = 0, total_growth = 0;
    float height = 0, ascent = 0;
};

HorizontalRequirements
row_measure_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& row = *reinterpret_cast<RowLayoutNode*>(node);
    auto& row_scratch = claim_scratch<RowScratch>(*ctx->scratch);
    HorizontalRequirements* x_requirements
        = arena_array_alloc<HorizontalRequirements>(
            *ctx->scratch, row.child_count);
    for (LayoutNode* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = measure_horizontal(ctx, child);
        *x_requirements++ = child_x;
        row_scratch.total_width += child_x.min_size;
        row_scratch.total_growth += child_x.growth_factor;
    }
    return HorizontalRequirements{
        .min_size = row_scratch.total_width,
        .growth_factor = resolve_growth_factor(row.flags)};
}

void
row_assign_widths(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& row = *reinterpret_cast<RowLayoutNode*>(node);
    auto& row_scratch = use_scratch<RowScratch>(*ctx->scratch);
    HorizontalRequirements* x_requirements
        = arena_array_alloc<HorizontalRequirements>(
            *ctx->scratch, row.child_count);
    auto const placement = resolve_horizontal_assignment(
        row.flags, assigned_width, row_scratch.total_width);
    float const total_extra_space
        = (std::max)(0.f, placement.size - row_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max)(0.00001f, row_scratch.total_growth);
    for (LayoutNode* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        assign_widths(ctx, child, child_x.min_size + extra_space);
    }
}

VerticalRequirements
row_measure_vertical(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& row = *reinterpret_cast<RowLayoutNode*>(node);
    auto& row_scratch = use_scratch<RowScratch>(*ctx->scratch);
    HorizontalRequirements* x_requirements
        = arena_array_alloc<HorizontalRequirements>(
            *ctx->scratch, row.child_count);
    // TODO: Stop repeating this logic everywhere.
    auto const placement = resolve_horizontal_assignment(
        row.flags, assigned_width, row_scratch.total_width);
    float const total_extra_space
        = (std::max)(0.f, placement.size - row_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max)(0.00001f, row_scratch.total_growth);
    float height = 0, ascent = 0, descent = 0;
    for (LayoutNode* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        auto const child_y
            = measure_vertical(ctx, child, child_x.min_size + extra_space);
        height = (std::max)(height, child_y.min_size);
        ascent = (std::max)(ascent, child_y.ascent);
        descent = (std::max)(descent, child_y.descent);
    }
    row_scratch.height = height;
    row_scratch.ascent = ascent;
    return VerticalRequirements{
        .min_size = (std::max)(height, ascent + descent),
        .growth_factor = resolve_growth_factor(row.flags),
        .ascent = ascent,
        .descent = descent};
}

void
row_assign_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& row = *reinterpret_cast<RowLayoutNode*>(node);
    auto& row_scratch = use_scratch<RowScratch>(*ctx->scratch);
    HorizontalRequirements* x_requirements
        = arena_array_alloc<HorizontalRequirements>(
            *ctx->scratch, row.child_count);
    auto const placement = resolve_assignment(
        row.flags,
        box.size,
        baseline,
        Vec2{row_scratch.total_width, row_scratch.height},
        row_scratch.ascent);
    float const total_extra_space
        = (std::max)(0.f, placement.size.x - row_scratch.total_width);
    // TODO: Figure out how to handle 0 total growth.
    float const one_over_total_growth
        = 1.0f / (std::max)(0.00001f, row_scratch.total_growth);
    float current_x = box.pos.x + placement.pos.x;
    for (LayoutNode* child = row.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = *x_requirements++;
        float const extra_space = total_extra_space * child_x.growth_factor
                                * one_over_total_growth;
        assign_boxes(
            ctx,
            child,
            Box{.pos = Vec2{current_x, box.pos.y + placement.pos.y},
                .size = Vec2{child_x.min_size + extra_space, box.size.y}},
            baseline);
        current_x += child_x.min_size + extra_space;
    }
}

LayoutNodeVtable row_vtable
    = {row_measure_horizontal,
       row_assign_widths,
       row_measure_vertical,
       row_assign_boxes,
       row_measure_horizontal,
       default_measure_wrapped_vertical,
       nullptr};

} // namespace alia
