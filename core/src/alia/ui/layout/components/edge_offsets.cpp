#include <alia/abi/base/geometry/box.h>
#include <alia/abi/ui/layout/components.h>
#include <alia/abi/ui/layout/utilities/flow.h>
#include <alia/abi/ui/style.h>
#include <alia/context.h>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>

using namespace alia::operators;

namespace alia {

struct edge_offsets_layout_node
{
    alia_layout_container container;
    alia_edge_offsets offsets;
};

alia_horizontal_requirements
edge_offsets_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& edge_offsets = *reinterpret_cast<edge_offsets_layout_node*>(node);
    auto const child_x
        = alia_measure_horizontal(ctx, edge_offsets.container.first_child);
    return alia_horizontal_requirements{
        .min_size = child_x.min_size + edge_offsets.offsets.left
                  + edge_offsets.offsets.right,
        .growth_factor = child_x.growth_factor};
}

alia_vertical_requirements
edge_offsets_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& edge_offsets = *reinterpret_cast<edge_offsets_layout_node*>(node);
    auto const child_y = alia_measure_vertical(
        ctx,
        main_axis,
        edge_offsets.container.first_child,
        assigned_width - edge_offsets.offsets.top
            - edge_offsets.offsets.bottom);
    return alia_vertical_requirements{
        .min_size = child_y.min_size + edge_offsets.offsets.top
                  + edge_offsets.offsets.bottom,
        .growth_factor = child_y.growth_factor,
        .ascent = child_y.ascent + edge_offsets.offsets.top,
        .descent = child_y.descent + edge_offsets.offsets.bottom};
}

void
edge_offsets_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& edge_offsets = *reinterpret_cast<edge_offsets_layout_node*>(node);

    if ((edge_offsets.container.flags & ALIA_PROVIDE_BOX) != 0)
    {
        uint32_t* count = arena_alloc<uint32_t>(ctx->arena);
        *count = 1;
        alia_box* boxes = arena_alloc_array<alia_box>(ctx->arena, *count);
        boxes[0] = box;
    }

    auto const child_box = alia_box{
        .min = box.min
             + alia_vec2f{edge_offsets.offsets.left, edge_offsets.offsets.top},
        .size = box.size
              - alia_vec2f{
                  edge_offsets.offsets.left + edge_offsets.offsets.right,
                  edge_offsets.offsets.top + edge_offsets.offsets.bottom}};

    alia_assign_boxes(
        ctx,
        main_axis,
        edge_offsets.container.first_child,
        child_box,
        baseline - edge_offsets.offsets.top);
}

typedef struct edge_offsets_flow_scratch
{
    int child_fragment_count = 0;
} edge_offsets_flow_scratch;

alia_flow_emission_counts
edge_offsets_count_flow_emissions(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& edge_offsets = *reinterpret_cast<edge_offsets_layout_node*>(node);
    alia_flow_emission_counts totals = {0};
    int child_count = 0;
    for (alia_layout_node* child = edge_offsets.container.first_child;
         child != nullptr;
         child = child->next_sibling)
    {
        ++child_count;
        totals = alia_flow_emission_counts_add(
            totals, alia_count_flow_emissions(ctx, child));
    }
    return alia_flow_emission_counts_add_control_fragments(
        totals,
        alia_flow_inter_child_gap_count(child_count)
            + alia_flow_run_scope_control_count());
}

void
edge_offsets_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter)
{
    auto& edge_offsets = *reinterpret_cast<edge_offsets_layout_node*>(node);
    auto& scratch = claim_scratch<edge_offsets_flow_scratch>(ctx->scratch);
    int const start_fragment = emitter->fragment_count;
    alia_flow_emit_run_push_control(emitter, edge_offsets.offsets);
    bool first_child = true;
    for (alia_layout_node* child = edge_offsets.container.first_child;
         child != nullptr;
         child = child->next_sibling, first_child = false)
    {
        if (!first_child)
            alia_flow_emit_gap_control(emitter, emitter->child_gap);
        alia_emit_flow_fragments(ctx, child, emitter);
    }
    alia_flow_emit_run_pop_control(emitter);
    scratch.child_fragment_count = emitter->fragment_count - start_fragment;
}

static alia_box
make_fragment_box(
    alia_flow_fragment const* fragment,
    alia_flow_fragment_placement const* placement)
{
    alia_flow_fragment_content_payload const* content
        = alia_flow_fragment_content(fragment);
    alia_vec2f const min
        = {placement->position.x,
           placement->position.y + placement->baseline - content->ascent};
    return alia_box_make(
        min, alia_vec2f_make(content->width, content->height));
}

static void
provide_flow_line_boxes(
    alia_placement_context* ctx,
    alia_flow_fragment const* fragments,
    alia_flow_fragment_placement const* placements,
    int start,
    int end)
{
    bool have_line = false;
    alia_box line_box = {};
    float line_y = 0.f;

    uint32_t* box_count = arena_alloc<uint32_t>(ctx->arena);
    *box_count = 0;

    auto flush_line = [&]() {
        if (!have_line)
            return;
        alia_box* provided_box = arena_alloc<alia_box>(ctx->arena);
        *provided_box = line_box;
        ++*box_count;
        have_line = false;
    };

    struct run_stack_state
    {
        alia_flow_layout_run_frame stack[ALIA_FLOW_LAYOUT_RUN_STACK_CAPACITY];
        int depth = 0;
    } run_state;
    alia_flow_layout_run_stack_seed_root(run_state.stack, run_state.depth);

    for (int i = start; i < end; ++i)
    {
        alia_flow_fragment const& fragment = fragments[i];
        alia_flow_fragment_placement const& placement = placements[i];

        switch (fragment.kind)
        {
            case ALIA_FLOW_FRAGMENT_KIND_RUN_PUSH:
                alia_flow_layout_run_push_frame(
                    run_state.stack, run_state.depth, fragment.run.offsets);
                continue;
            case ALIA_FLOW_FRAGMENT_KIND_RUN_POP:
                alia_flow_layout_run_pop_frame(
                    run_state.stack, run_state.depth);
                continue;
            default:
                break;
        }

        if (!(fragment.flags & ALIA_FLOW_FRAGMENT_OMIT_FROM_BOUNDS)
            && !alia_flow_fragment_is_control(&fragment)
            && !(placement.flags & ALIA_FLOW_FRAGMENT_PLACEMENT_SUPPRESSED))
        {
            float const active_left
                = run_state.stack[run_state.depth - 1].total_left;
            float const active_right
                = run_state.stack[run_state.depth - 1].total_right;
            alia_box fragment_box = make_fragment_box(&fragment, &placement);
            fragment_box.min.x -= active_left;
            fragment_box.size.x += active_left + active_right;
            if (have_line && placement.position.y != line_y)
                flush_line();

            if (!have_line)
            {
                line_box = fragment_box;
                line_y = placement.position.y;
                have_line = true;
            }
            else
            {
                line_box = alia_box_union(line_box, fragment_box);
            }
        }
    }

    flush_line();
}

void
edge_offsets_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader)
{
    auto& edge_offsets = *reinterpret_cast<edge_offsets_layout_node*>(node);
    auto& scratch = use_scratch<edge_offsets_flow_scratch>(ctx->scratch);
    int const start = reader->index;
    int const end = start + scratch.child_fragment_count;

    if ((edge_offsets.container.flags & ALIA_PROVIDE_BOX) != 0)
    {
        provide_flow_line_boxes(
            ctx, reader->fragments, reader->placements, start, end);
    }

    alia_layout_skip_flow_run_push_fragment(reader);

    bool first_child = true;
    for (alia_layout_node* child = edge_offsets.container.first_child;
         child != nullptr;
         child = child->next_sibling, first_child = false)
    {
        if (!first_child)
            alia_layout_skip_flow_gap_fragment(reader);
        alia_layout_read_fragment_placements(ctx, child, reader);
    }

    alia_layout_skip_flow_run_pop_fragment(reader);
}

alia_layout_node_vtable edge_offsets_vtable
    = {edge_offsets_measure_horizontal,
       edge_offsets_measure_vertical,
       edge_offsets_assign_boxes,
       edge_offsets_count_flow_emissions,
       edge_offsets_emit_flow_fragments,
       edge_offsets_read_fragment_placements};

} // namespace alia

extern "C" {

using namespace alia;

struct alia_layout_edge_offsets_scope
{
    edge_offsets_layout_node* node;
};

void
alia_layout_edge_offsets_begin(
    alia_context* ctx, alia_edge_offsets offsets, alia_layout_flags_t flags)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_push<alia_layout_edge_offsets_scope>(ctx);
        edge_offsets_layout_node* node = arena_alloc<edge_offsets_layout_node>(
            ctx->layout->emission.arena);
        *node = edge_offsets_layout_node{
            .container
            = {.base = {.vtable = &edge_offsets_vtable, .next_sibling = 0},
               .flags = flags,
               .first_child = 0},
            .offsets = offsets};
        scope.node = node;
        alia_layout_container_activate(ctx, &node->container);
    }
}

void
alia_layout_edge_offsets_end(alia_context* ctx)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope = stack_pop<alia_layout_edge_offsets_scope>(ctx);
        alia_layout_container_deactivate(ctx, &scope.node->container);
    }
}

} // extern "C"
