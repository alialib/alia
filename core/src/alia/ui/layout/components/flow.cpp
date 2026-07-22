#include <alia/abi/ui/layout/utilities/emission.h>
#include <alia/abi/ui/layout/utilities/flow.h>
#include <alia/abi/ui/layout/utilities/line.h>
#include <alia/abi/ui/layout/utilities/placement.h>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>

using namespace alia::operators;

namespace alia {

struct flow_layout_node
{
    alia_layout_node base;
    alia_layout_flags_t flags;
    alia_layout_node* first_child;
    float gap;
    float line_gap;
    float minimum_line_height;
};

// TODO: This is sketchy.
static alia_layout_container*
flow_as_container(flow_layout_node* node)
{
    return reinterpret_cast<alia_layout_container*>(node);
}

static flow_layout_node const*
flow_from_node(alia_layout_node* node)
{
    return reinterpret_cast<flow_layout_node*>(node);
}

struct flow_scratch
{
    int fragment_count = 0;
    float max_fragment_width = 0;
    float overall_height = 0, overall_ascent = 0;
    alia_arena_marker scratch_end = {};
};

struct flow_line_wrapper_context
{
    alia_flow_fragment const* fragments;
    int fragment_count;
    float available_width;
};

struct flow_context_frame
{
    float line_gap;
    float minimum_line_height;
    float cumulative_line_gap;
    float cumulative_minimum_line_height;
};

static int constexpr FLOW_CONTEXT_STACK_CAPACITY = 16;

static void
flow_context_stack_seed_root(flow_context_frame* stack, int& depth)
{
    stack[0] = flow_context_frame{
        .line_gap = 0.f,
        .minimum_line_height = 0.f,
        .cumulative_line_gap = 0.f,
        .cumulative_minimum_line_height = 0.f};
    depth = 1;
}

struct flow_line_wrapper_state
{
    float current_x = 0.f;

    // "Anchors" are fragments that are not suppressed at the ends of the line.
    // The portion of the line that is said to be "anchored" is the part that
    // lies between anchors. (e.g., Words in a paragraph are anchors. The
    // spaces between words are not anchors, but spaces that happen to land
    // between words on the same line are still part of the "anchored" part of
    // the line.)
    bool have_anchors = false;
    int first_anchor_index = 0;
    int last_anchor_index = 0;
    float anchored_width = 0.f;

    alia_line_requirements line = {0};

    // stack[0] is a permanent root frame (zero limits); depth is always >= 1.
    flow_context_frame context_stack[FLOW_CONTEXT_STACK_CAPACITY];
    int context_depth = 0;

    // maximum `line_gap` / `minimum_line_height` values among all contexts
    // active at any point on the current line
    float line_max_line_gap = 0.f;
    float line_max_minimum_line_height = 0.f;

    alia_flow_layout_run_frame run_stack[ALIA_FLOW_LAYOUT_RUN_STACK_CAPACITY];
    int run_depth = 0;
};

void
seed_flow_line_wrapper_state(flow_line_wrapper_state& state)
{
    flow_context_stack_seed_root(state.context_stack, state.context_depth);
    alia_flow_layout_run_stack_seed_root(state.run_stack, state.run_depth);
}

static void
flow_record_line_context_limits(
    flow_line_wrapper_state& state, flow_context_frame const& frame)
{
    state.line_max_line_gap
        = alia_max(state.line_max_line_gap, frame.cumulative_line_gap);
    state.line_max_minimum_line_height = alia_max(
        state.line_max_minimum_line_height,
        frame.cumulative_minimum_line_height);
}

static flow_context_frame
make_flow_context_frame(
    flow_context_frame const& parent,
    float line_gap,
    float minimum_line_height)
{
    return flow_context_frame{
        .line_gap = line_gap,
        .minimum_line_height = minimum_line_height,
        .cumulative_line_gap = alia_max(parent.cumulative_line_gap, line_gap),
        .cumulative_minimum_line_height = alia_max(
            parent.cumulative_minimum_line_height, minimum_line_height)};
}

template<bool kMeasurePass>
static void
flow_apply_horizontal_control(
    alia_flow_fragment const* fragment,
    bool is_at_line_start,
    float& current_x,
    alia_flow_layout_run_frame* run_stack,
    int& run_depth,
    flow_line_wrapper_state* layout_state)
{
    if constexpr (kMeasurePass)
        ALIA_ASSERT(layout_state != nullptr);

    switch (fragment->kind)
    {
        case ALIA_FLOW_FRAGMENT_KIND_GAP:
            if (is_at_line_start)
                return;
            current_x += fragment->gap.gap;
            if constexpr (kMeasurePass)
            {
                if (layout_state->have_anchors)
                {
                    layout_state->anchored_width
                        = current_x
                        + alia_flow_layout_run_total_right(
                              run_stack, run_depth);
                }
            }
            break;
        case ALIA_FLOW_FRAGMENT_KIND_CONTEXT_PUSH:
            if constexpr (kMeasurePass)
            {
                ALIA_ASSERT(layout_state->context_depth >= 1);
                ALIA_ASSERT(
                    layout_state->context_depth < FLOW_CONTEXT_STACK_CAPACITY);
                int const depth = layout_state->context_depth;
                flow_context_frame const frame = make_flow_context_frame(
                    layout_state->context_stack[depth - 1],
                    fragment->context.line_gap,
                    fragment->context.minimum_line_height);
                layout_state->context_stack[depth] = frame;
                ++layout_state->context_depth;
                flow_record_line_context_limits(*layout_state, frame);
            }
            break;
        case ALIA_FLOW_FRAGMENT_KIND_CONTEXT_POP:
            if constexpr (kMeasurePass)
            {
                ALIA_ASSERT(layout_state->context_depth > 1);
                --layout_state->context_depth;
            }
            break;
        case ALIA_FLOW_FRAGMENT_KIND_RUN_PUSH:
            alia_flow_layout_run_push(
                run_stack, run_depth, fragment->run.offsets, current_x);
            break;
        case ALIA_FLOW_FRAGMENT_KIND_RUN_POP:
            alia_flow_layout_run_pop(run_stack, run_depth, current_x);
            break;
        default:
            ALIA_ASSERT(false);
            break;
    }
}

static void
flow_finalize_line(
    flow_line_wrapper_state const& state, alia_line_requirements* line)
{
    alia_layout_line_finalize_height_with_minimum(
        line, state.line_max_minimum_line_height);
}

static void
flow_reset_line(flow_line_wrapper_state& state)
{
    state.have_anchors = false;
    state.first_anchor_index = 0;
    state.last_anchor_index = 0;
    state.anchored_width = 0.f;
    state.line_max_line_gap = 0.f;
    state.line_max_minimum_line_height = 0.f;
    flow_record_line_context_limits(
        state, state.context_stack[state.context_depth - 1]);
    state.current_x = alia_flow_layout_run_line_start_offset(
        state.run_stack, state.run_depth);
    alia_layout_line_reset(&state.line);
}

static void
flow_emit_child_fragments(
    alia_measurement_context* ctx,
    flow_layout_node const& flow,
    alia_layout_node* child,
    alia_flow_fragment_emitter* emitter,
    bool emit_gap_before)
{
    emitter->child_gap = flow.gap;
    if (emit_gap_before)
        alia_flow_emit_gap_control(emitter, flow.gap);
    alia_emit_flow_fragments(ctx, child, emitter);
}

static void
flow_emit_all_fragments(
    alia_measurement_context* ctx,
    flow_layout_node const& flow,
    alia_flow_fragment_emitter* emitter)
{
    if (flow.first_child == nullptr)
        return;

    alia_flow_emit_context_push_control(
        emitter, flow.line_gap, flow.minimum_line_height);

    bool first_child = true;
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling, first_child = false)
    {
        flow_emit_child_fragments(ctx, flow, child, emitter, !first_child);
    }

    alia_flow_emit_context_pop_control(emitter);
}

static void
process_flow_control_fragment(
    flow_line_wrapper_context const& ctx,
    flow_line_wrapper_state& state,
    alia_flow_fragment const* fragment,
    int fragment_index,
    bool is_at_line_start)
{
    (void) ctx;
    (void) fragment_index;
    flow_apply_horizontal_control<true>(
        fragment,
        is_at_line_start,
        state.current_x,
        state.run_stack,
        state.run_depth,
        &state);
}

template<class OnWrap>
void
process_flow_fragment(
    flow_line_wrapper_context const& ctx,
    flow_line_wrapper_state& state,
    alia_flow_fragment const* fragment,
    int fragment_index,
    OnWrap&& on_wrap)
{
    if (alia_flow_fragment_is_control(fragment))
    {
        process_flow_control_fragment(
            ctx, state, fragment, fragment_index, !state.have_anchors);
        return;
    }

    float fragment_width = fragment->content.size.x;

    if (!state.have_anchors)
    {
        if (fragment->flags & ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_START)
            return;
        // Otherwise, this is the first anchor on this line.
        state.have_anchors = true;
        state.first_anchor_index = fragment_index;
    }

    float x_at_fragment = state.current_x;
    float const end_of_run_edge_offsets
        = alia_flow_layout_run_total_right(state.run_stack, state.run_depth);
    // Note that we only allow wrapping to occur if we already have anchors.
    // This prevents large fragments from throwing us into an infinite wrapping
    // loop. (In theory this shouldn't happen with the current width
    // calculation logic, so this is mostly a check for the future and/or a
    // guard against floating point imprecision errors.)
    if (state.have_anchors
        && x_at_fragment + fragment_width + end_of_run_edge_offsets
               > ctx.available_width)
    {
        flow_finalize_line(state, &state.line);
        on_wrap(false);
        flow_reset_line(state);
        // We're at the start of a new line now, so we need to again apply the
        // start-of-line suppression/anchor logic.
        if (fragment->flags & ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_START)
            return;
        state.have_anchors = true;
        state.first_anchor_index = fragment_index;
        x_at_fragment = state.current_x;
    }

    alia_layout_line_fold_in_fragment(&state.line, fragment);

    state.current_x = x_at_fragment + fragment_width;

    // Update the anchor tracking.
    if (!(fragment->flags & ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_END))
    {
        state.last_anchor_index = fragment_index;
        state.anchored_width = state.current_x + end_of_run_edge_offsets;
    }

    if (fragment->flags & ALIA_FLOW_FRAGMENT_BREAK_AFTER)
    {
        flow_finalize_line(state, &state.line);
        on_wrap(true);
        flow_reset_line(state);
    }
}

alia_horizontal_requirements
flow_measure_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& flow = *flow_from_node(node);
    auto& scratch = claim_scratch<flow_scratch>(ctx->scratch);

    // Count the total number of fragments that will be emitted. Note that the
    // layout protocol requires that these functions don't touch the scratch
    // space.
    alia_flow_emission_counts totals = {0};
    int child_count = 0;
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        ++child_count;
        totals = alia_flow_emission_counts_add(
            totals, alia_count_flow_emissions(ctx, child));
    }
    totals.fragment_count
        += alia_flow_inter_child_gap_count(child_count)
         + alia_flow_context_scope_control_count(child_count > 0);
    scratch.fragment_count = totals.fragment_count;

    // Allocate our fragment array. (Again, note that we are at the start of
    // our scratch space.)
    alia_flow_fragment* fragments = arena_alloc_array<alia_flow_fragment>(
        ctx->scratch, totals.fragment_count);
    // Reserve space for the fragment placements.
    arena_alloc_array<alia_flow_fragment_placement>(
        ctx->scratch, totals.fragment_count);

    // Invoke the children to emit their fragments.
    alia_flow_fragment_emitter emitter
        = {.fragments = fragments, .fragment_count = 0, .run_stack_depth = 0};
    alia_flow_emitter_run_stack_seed_root(&emitter);
    flow_emit_all_fragments(ctx, flow, &emitter);

    // Mark the end of the scratch space for this overall node.
    scratch.scratch_end = alia_arena_mark(&ctx->scratch);

    // Compute the maximum fragment width.
    // TODO: Add a flag that bypasses this computation and allows the flow to
    // be smaller than its largest fragment.
    float max_fragment_width = 0;
    alia_flow_layout_run_frame run_stack[ALIA_FLOW_LAYOUT_RUN_STACK_CAPACITY];
    int run_depth = 0;
    alia_flow_layout_run_stack_seed_root(run_stack, run_depth);
    float current_x = 0.f;
    for (int i = 0; i < totals.fragment_count; ++i)
    {
        auto const& fragment = fragments[i];
        switch (fragment.kind)
        {
            case ALIA_FLOW_FRAGMENT_KIND_RUN_PUSH:
                alia_flow_layout_run_push(
                    run_stack, run_depth, fragment.run.offsets, current_x);
                break;
            case ALIA_FLOW_FRAGMENT_KIND_RUN_POP:
                alia_flow_layout_run_pop(run_stack, run_depth, current_x);
                break;
            case ALIA_FLOW_FRAGMENT_KIND_CONTENT:
                max_fragment_width = alia_max(
                    max_fragment_width,
                    alia_flow_layout_content_horizontal_extent(
                        fragment.content.size.x, run_stack, run_depth));
                break;
            default:
                break;
        }
    }
    scratch.max_fragment_width = max_fragment_width;

    return alia_horizontal_requirements{
        .min_size = max_fragment_width,
        .growth_factor = alia_resolve_growth_factor(flow.flags)};
}

alia_vertical_requirements
flow_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& flow = *flow_from_node(node);
    auto& scratch = use_scratch<flow_scratch>(ctx->scratch);

    alia_flow_fragment* fragments = arena_alloc_array<alia_flow_fragment>(
        ctx->scratch, scratch.fragment_count);

    // We don't actually invoke the children at all during this step, so we
    // need to jump over the scratch space that they would have used.
    // (Note that we are also jumping over the fragment placements since we
    // don't need them here either.)
    alia_arena_jump(&ctx->scratch, scratch.scratch_end);

    auto const assignment = alia_resolve_container_x(
        alia_fold_in_cross_axis_flags(flow.flags, main_axis),
        assigned_width,
        scratch.max_fragment_width);

    int const fragment_count = scratch.fragment_count;

    flow_line_wrapper_context wrapper_ctx
        = {.fragments = fragments,
           .fragment_count = fragment_count,
           .available_width = assignment.size};

    flow_line_wrapper_state state;
    seed_flow_line_wrapper_state(state);

    bool wrapping_has_occurred = false;
    float overall_height = 0.f, overall_ascent = 0.f;
    float current_line_gap = 0;

    auto on_wrap = [&](bool is_explicit_break) {
        (void) is_explicit_break;
        flow_finalize_line(state, &state.line);
        overall_height += current_line_gap;
        current_line_gap = state.line_max_line_gap;
        if (!wrapping_has_occurred)
        {
            overall_ascent = state.line.ascent;
            wrapping_has_occurred = true;
        }
        overall_height += state.line.height;
    };

    for (int i = 0; i < fragment_count; ++i)
        process_flow_fragment(wrapper_ctx, state, &fragments[i], i, on_wrap);
    // Process the final line.
    on_wrap(true);

    scratch.overall_height = overall_height;
    scratch.overall_ascent = overall_ascent;

    return alia_mask_reported_vertical_requirements(
        flow.flags,
        main_axis,
        alia_vertical_requirements{
            .min_size = overall_height,
            .growth_factor = alia_resolve_growth_factor(flow.flags),
            .ascent = overall_ascent,
            .descent = overall_height - overall_ascent});
}

static void
flow_place_line(
    alia_flow_fragment const* fragments,
    alia_flow_fragment_placement* placements,
    int first_fragment_index,
    int last_fragment_index,
    alia_vec2f position,
    float baseline,
    float spacer_padding)
{
    alia_flow_layout_run_frame run_stack[ALIA_FLOW_LAYOUT_RUN_STACK_CAPACITY];
    int run_depth = 0;
    alia_flow_layout_replay_run_stack_until(
        fragments, first_fragment_index, run_stack, run_depth);
    position.x += alia_flow_layout_run_line_start_offset(run_stack, run_depth);

    for (int i = first_fragment_index; i <= last_fragment_index; ++i)
    {
        alia_flow_fragment const& fragment = fragments[i];
        if (alia_flow_fragment_is_control(&fragment))
        {
            flow_apply_horizontal_control<false>(
                &fragment,
                i == first_fragment_index,
                position.x,
                run_stack,
                run_depth,
                nullptr);
            placements[i]
                = {.flags = ALIA_FLOW_FRAGMENT_PLACEMENT_SUPPRESSED,
                   .position = {0, 0},
                   .baseline = 0.f};
            continue;
        }

        float const spacer_extra
            = (fragment.flags & ALIA_FLOW_FRAGMENT_EXPANDABLE)
                ? spacer_padding
                : 0.f;

        placements[i]
            = {.flags = 0, .position = position, .baseline = baseline};

        position.x += fragment.content.size.x + spacer_extra;
    }
}

void
flow_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader);

void
flow_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& flow = *flow_from_node(node);
    auto& scratch = use_scratch<flow_scratch>(ctx->scratch);

    alia_flow_fragment* fragments = arena_alloc_array<alia_flow_fragment>(
        ctx->scratch, scratch.fragment_count);
    alia_flow_fragment_placement* placements
        = arena_alloc_array<alia_flow_fragment_placement>(
            ctx->scratch, scratch.fragment_count);

    auto const placement = alia_resolve_container_box(
        alia_fold_in_cross_axis_flags(flow.flags, main_axis),
        box.size,
        baseline,
        {scratch.max_fragment_width, scratch.overall_height},
        scratch.overall_ascent);

    if ((flow.flags & ALIA_PROVIDE_BOX) != 0)
    {
        alia_box* provided_box = arena_alloc<alia_box>(ctx->arena);
        provided_box->min = box.min + placement.min;
        provided_box->size = placement.size;
    }

    int const fragment_count = scratch.fragment_count;

    flow_line_wrapper_context wrapper_ctx
        = {.fragments = fragments,
           .fragment_count = fragment_count,
           .available_width = placement.size.x};

    flow_line_wrapper_state state;
    seed_flow_line_wrapper_state(state);

    float const x_assignment_base = box.min.x + placement.min.x;
    float current_y = box.min.y + placement.min.y;
    float current_line_gap = 0;

    int first_suppressed_fragment_index = 0;

    auto on_wrap = [&](bool is_explicit_break) {
        flow_finalize_line(state, &state.line);
        current_y += current_line_gap;
        current_line_gap = state.line_max_line_gap;

        for (int i = first_suppressed_fragment_index;
             i < state.first_anchor_index;
             ++i)
        {
            placements[i]
                = {.flags = ALIA_FLOW_FRAGMENT_PLACEMENT_SUPPRESSED,
                   .position = {0, 0},
                   .baseline = 0.f};
        }

        // Count the number of expandable fragments on the line.
        int expandable_count = 0;
        for (int i = state.first_anchor_index; i <= state.last_anchor_index;
             ++i)
        {
            if (fragments[i].flags & ALIA_FLOW_FRAGMENT_EXPANDABLE)
            {
                ALIA_ASSERT(!alia_flow_fragment_is_control(&fragments[i]));
                ++expandable_count;
            }
        }

        alia_layout_line_justification_spacing line_spacing
            = alia_layout_justify_line(
                is_explicit_break
                    ? alia_layout_justify_flags_for_incomplete_line(flow.flags)
                    : flow.flags,
                placement.size.x - state.anchored_width,
                // +1 because this parameter represents the number of items,
                // not the expandable spaces between them.
                expandable_count + 1);

        float const line_baseline = alia_resolve_baseline(
            flow.flags,
            state.line.height,
            state.line.ascent,
            state.line.descent);

        flow_place_line(
            fragments,
            placements,
            state.first_anchor_index,
            state.last_anchor_index,
            {x_assignment_base + line_spacing.before_items, current_y},
            line_baseline,
            line_spacing.between_items);

        first_suppressed_fragment_index = state.last_anchor_index + 1;

        current_y += state.line.height;
    };

    for (int i = 0; i < fragment_count; ++i)
        process_flow_fragment(wrapper_ctx, state, &fragments[i], i, on_wrap);
    // Process the final line.
    on_wrap(true);

    for (int i = first_suppressed_fragment_index; i < fragment_count; ++i)
    {
        placements[i]
            = {.flags = ALIA_FLOW_FRAGMENT_PLACEMENT_SUPPRESSED,
               .position = {0, 0},
               .baseline = 0.f};
    }

    alia_flow_fragment_reader reader
        = {.fragments = fragments,
           .placements = placements,
           .fragment_count = fragment_count,
           .index = 0};
    flow_read_fragment_placements(ctx, node, &reader);
}

alia_flow_emission_counts
flow_count_flow_emissions(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& flow = *flow_from_node(node);
    alia_flow_emission_counts totals = {0};
    int child_count = 0;
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        ++child_count;
        totals = alia_flow_emission_counts_add(
            totals, alia_count_flow_emissions(ctx, child));
    }
    return alia_flow_emission_counts_add_control_fragments(
        totals,
        alia_flow_inter_child_gap_count(child_count)
            + alia_flow_context_scope_control_count(child_count > 0));
}

void
flow_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter)
{
    flow_emit_all_fragments(ctx, *flow_from_node(node), emitter);
}

void
flow_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader)
{
    auto& flow = *flow_from_node(node);
    alia_layout_skip_flow_context_push_fragment(reader);

    bool first_child = true;
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling, first_child = false)
    {
        if (!first_child)
            alia_layout_skip_flow_gap_fragment(reader);
        alia_layout_read_fragment_placements(ctx, child, reader);
    }

    alia_layout_skip_flow_context_pop_fragment(reader);
}

alia_layout_node_vtable flow_vtable = {
    flow_measure_horizontal,
    flow_measure_vertical,
    flow_assign_boxes,
    flow_count_flow_emissions,
    flow_emit_flow_fragments,
    flow_read_fragment_placements,
};

} // namespace alia

extern "C" {

struct alia_layout_flow_scope
{
    alia::flow_layout_node* node;
};

void
alia_layout_flow_begin(
    alia_context* ctx,
    alia_layout_flags_t flags,
    float gap,
    float line_gap,
    float minimum_line_height)
{
    if (alia::is_refresh_event(*ctx))
    {
        auto& scope = alia::stack_push<alia_layout_flow_scope>(ctx);
        auto& emission = ctx->layout->emission;
        auto* node = alia::arena_alloc<alia::flow_layout_node>(emission.arena);
        scope.node = node;
        *node = alia::flow_layout_node{
            .base = {.vtable = &alia::flow_vtable, .next_sibling = 0},
            .flags = flags,
            .first_child = 0,
            .gap = gap,
            .line_gap = line_gap,
            .minimum_line_height = minimum_line_height};
        alia_layout_container_activate(ctx, alia::flow_as_container(node));
    }
}

void
alia_layout_flow_end(alia_context* ctx)
{
    if (alia::is_refresh_event(*ctx))
    {
        auto& scope = alia::stack_pop<alia_layout_flow_scope>(ctx);
        alia_layout_container_deactivate(
            ctx, alia::flow_as_container(scope.node));
    }
}

} // extern "C"
