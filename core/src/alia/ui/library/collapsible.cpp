#include <alia/abi/ui/library.h>

#include <alia/abi/base/geometry.h>
#include <alia/abi/kernel/animation.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/geometry.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/layout/components.h>
#include <alia/abi/ui/layout/utilities.h>

#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>
#include <alia/impl/kernel/animation.hpp>

using namespace alia::operators;

namespace alia {

struct collapsible_bit_layout
{
    impl::smoothing_bitfield expansion_smoothing;
};

struct collapsible_state
{
    bitpack<collapsible_bit_layout> bits;
    // TODO: We could use the animation bits/state themselves instead of
    // tracking the previous expansion value.
    float prev_expansion = 0.f;
    float expansion = 0.f;
    float content_height = 0.f;
};

struct collapsible_placement
{
    alia_box window = {};
    float content_height = 0.f;
};

struct collapsible_layout_node
{
    alia_layout_container base;
    collapsible_state* data = nullptr;
};

static alia_animated_transition const default_collapsible_transition
    = {alia_default_curve, milliseconds(200)};

static inline collapsible_layout_node*
collapsible_node_from(alia_layout_node* node)
{
    return reinterpret_cast<collapsible_layout_node*>(node);
}

alia_horizontal_requirements
collapsible_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    (void) ctx;
    auto& n = *collapsible_node_from(node);
    return alia_measure_horizontal(ctx, n.base.first_child);
}

void
collapsible_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& n = *collapsible_node_from(node);
    alia_assign_widths(ctx, main_axis, n.base.first_child, assigned_width);
}

alia_vertical_requirements
collapsible_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& n = *collapsible_node_from(node);
    auto& d = *n.data;

    alia_vertical_requirements child = alia_measure_vertical(
        ctx, main_axis, n.base.first_child, assigned_width);

    d.content_height = child.min_size;
    float const visible_height = d.content_height * d.expansion;
    return {
        .min_size = visible_height,
        .growth_factor = alia_resolve_growth_factor(n.base.flags),
        .ascent = 0.f,
        .descent = visible_height};
}

void
collapsible_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& n = *collapsible_node_from(node);
    auto& d = *n.data;

    float const full_height = d.content_height;

    collapsible_placement* placement
        = arena_alloc<collapsible_placement>(ctx->arena);
    placement->window = box;
    placement->content_height = full_height;

    alia_assign_boxes(
        ctx,
        main_axis,
        n.base.first_child,
        {.min = box.min, .size = {box.size.x, full_height}},
        baseline);
}

alia_layout_node_vtable collapsible_vtable
    = {collapsible_measure_horizontal,
       collapsible_assign_widths,
       collapsible_measure_vertical,
       collapsible_assign_boxes,
       collapsible_measure_horizontal,
       alia_default_measure_wrapped_vertical,
       nullptr};

struct collapsible_scope
{
    collapsible_layout_node* node = nullptr;
    collapsible_state* data = nullptr;
    collapsible_placement placement = {};
    alia_element_id id = 0;
    float offset_factor = 1.f;
};

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

void
alia_ui_collapsible_begin(
    alia_context* ctx,
    alia_bool_signal* expanded,
    alia_layout_flags_t column_flags,
    float offset_factor,
    alia_animated_transition const* transition)
{
    auto& scope = stack_push<collapsible_scope>(ctx);

    alia_substrate_usage_result result = alia_substrate_use_memory(
        ctx, sizeof(collapsible_state), alignof(collapsible_state));
    auto* data = reinterpret_cast<collapsible_state*>(result.ptr);
    if (result.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
    {
        new (data) collapsible_state{};
        data->bits = {0};
        data->prev_expansion = 0.f;
    }

    scope.data = data;
    scope.offset_factor = offset_factor;
    scope.id = result.ptr;

    bool const expanded_state
        = (expanded != nullptr
           && (expanded->flags & ALIA_SIGNAL_READABLE) != 0)
            ? expanded->value
            : false;

    alia_animated_transition const* transition_eff
        = transition != nullptr ? transition : &default_collapsible_transition;

    float const expansion = alia_smooth_float(
        ctx,
        transition_eff,
        ALIA_BITREF(data->bits, expansion_smoothing),
        expanded_state,
        1.f,
        0.f);

    data->expansion = expansion;

    if (is_refresh_event(*ctx))
    {
        // TODO: When expansion > prev_expansion, request scroll-into-view from
        // scroll parents (legacy make_widget_visible). Needs a coordinated
        // visibility-request path; not wired yet.
        (void) data->prev_expansion;
        data->prev_expansion = expansion;

        // TODO: Layout could be conditional based on the expansion value.
        auto* node = arena_alloc<collapsible_layout_node>(
            ctx->layout->emission.arena);
        *node = collapsible_layout_node{
            .base
            = {.base
               = {.vtable = &collapsible_vtable, .next_sibling = nullptr},
               .flags = column_flags,
               .first_child = nullptr},
            .data = data};
        scope.node = node;
        alia_layout_container_activate(ctx, &node->base);
        alia_layout_column_begin(ctx, column_flags);
        return;
    }

    auto* placement = arena_alloc<collapsible_placement>(
        *alia_layout_placement_arena(ctx));
    scope.placement = *placement;

    alia_element_box_region(
        ctx, scope.id, &scope.placement.window, ALIA_CURSOR_DEFAULT);

    // TODO: Only apply geometry when expansion is not 0 or 1.
    // (This requires the user to omit their content when fully collapsed.)
    // bool const apply_geometry = expansion != 0.f && expansion != 1.f;
    bool const apply_geometry = true;
    if (apply_geometry)
    {
        alia_geometry_push_clip_box(
            ctx,
            alia_box_translate(scope.placement.window, ctx->geometry->offset));

        float const offset = scope.offset_factor * (1.f - expansion)
                           * scope.placement.content_height;
        alia_geometry_push_translation(ctx, {0.f, -offset});
    }

    // TODO: We could combine all this stack data into a single structure
    // and avoid some of this conditional push/pop complexity.
    stack_push<bool>(ctx) = apply_geometry;

    // TODO: Layout could be entirely conditional based on the expansion value.
    alia_layout_column_begin(ctx, column_flags);
}

void
alia_ui_collapsible_end(alia_context* ctx)
{
    if (is_refresh_event(*ctx))
    {
        alia_layout_column_end(ctx);
        auto scope = stack_pop<collapsible_scope>(ctx);
        if (scope.node)
            alia_layout_container_deactivate(ctx, &scope.node->base);
        return;
    }

    alia_layout_column_end(ctx);

    bool const apply_geometry = stack_pop<bool>(ctx);
    if (apply_geometry)
    {
        alia_geometry_pop_translation(ctx);
        alia_geometry_pop_clip_box(ctx);
    }

    (void) stack_pop<collapsible_scope>(ctx);
}

ALIA_EXTERN_C_END
