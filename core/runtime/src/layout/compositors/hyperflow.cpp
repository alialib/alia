#include <alia/layout/compositors/hyperflow.hpp>

#include <alia/layout/utilities.hpp>

namespace alia {

void
begin_hyperflow(
    context& ctx, layout_container_scope& scope, layout_flag_set flags)
{
    begin_container(ctx, scope, &hyperflow_vtable, flags);
}

void
end_hyperflow(context& ctx, layout_container_scope& scope)
{
    end_container(ctx, scope);
}

struct child_requirements
{
    horizontal_requirements x;
    vertical_requirements y;
};

struct hyperflow_scratch
{
    std::uint32_t child_count = 0;
    float total_height = 0, ascent = 0;
};

horizontal_requirements
hyperflow_measure_horizontal(measurement_context* ctx, layout_node* node)
{
    auto& hyperflow = *reinterpret_cast<hyperflow_layout_node*>(node);
    auto& scratch = claim_scratch<hyperflow_scratch>(*ctx->scratch);

    for (layout_node* child = hyperflow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        ++scratch.child_count;
    }

    auto* child_requirements = arena_array_alloc<alia::child_requirements>(
        *ctx->scratch, scratch.child_count);

    float max_child_width = 0;
    for (layout_node* child = hyperflow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = measure_horizontal(ctx, child);
        max_child_width = (std::max) (max_child_width, child_x.min_size);
        child_requirements->x = child_x;
        ++child_requirements;
    }

    return horizontal_requirements{
        .min_size = max_child_width,
        .growth_factor = resolve_growth_factor(hyperflow.flags)};
}

void
hyperflow_assign_widths(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    // TODO
}

vertical_requirements
hyperflow_measure_vertical(
    measurement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    float assigned_width)
{
    auto& hyperflow = *reinterpret_cast<hyperflow_layout_node*>(node);
    auto& scratch = use_scratch<hyperflow_scratch>(*ctx->scratch);

    auto* child_requirements = arena_array_alloc<alia::child_requirements>(
        *ctx->scratch, scratch.child_count);

    float line_height = 0, line_ascent = 0, line_descent = 0;
    float overall_height = 0, overall_ascent = 0;
    float current_x_offset = 0;
    bool wrapping_has_occurred = false;
    for (layout_node* child = hyperflow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_y = measure_vertical(
            ctx, ALIA_MAIN_AXIS_X, child, child_requirements->x.min_size);
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
                += (std::max) (line_height, line_ascent + line_descent);

            current_x_offset = 0;
            line_height = 0;
            line_ascent = 0;
            line_descent = 0;
        }

        line_height = (std::max) (line_height, child_y.min_size);
        line_ascent = (std::max) (line_ascent, child_y.ascent);
        line_descent = (std::max) (line_descent, child_y.descent);

        current_x_offset += child_requirements->x.min_size;
    }

    if (!wrapping_has_occurred)
        overall_ascent = line_ascent;
    overall_height += (std::max) (line_height, line_ascent + line_descent);

    scratch.total_height = overall_height;
    scratch.ascent = overall_ascent;

    return vertical_requirements{
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
    placement_context* ctx,
    main_axis_index main_axis,
    layout_node* node,
    box box,
    float baseline)
{
    auto& hyperflow = *reinterpret_cast<hyperflow_layout_node*>(node);
    auto& scratch = use_scratch<hyperflow_scratch>(*ctx->scratch);

    auto* child_requirements = arena_array_alloc<alia::child_requirements>(
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

    float current_x = 0, current_y = box.min.y + placement.offset;
    layout_node* line_start_child = hyperflow.first_child;
    int child_index = 0, line_start_index = 0;
    for (layout_node* child = hyperflow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const& requirements = child_requirements[child_index];

        if (current_x + requirements.x.min_size > box.size.x)
        {
            float x = box.min.x;
            int i = line_start_index;
            line_height = (std::max) (line_height, line_ascent + line_descent);
            for (layout_node* c = line_start_child; c != child;
                 c = c->next_sibling)
            {
                auto const& child_x = child_requirements[i].x;
                ++i;
                assign_boxes(
                    ctx,
                    ALIA_MAIN_AXIS_X,
                    c,
                    {.min = vec2{x, current_y},
                     .size = vec2{child_x.min_size, line_height}},
                    assign_baseline(
                        hyperflow.flags,
                        line_height,
                        line_ascent,
                        line_descent));
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

        line_height = (std::max) (line_height, requirements.y.min_size);
        line_ascent = (std::max) (line_ascent, requirements.y.ascent);
        line_descent = (std::max) (line_descent, requirements.y.descent);

        current_x += requirements.x.min_size;
        ++child_index;
    }

    if (line_start_child != nullptr)
    {
        float x = box.min.x;
        int i = line_start_index;
        line_height = (std::max) (line_height, line_ascent + line_descent);
        for (layout_node* c = line_start_child; c != nullptr;
             c = c->next_sibling)
        {
            auto const& child_x = child_requirements[i].x;
            ++i;
            assign_boxes(
                ctx,
                ALIA_MAIN_AXIS_X,
                c,
                {.min = vec2{x, current_y},
                 .size = vec2{child_x.min_size, line_height}},
                assign_baseline(
                    hyperflow.flags, line_height, line_ascent, line_descent));
            x += child_x.min_size;
        }
    }
}

layout_node_vtable hyperflow_vtable = {
    hyperflow_measure_horizontal,
    hyperflow_assign_widths,
    hyperflow_measure_vertical,
    hyperflow_assign_boxes,
    hyperflow_measure_horizontal,
    default_measure_wrapped_vertical,
    nullptr,
};

} // namespace alia
