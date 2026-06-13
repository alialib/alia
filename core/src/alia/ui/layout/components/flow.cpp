#include <alia/abi/ui/layout/utilities/flow.h>
#include <alia/abi/ui/layout/utilities/line.h>
#include <alia/abi/ui/style.h>
#include <alia/impl/ui/layout.hpp>

using namespace alia::operators;

namespace alia {

using flow_layout_node = alia_layout_container;

struct flow_scratch
{
    int fragment_count = 0;
    int run_count = 0;
    float max_fragment_width = 0;
    float overall_height = 0, overall_ascent = 0;
    alia_arena_marker scratch_end = {};
};

struct flow_line_wrapper_context
{
    alia_flow_run_style const* runs;
    alia_flow_fragment const* fragments;
    int fragment_count;
    float available_width;
};

struct flow_line_wrapper_state
{
    alia_flow_run_index active_run_index = 0;

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
};

static void
flow_reset_line(flow_line_wrapper_state& state)
{
    state.have_anchors = false;
    state.first_anchor_index = 0;
    state.last_anchor_index = 0;
    state.anchored_width = 0.f;
    state.active_run_index = 0;
    state.current_x = 0.f;
    alia_layout_line_reset(&state.line);
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
    float fragment_width = fragment->width;

    if (!state.have_anchors)
    {
        if (fragment->flags & ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_START)
            return;
        // Otherwise, this is the first anchor on this line.
        state.have_anchors = true;
        state.first_anchor_index = fragment_index;
    }

    // If this fragment is in a different run than the previous fragment, we
    // need to add edge offsets to end the previous run and adjust the fragment
    // width to include the edge offsets that start its run. Note that all
    // lines start with an active run index of 0 (which has no edge offsets),
    // so this also takes care of adding edge offsets at the start of the line
    // where needed.
    alia_flow_run_index const run_index = fragment->run_index;
    if (run_index != state.active_run_index)
    {
        state.current_x += alia_flow_run_transition_offset(
            ctx.runs, state.active_run_index, run_index);
        state.active_run_index = run_index;
    }

    // Check to see if the fragment will cause the line to overflow.
    float const end_of_run_edge_offsets
        = alia_flow_run_total_right(ctx.runs, run_index);
    // Note that we only allow wrapping to occur if we already have anchors.
    // This prevents large fragments from throwing us into an infinite wrapping
    // loop. (In theory this shouldn't happen with the current width
    // calculation logic, so this is mostly a check for the future and/or a
    // guard against floating point imprecision errors.)
    if (state.have_anchors
        && state.current_x + fragment_width + end_of_run_edge_offsets
               > ctx.available_width)
    {
        alia_layout_line_finalize_height(&state.line);
        on_wrap(false);
        flow_reset_line(state);
        // We're at the start of a new line now, so we need to again apply the
        // start-of-line suppression/anchor logic.
        if (fragment->flags & ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_START)
            return;
        state.have_anchors = true;
        state.first_anchor_index = fragment_index;
        // Also apply the start-of-line run transition logic.
        if (run_index != state.active_run_index)
        {
            state.current_x += alia_flow_run_transition_offset(
                ctx.runs, state.active_run_index, run_index);
            state.active_run_index = run_index;
        }
    }

    alia_layout_line_fold_in_fragment(&state.line, fragment);

    state.current_x += fragment_width;

    // Update the anchor tracking.
    if (!(fragment->flags & ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_END))
    {
        state.last_anchor_index = fragment_index;
        state.anchored_width = state.current_x + end_of_run_edge_offsets;
    }

    if (fragment->flags & ALIA_FLOW_FRAGMENT_BREAK_AFTER)
    {
        alia_layout_line_finalize_height(&state.line);
        on_wrap(true);
        flow_reset_line(state);
    }
}

alia_horizontal_requirements
flow_measure_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = claim_scratch<flow_scratch>(ctx->scratch);

    // Count the total number of fragments and runs that will be emitted. Note
    // that the layout protocol requires that these functions don't touch the
    // scratch space.
    alia_flow_emission_counts totals = {0, 0};
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        totals = alia_flow_emission_counts_add(
            totals, alia_count_flow_emissions(ctx, child));
    }
    scratch.fragment_count = totals.fragment_count;
    scratch.run_count = totals.run_count + 1;

    // Allocate our fragment and run arrays. (Again, note that we are at the
    // start of our scratch space.)
    alia_flow_fragment* fragments = arena_alloc_array<alia_flow_fragment>(
        ctx->scratch, totals.fragment_count);
    alia_flow_run_style* runs = arena_alloc_array<alia_flow_run_style>(
        ctx->scratch, scratch.run_count);
    // Reserve space for the fragment placements.
    arena_alloc_array<alia_flow_fragment_placement>(
        ctx->scratch, totals.fragment_count);

    // Invoke the children to emit their fragments and runs.
    runs[0] = alia_flow_run_style{};
    alia_flow_fragment_emitter emitter
        = {.fragments = fragments,
           .fragment_count = 0,
           .runs = runs,
           .run_capacity = scratch.run_count,
           .run_count = 1,
           .active_run_index = 0};
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        alia_emit_flow_fragments(ctx, child, &emitter);
    }

    // Mark the end of the scratch space for this overall node.
    scratch.scratch_end = alia_arena_mark(&ctx->scratch);

    // Compute the maximum fragment width.
    // TODO: Add a flag that bypasses this computation and allows the flow to
    // be smaller than its largest fragment.
    float max_fragment_width = 0;
    for (int i = 0; i < totals.fragment_count; ++i)
    {
        auto const& fragment = fragments[i];
        max_fragment_width = alia_max(
            max_fragment_width,
            fragment.width + alia_flow_run_total_left(runs, fragment.run_index)
                + alia_flow_run_total_right(runs, fragment.run_index));
    }
    scratch.max_fragment_width = max_fragment_width;

    return alia_horizontal_requirements{
        .min_size = max_fragment_width,
        .growth_factor = alia_resolve_growth_factor(flow.flags)};
}

void
flow_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    (void) ctx;
    (void) main_axis;
    (void) node;
    (void) assigned_width;
}

alia_vertical_requirements
flow_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = use_scratch<flow_scratch>(ctx->scratch);

    alia_flow_fragment* fragments = arena_alloc_array<alia_flow_fragment>(
        ctx->scratch, scratch.fragment_count);
    alia_flow_run_style* runs = arena_alloc_array<alia_flow_run_style>(
        ctx->scratch, scratch.run_count);

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
        = {.runs = runs,
           .fragments = fragments,
           .fragment_count = fragment_count,
           .available_width = assignment.size};

    flow_line_wrapper_state state = {};

    bool wrapping_has_occurred = false;
    float overall_height = 0.f, overall_ascent = 0.f;

    auto on_wrap = [&](bool is_explicit_break) {
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

    return alia_vertical_requirements{
        .min_size = overall_height,
        .growth_factor = alia_resolve_growth_factor(flow.flags),
        .ascent = (flow.flags & ALIA_Y_ALIGNMENT_MASK) == ALIA_BASELINE_Y
                    ? overall_ascent
                    : 0.0f,
        .descent = (flow.flags & ALIA_Y_ALIGNMENT_MASK) == ALIA_BASELINE_Y
                     ? overall_height - overall_ascent
                     : 0.0f};
}

static void
flow_place_line(
    flow_line_wrapper_context const& ctx,
    alia_flow_fragment_placement* placements,
    int first_fragment_index,
    int last_fragment_index,
    alia_vec2f position,
    float baseline,
    float spacer_padding)
{
    alia_flow_run_index active_run_index = 0;
    for (int i = first_fragment_index; i <= last_fragment_index; ++i)
    {
        alia_flow_fragment const& fragment = ctx.fragments[i];

        alia_flow_run_index const run_index = fragment.run_index;
        if (run_index != active_run_index)
        {
            position.x += alia_flow_run_transition_offset(
                ctx.runs, active_run_index, run_index);
            active_run_index = run_index;
        }

        placements[i]
            = {.flags = 0, .position = position, .baseline = baseline};

        float const spacer_extra
            = (fragment.flags & ALIA_FLOW_FRAGMENT_EXPANDABLE)
                ? spacer_padding
                : 0.f;

        position.x += fragment.width + spacer_extra;
    }
}

void
flow_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    auto& scratch = use_scratch<flow_scratch>(ctx->scratch);

    alia_flow_fragment* fragments = arena_alloc_array<alia_flow_fragment>(
        ctx->scratch, scratch.fragment_count);
    alia_flow_run_style* runs = arena_alloc_array<alia_flow_run_style>(
        ctx->scratch, scratch.run_count);
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
        = {.runs = runs,
           .fragments = fragments,
           .fragment_count = fragment_count,
           .available_width = placement.size.x};

    flow_line_wrapper_state state = {};

    float const x_assignment_base = box.min.x + placement.min.x;
    float current_y = box.min.y + placement.min.y;

    int first_suppressed_fragment_index = 0;

    auto on_wrap = [&](bool is_explicit_break) {
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
                ++expandable_count;
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
            wrapper_ctx,
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
           .runs = runs,
           .run_count = scratch.run_count,
           .index = 0};
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        alia_layout_read_fragment_placements(ctx, child, &reader);
    }
}

alia_flow_emission_counts
flow_count_flow_emissions(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    alia_flow_emission_counts totals = {0, 0};
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        totals = alia_flow_emission_counts_add(
            totals, alia_count_flow_emissions(ctx, child));
    }
    return totals;
}

void
flow_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        alia_emit_flow_fragments(ctx, child, emitter);
    }
}

void
flow_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader)
{
    auto& flow = *reinterpret_cast<flow_layout_node*>(node);
    for (alia_layout_node* child = flow.first_child; child != nullptr;
         child = child->next_sibling)
    {
        alia_layout_read_fragment_placements(ctx, child, reader);
    }
}

alia_layout_node_vtable flow_vtable = {
    flow_measure_horizontal,
    flow_assign_widths,
    flow_measure_vertical,
    flow_assign_boxes,
    flow_count_flow_emissions,
    flow_emit_flow_fragments,
    flow_read_fragment_placements,
};

} // namespace alia

extern "C" {

void
alia_layout_flow_begin(alia_context* ctx, alia_layout_flags_t flags)
{
    alia_layout_container_simple_begin(ctx, &alia::flow_vtable, flags, 0.f);
}

void
alia_layout_flow_end(alia_context* ctx)
{
    alia_layout_container_simple_end(ctx);
}

} // extern "C"
