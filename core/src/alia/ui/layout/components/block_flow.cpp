#include <alia/abi/ui/layout/utilities.h>
#include <alia/abi/ui/style.h>
#include <alia/impl/ui/layout.hpp>

using namespace alia::operators;

namespace alia {

using block_flow_layout_node = alia_layout_container;

struct block_flow_child_scratch
{
    alia_horizontal_requirements x;
    float assigned_width;
    alia_vertical_requirements y;
};

struct block_flow_scratch
{
    std::uint32_t child_count = 0;
    float max_child_width = 0, total_height = 0, ascent = 0;
};

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

    auto* child_scratch = arena_alloc_array<block_flow_child_scratch>(
        ctx->scratch, scratch.child_count);

    float max_child_width = 0;
    for (alia_layout_node* child = block_flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const child_x = alia_measure_horizontal(ctx, child);
        max_child_width = (std::max) (max_child_width, child_x.min_size);
        child_scratch->x = child_x;
        ++child_scratch;
    }

    scratch.max_child_width = max_child_width;

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

static alia_line_requirements
block_flow_measure_line_vertical(
    alia_measurement_context* ctx,
    int child_count,
    alia_layout_node* line_start_child,
    block_flow_child_scratch* child_scratch,
    float extra_space,
    float line_growth)
{
    alia_line_requirements line = {0};
    alia_layout_node* child = line_start_child;
    float const growth_scale
        = line_growth > 0 ? extra_space / line_growth : 0.f;
    for (int i = 0; i < child_count; ++i)
    {
        auto& cs = child_scratch[i];
        float const assigned_width
            = cs.x.min_size + cs.x.growth_factor * growth_scale;
        cs.assigned_width = assigned_width;
        auto const child_y = alia_measure_vertical(
            ctx, ALIA_MAIN_AXIS_X, child, assigned_width);
        cs.y = child_y;
        alia_layout_line_fold_in_child(line, child_y);
        child = child->next_sibling;
    }
    alia_layout_line_finalize_height(line);
    return line;
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

    auto* child_scratch = arena_alloc_array<block_flow_child_scratch>(
        ctx->scratch, scratch.child_count);

    auto const assignment = alia_resolve_container_x(
        alia_fold_in_cross_axis_flags(block_flow.flags, main_axis),
        assigned_width,
        scratch.max_child_width);

    float overall_height = 0, overall_ascent = 0;
    float current_x_offset = 0, line_growth = 0;
    bool wrapping_has_occurred = false;
    alia_layout_node* line_start_child = block_flow.first_child;
    int child_index = 0, line_start_index = 0;
    for (alia_layout_node* child = block_flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const& cs = child_scratch[child_index];
        if (current_x_offset + cs.x.min_size > assignment.size)
        {
            int const line_child_count = child_index - line_start_index;
            auto line = block_flow_measure_line_vertical(
                ctx,
                line_child_count,
                line_start_child,
                child_scratch + line_start_index,
                assignment.size - current_x_offset,
                line_growth);

            if (!wrapping_has_occurred)
            {
                overall_ascent = line.ascent;
                wrapping_has_occurred = true;
            }
            overall_height += line.height;

            current_x_offset = 0;
            line_growth = 0;
            line_start_index = child_index;
            line_start_child = child;
        }

        current_x_offset += cs.x.min_size;
        line_growth += cs.x.growth_factor;
        ++child_index;
    }

    {
        int const line_child_count = child_index - line_start_index;
        auto line = block_flow_measure_line_vertical(
            ctx,
            line_child_count,
            line_start_child,
            child_scratch + line_start_index,
            assignment.size - current_x_offset,
            line_growth);

        if (!wrapping_has_occurred)
        {
            overall_ascent = line.ascent;
            wrapping_has_occurred = true;
        }
        overall_height += line.height;
    }

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
            float const gap = extra / static_cast<float>(n);
            *out_leading = gap * 0.5f;
            *out_gap = gap;
            break;
        }
        case ALIA_JUSTIFY_SPACE_EVENLY: {
            float const gap = extra / static_cast<float>(n + 1);
            *out_leading = gap;
            *out_gap = gap;
            break;
        }
    }
}

static void
block_flow_assign_line_boxes(
    alia_placement_context* ctx,
    alia_layout_flags_t block_flags,
    alia_vec2f position,
    float gap,
    alia_line_requirements& line,
    int child_count,
    alia_layout_node* line_start_child,
    block_flow_child_scratch* child_scratch)
{
    alia_layout_line_finalize_height(line);
    float const baseline = alia_resolve_baseline(
        block_flags, line.height, line.ascent, line.descent);
    alia_layout_node* child = line_start_child;
    for (int i = 0; i < child_count; ++i)
    {
        auto const& cs = child_scratch[i];
        alia_assign_boxes(
            ctx,
            ALIA_MAIN_AXIS_X,
            child,
            {.min = position, .size = {cs.assigned_width, line.height}},
            baseline);
        position.x += cs.assigned_width + gap;
        child = child->next_sibling;
    }
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

    auto* child_scratch = arena_alloc_array<block_flow_child_scratch>(
        ctx->scratch, scratch.child_count);

    if (scratch.child_count == 0)
        return;

    auto const placement = alia_resolve_container_box(
        alia_fold_in_cross_axis_flags(block_flow.flags, main_axis),
        box.size,
        baseline,
        {scratch.max_child_width, scratch.total_height},
        scratch.ascent);

    if ((block_flow.flags & ALIA_PROVIDE_BOX) != 0)
    {
        alia_box* provided_box = arena_alloc<alia_box>(ctx->arena);
        provided_box->min = box.min + placement.min;
        provided_box->size = placement.size;
    }

    alia_line_requirements line = {0};
    float assignment_x_base = box.min.x + placement.min.x,
          assignment_x_offset = 0, assignment_y = box.min.y + placement.min.y;
    // `wrapping_x_offset` is used to recreate the wrapping behavior that was
    // done during measurement. (In theory, the assigned widths should yield
    // the same result, but they are susceptible to floating point errors.)
    float wrapping_x_offset = 0;
    alia_layout_node* line_start_child = block_flow.first_child;
    int child_index = 0, line_start_index = 0;
    for (alia_layout_node* child = block_flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        auto const& cs = child_scratch[child_index];
        if (wrapping_x_offset + cs.x.min_size > box.size.x)
        {
            int const line_child_count = child_index - line_start_index;

            float leading, gap;
            block_flow_justify_line(
                block_flow.flags,
                placement.size.x,
                assignment_x_offset,
                line_child_count,
                &leading,
                &gap);

            block_flow_assign_line_boxes(
                ctx,
                block_flow.flags,
                {assignment_x_base + leading, assignment_y},
                gap,
                line,
                line_child_count,
                line_start_child,
                child_scratch + line_start_index);

            assignment_x_offset = 0;
            assignment_y += line.height;
            alia_layout_line_reset(line);
            line_start_index = child_index;
            line_start_child = child;
            wrapping_x_offset = 0;
        }

        wrapping_x_offset += cs.x.min_size;

        alia_layout_line_fold_in_child(line, cs.y);
        assignment_x_offset += cs.assigned_width;
        ++child_index;
    }

    if (line_start_child != nullptr)
    {
        int const line_child_count
            = static_cast<int>(scratch.child_count) - line_start_index;
        float leading, gap;
        block_flow_justify_line(
            block_flow.flags,
            placement.size.x,
            assignment_x_offset,
            line_child_count,
            &leading,
            &gap);

        block_flow_assign_line_boxes(
            ctx,
            block_flow.flags,
            {assignment_x_base + leading, assignment_y},
            gap,
            line,
            line_child_count,
            line_start_child,
            child_scratch + line_start_index);
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
