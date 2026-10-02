#include <alia/abi/ui/geometry.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/layout/utilities/placement.h>
#include <alia/context.h>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>

#include <algorithm>

using namespace alia::operators;

namespace alia {

struct clamped_node
{
    alia_layout_container container;
    alia_layout_flags_t flags;
    alia_vec2f max_size;
};

struct clamped_scratch
{
    alia_horizontal_requirements horizontal;
    alia_vertical_requirements vertical;
};

// Cap `assigned` at `max_size` when set, but never below the child's min.
static inline float
clamped_axis_size(float assigned, float max_size, float child_min)
{
    float const capped
        = max_size <= 0.f ? assigned : (std::min) (assigned, max_size);
    return (std::max) (capped, child_min);
}

alia_horizontal_requirements
clamped_measure_horizontal(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* base_node)
{
    auto& node = *reinterpret_cast<clamped_node*>(base_node);
    auto& scratch = claim_scratch<clamped_scratch>(ctx->scratch);
    if (!node.container.first_child)
    {
        scratch.horizontal = alia_horizontal_requirements{
            .min_size = 0.f,
            .growth_factor = alia_resolve_growth_factor(node.flags)};
        return scratch.horizontal;
    }
    scratch.horizontal
        = alia_measure_horizontal(ctx, main_axis, node.container.first_child);
    scratch.horizontal.growth_factor = alia_resolve_growth_factor(node.flags);
    return scratch.horizontal;
}

alia_vertical_requirements
clamped_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* base_node,
    float assigned_width)
{
    auto& node = *reinterpret_cast<clamped_node*>(base_node);
    auto& scratch = use_scratch<clamped_scratch>(ctx->scratch);
    if (!node.container.first_child)
    {
        scratch.vertical = alia_vertical_requirements{
            .min_size = 0.f,
            .growth_factor = alia_resolve_growth_factor(node.flags),
            .ascent = 0.f,
            .descent = 0.f};
        return scratch.vertical;
    }
    float const content_width = clamped_axis_size(
        assigned_width, node.max_size.x, scratch.horizontal.min_size);
    scratch.vertical = alia_measure_vertical(
        ctx, main_axis, node.container.first_child, content_width);
    scratch.vertical.growth_factor = alia_resolve_growth_factor(node.flags);
    return alia_mask_reported_vertical_requirements(
        node.flags, main_axis, scratch.vertical);
}

void
clamped_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* base_node,
    alia_box box,
    float baseline)
{
    auto& node = *reinterpret_cast<clamped_node*>(base_node);
    auto& scratch = use_scratch<clamped_scratch>(ctx->scratch);
    if (!node.container.first_child)
        return;

    alia_vec2f const content_size{
        clamped_axis_size(
            box.size.x, node.max_size.x, scratch.horizontal.min_size),
        clamped_axis_size(
            box.size.y, node.max_size.y, scratch.vertical.min_size)};

    alia_layout_flags_t const place_flags
        = alia_resolve_alignment_flags(node.flags, main_axis);
    // On axes that have a cap, FILL would undo the cap.
    ALIA_ASSERT(
        node.max_size.x <= 0.f
        || (place_flags & ALIA_X_ALIGNMENT_MASK) != ALIA_FILL_X);
    ALIA_ASSERT(
        node.max_size.y <= 0.f
        || (place_flags & ALIA_Y_ALIGNMENT_MASK) != ALIA_FILL_Y);

    alia_box const placement = alia_resolve_container_box(
        place_flags,
        box.size,
        baseline,
        content_size,
        scratch.vertical.ascent);

    // Prefer the computed content size over stretchy container placement.
    alia_box const child_box{
        .min = box.min + placement.min, .size = content_size};

    alia_assign_boxes(
        ctx,
        main_axis,
        node.container.first_child,
        child_box,
        scratch.vertical.ascent);
}

alia_layout_node_vtable clamped_vtable
    = {clamped_measure_horizontal,
       clamped_measure_vertical,
       clamped_assign_boxes,
       alia_default_count_flow_emissions,
       alia_default_emit_flow_fragments,
       alia_default_read_fragment_placements};

} // namespace alia

using namespace alia;

extern "C" {

struct alia_layout_clamped_scope
{
    clamped_node* node;
};

void
alia_layout_clamped_begin(
    alia_context* ctx, alia_vec2f max_size, alia_layout_flags_t flags)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_push<alia_layout_clamped_scope>(ctx);
        auto* node = arena_alloc<clamped_node>(ctx->layout->emission.arena);
        *node = clamped_node{
            .container
            = {.base = {.vtable = &clamped_vtable, .next_sibling = 0},
               .flags = 0,
               .first_child = 0},
            .flags = flags,
            .max_size
            = alia_vec2f{alia_px(ctx, max_size.x), alia_px(ctx, max_size.y)}};
        scope.node = node;
        alia_layout_container_activate(ctx, &node->container);
    }
}

void
alia_layout_clamped_end(alia_context* ctx)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_pop<alia_layout_clamped_scope>(ctx);
        alia_layout_container_deactivate(ctx, &scope.node->container);
    }
}

} // extern "C"
