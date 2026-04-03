#include <alia/abi/ui/layout/utilities.h>
#include <alia/abi/ui/style.h>
#include <alia/impl/ui/layout.hpp>

namespace alia {

using block_flow_layout_node = alia_layout_container;

struct child_requirements
{
    alia_horizontal_requirements x;
    alia_vertical_requirements y;
};

struct block_flow_scratch
{
    std::uint32_t child_count = 0;
    float total_height = 0, ascent = 0;
};

static void
block_flow_justify_line(
    alia_layout_flags_t flags,
    float line_width,
    float sum_widths,
    int n,
    float* out_leading,
    float* out_gap)
{
    float const extra = line_width - sum_widths;
    if (extra <= 0.f || n <= 0)
    {
        *out_leading = 0.f;
        *out_gap = 0.f;
        return;
    }
    switch (flags & ALIA_JUSTIFY_MASK)
    {
        case ALIA_JUSTIFY_START:
        default:
            *out_leading = 0.f;
            *out_gap = 0.f;
            break;
        case ALIA_JUSTIFY_END:
            *out_leading = extra;
            *out_gap = 0.f;
            break;
        case ALIA_JUSTIFY_CENTER:
            *out_leading = extra * 0.5f;
            *out_gap = 0.f;
            break;
        case ALIA_JUSTIFY_SPACE_BETWEEN:
            if (n >= 2)
            {
                *out_leading = 0.f;
                *out_gap = extra / static_cast<float>(n - 1);
            }
            else
            {
                *out_leading = 0.f;
                *out_gap = 0.f;
            }
            break;
        case ALIA_JUSTIFY_SPACE_AROUND: {
            float const g = extra / static_cast<float>((std::max) (1, n));
            *out_leading = g * 0.5f;
            *out_gap = g;
            break;
        }
        case ALIA_JUSTIFY_SPACE_EVENLY: {
            float const g = extra / static_cast<float>(n + 1);
            *out_leading = g;
            *out_gap = g;
            break;
        }
    }
}

alia_horizontal_requirements
block_flow_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& block_flow = *reinterpret_cast<block_flow_layout_node*>(node);
    auto& scratch = claim_scratch<block_flow_scratch>(ctx->scratch);

    for (alia_layout_node* child = block_flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        ++scratch.child_count;
    }

    auto* child_requirements = arena_alloc_array<alia::child_requirements>(
        ctx->scratch, scratch.child_count);

    float max_child_width = 0;
    for (alia_layout_node* child = block_flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = alia_measure_horizontal(ctx, child);
        max_child_width = (std::max) (max_child_width, child_x.min_size);
        child_requirements->x = child_x;
        ++child_requirements;
    }

    return alia_horizontal_requirements{
        .min_size = max_child_width,
        .growth_factor = alia_resolve_growth_factor(block_flow.flags)};
}

void
block_flow_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    // TODO
}

alia_vertical_requirements
block_flow_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& block_flow = *reinterpret_cast<block_flow_layout_node*>(node);
    auto& scratch = use_scratch<block_flow_scratch>(ctx->scratch);

    auto* child_requirements = arena_alloc_array<alia::child_requirements>(
        ctx->scratch, scratch.child_count);

    float line_height = 0, line_ascent = 0, line_descent = 0;
    float overall_height = 0, overall_ascent = 0;
    float current_x_offset = 0;
    bool wrapping_has_occurred = false;
    for (alia_layout_node* child = block_flow.first_child; child != nullptr;
         child = child->next_sibling, ++child_requirements)
    {
        auto const child_y = alia_measure_vertical(
            ctx, ALIA_MAIN_AXIS_X, child, child_requirements->x.min_size);
        child_requirements->y = child_y;

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

    return alia_vertical_requirements{
        .min_size = overall_height,
        .growth_factor = alia_resolve_growth_factor(block_flow.flags),
        .ascent = (block_flow.flags & ALIA_Y_ALIGNMENT_MASK) == ALIA_BASELINE_Y
                    ? overall_ascent
                    : 0.0f,
        .descent
        = (block_flow.flags & ALIA_Y_ALIGNMENT_MASK) == ALIA_BASELINE_Y
            ? overall_height - overall_ascent
            : 0.0f};
}

void
block_flow_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& block_flow = *reinterpret_cast<block_flow_layout_node*>(node);
    auto& scratch = use_scratch<block_flow_scratch>(ctx->scratch);

    auto* child_requirements = arena_alloc_array<alia::child_requirements>(
        ctx->scratch, scratch.child_count);

    if (scratch.child_count == 0)
        return;

    auto const placement = alia_resolve_container_y(
        alia_fold_in_cross_axis_flags(block_flow.flags, main_axis),
        box.size.y,
        baseline,
        scratch.total_height,
        scratch.ascent);

    float line_height = 0, line_ascent = 0, line_descent = 0;

    float current_x = 0, current_y = box.min.y + placement.offset;
    alia_layout_node* line_start_child = block_flow.first_child;
    int child_index = 0, line_start_index = 0;
    for (alia_layout_node* child = block_flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const& requirements = child_requirements[child_index];

        if (current_x + requirements.x.min_size > box.size.x)
        {
            int const n = child_index - line_start_index;
            float leading, gap;
            block_flow_justify_line(
                block_flow.flags, box.size.x, current_x, n, &leading, &gap);

            float x = box.min.x + leading;
            int i = line_start_index;
            line_height = (std::max) (line_height, line_ascent + line_descent);
            for (alia_layout_node* c = line_start_child; c != child;
                 c = c->next_sibling)
            {
                auto const& child_x = child_requirements[i].x;
                ++i;
                alia_assign_boxes(
                    ctx,
                    ALIA_MAIN_AXIS_X,
                    c,
                    {.min = {x, current_y},
                     .size = {child_x.min_size, line_height}},
                    alia_resolve_baseline(
                        block_flow.flags,
                        line_height,
                        line_ascent,
                        line_descent));
                x += child_x.min_size + gap;
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
        int const n = static_cast<int>(scratch.child_count) - line_start_index;
        float leading, gap;
        block_flow_justify_line(
            block_flow.flags, box.size.x, current_x, n, &leading, &gap);

        float x = box.min.x + leading;
        int i = line_start_index;
        line_height = (std::max) (line_height, line_ascent + line_descent);
        for (alia_layout_node* c = line_start_child; c != nullptr;
             c = c->next_sibling)
        {
            auto const& child_x = child_requirements[i].x;
            ++i;
            alia_assign_boxes(
                ctx,
                ALIA_MAIN_AXIS_X,
                c,
                {.min = {x, current_y},
                 .size = {child_x.min_size, line_height}},
                alia_resolve_baseline(
                    block_flow.flags, line_height, line_ascent, line_descent));
            x += child_x.min_size + gap;
        }
    }
}

alia_layout_node_vtable block_flow_vtable = {
    block_flow_measure_horizontal,
    block_flow_assign_widths,
    block_flow_measure_vertical,
    block_flow_assign_boxes,
    block_flow_measure_horizontal,
    alia_default_measure_wrapped_vertical,
    nullptr,
};

} // namespace alia

extern "C" {

void
alia_layout_block_flow_begin(alia_context* ctx, alia_layout_flags_t flags)
{
    alia_layout_container_simple_begin(ctx, &alia::block_flow_vtable, flags);
}

void
alia_layout_block_flow_end(alia_context* ctx)
{
    alia_layout_container_simple_end(ctx);
}

} // extern "C"
