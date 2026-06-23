#ifndef ALIA_ABI_UI_LAYOUT_UTILITIES_FLOW_H
#define ALIA_ABI_UI_LAYOUT_UTILITIES_FLOW_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/layout/protocol.h>
#include <alia/abi/ui/style.h>

ALIA_EXTERN_C_BEGIN

#define ALIA_FLOW_LAYOUT_RUN_STACK_CAPACITY 16

// Horizontal run state reconstructed while walking the fragment stream during
// layout and bounds scans. Vertical inset is folded into content at emit time.
// stack[0] is a permanent root frame (zero offsets); depth is always >= 1.
typedef struct alia_flow_layout_run_frame
{
    alia_edge_offsets offsets;
    float total_left;
    float total_right;
} alia_flow_layout_run_frame;

static inline void
alia_flow_layout_run_stack_seed_root(
    alia_flow_layout_run_frame* stack, int& depth)
{
    stack[0] = alia_flow_layout_run_frame{
        .offsets = {0}, .total_left = 0.f, .total_right = 0.f};
    depth = 1;
}

static inline float
alia_flow_layout_run_total_right(
    alia_flow_layout_run_frame const* stack, int depth)
{
    ALIA_ASSERT(depth >= 1);
    return stack[depth - 1].total_right;
}

static inline float
alia_flow_layout_run_line_start_offset(
    alia_flow_layout_run_frame const* stack, int depth)
{
    float offset = 0.f;
    for (int i = 0; i < depth; ++i)
        offset += stack[i].offsets.left;
    return offset;
}

static inline float
alia_flow_layout_content_horizontal_extent(
    float content_width,
    alia_flow_layout_run_frame const* stack,
    int depth)
{
    ALIA_ASSERT(depth >= 1);
    return content_width + stack[depth - 1].total_right
         + stack[depth - 1].total_left;
}

static inline void
alia_flow_layout_run_push_frame(
    alia_flow_layout_run_frame* stack, int& depth, alia_edge_offsets offsets)
{
    ALIA_ASSERT(depth >= 1);
    ALIA_ASSERT(depth < ALIA_FLOW_LAYOUT_RUN_STACK_CAPACITY);
    float const parent_left = stack[depth - 1].total_left;
    float const parent_right = stack[depth - 1].total_right;
    stack[depth] = alia_flow_layout_run_frame{
        .offsets = offsets,
        .total_left = parent_left + offsets.left,
        .total_right = parent_right + offsets.right};
    ++depth;
}

static inline void
alia_flow_layout_run_pop_frame(
    alia_flow_layout_run_frame* stack, int& depth)
{
    (void) stack;
    ALIA_ASSERT(depth > 1);
    --depth;
}

static inline void
alia_flow_layout_run_push(
    alia_flow_layout_run_frame* stack,
    int& depth,
    alia_edge_offsets offsets,
    float& current_x)
{
    alia_flow_layout_run_push_frame(stack, depth, offsets);
    current_x += offsets.left;
}

static inline void
alia_flow_layout_run_pop(
    alia_flow_layout_run_frame* stack, int& depth, float& current_x)
{
    ALIA_ASSERT(depth > 1);
    alia_edge_offsets const offsets = stack[depth - 1].offsets;
    alia_flow_layout_run_pop_frame(stack, depth);
    current_x += offsets.right;
}

static inline void
alia_flow_layout_replay_run_stack_until(
    alia_flow_fragment const* fragments,
    int end_index,
    alia_flow_layout_run_frame* stack,
    int& depth)
{
    alia_flow_layout_run_stack_seed_root(stack, depth);
    for (int i = 0; i < end_index; ++i)
    {
        alia_flow_fragment const& fragment = fragments[i];
        switch (fragment.kind)
        {
            case ALIA_FLOW_FRAGMENT_KIND_RUN_PUSH:
                alia_flow_layout_run_push_frame(
                    stack, depth, fragment.run.offsets);
                break;
            case ALIA_FLOW_FRAGMENT_KIND_RUN_POP:
                alia_flow_layout_run_pop_frame(stack, depth);
                break;
            default:
                break;
        }
    }
}

static inline void
alia_flow_emitter_run_stack_seed_root(alia_flow_fragment_emitter* emitter)
{
    emitter->run_stack[0] = alia_flow_emitter_run_frame{
        .offsets = {0},
        .cumulative_top = 0.f,
        .cumulative_bottom = 0.f};
    emitter->run_stack_depth = 1;
}

static inline void
alia_flow_emitter_run_stack_push(
    alia_flow_fragment_emitter* emitter, alia_edge_offsets offsets)
{
    int const depth = emitter->run_stack_depth;
    ALIA_ASSERT(depth >= 1);
    ALIA_ASSERT(depth < ALIA_FLOW_EMITTER_RUN_STACK_CAPACITY);
    float const parent_top = emitter->run_stack[depth - 1].cumulative_top;
    float const parent_bottom
        = emitter->run_stack[depth - 1].cumulative_bottom;
    emitter->run_stack[depth] = alia_flow_emitter_run_frame{
        .offsets = offsets,
        .cumulative_top = parent_top + offsets.top,
        .cumulative_bottom = parent_bottom + offsets.bottom};
    ++emitter->run_stack_depth;
}

static inline void
alia_flow_emitter_run_stack_pop(alia_flow_fragment_emitter* emitter)
{
    ALIA_ASSERT(emitter->run_stack_depth > 1);
    --emitter->run_stack_depth;
}

static inline alia_line_requirements
alia_layout_line_requirements_with_run_offsets(
    alia_flow_fragment_emitter const* emitter, alia_line_requirements line)
{
    ALIA_ASSERT(emitter->run_stack_depth >= 1);
    alia_flow_emitter_run_frame const* const frame
        = &emitter->run_stack[emitter->run_stack_depth - 1];
    line.height += frame->cumulative_top + frame->cumulative_bottom;
    line.ascent += frame->cumulative_top;
    line.descent += frame->cumulative_bottom;
    return line;
}

static inline bool
alia_flow_fragment_is_control(alia_flow_fragment const* fragment)
{
    return fragment->kind != ALIA_FLOW_FRAGMENT_KIND_CONTENT;
}

static inline bool
alia_flow_fragment_is_content(alia_flow_fragment const* fragment)
{
    return fragment->kind == ALIA_FLOW_FRAGMENT_KIND_CONTENT;
}

static inline alia_flow_fragment_content_payload const*
alia_flow_fragment_content(alia_flow_fragment const* fragment)
{
    ALIA_ASSERT(fragment->kind == ALIA_FLOW_FRAGMENT_KIND_CONTENT);
    return &fragment->content;
}

static inline alia_flow_fragment_content_payload*
alia_flow_fragment_content_mut(alia_flow_fragment* fragment)
{
    ALIA_ASSERT(fragment->kind == ALIA_FLOW_FRAGMENT_KIND_CONTENT);
    return &fragment->content;
}

static inline void
alia_layout_emit_flow_fragment_raw(
    alia_flow_fragment_emitter* emitter, alia_flow_fragment fragment)
{
    emitter->fragments[emitter->fragment_count] = fragment;
    ++emitter->fragment_count;
}

static inline void
alia_flow_emit_gap_control(alia_flow_fragment_emitter* emitter, float gap)
{
    alia_layout_emit_flow_fragment_raw(
        emitter,
        alia_flow_fragment{
            .flags = ALIA_FLOW_FRAGMENT_CONTROL_BASE_FLAGS,
            .kind = ALIA_FLOW_FRAGMENT_KIND_GAP,
            .gap = {.gap = gap}});
}

static inline void
alia_flow_emit_context_push_control(
    alia_flow_fragment_emitter* emitter,
    float line_gap,
    float minimum_line_height)
{
    alia_layout_emit_flow_fragment_raw(
        emitter,
        alia_flow_fragment{
            .flags = ALIA_FLOW_FRAGMENT_CONTROL_BASE_FLAGS,
            .kind = ALIA_FLOW_FRAGMENT_KIND_CONTEXT_PUSH,
            .context
            = {.line_gap = line_gap,
               .minimum_line_height = minimum_line_height}});
}

static inline void
alia_flow_emit_context_pop_control(alia_flow_fragment_emitter* emitter)
{
    alia_layout_emit_flow_fragment_raw(
        emitter,
        alia_flow_fragment{
            .flags = ALIA_FLOW_FRAGMENT_CONTROL_BASE_FLAGS,
            .kind = ALIA_FLOW_FRAGMENT_KIND_CONTEXT_POP});
}

static inline void
alia_flow_emit_run_push_control(
    alia_flow_fragment_emitter* emitter, alia_edge_offsets offsets)
{
    alia_flow_emitter_run_stack_push(emitter, offsets);
    alia_layout_emit_flow_fragment_raw(
        emitter,
        alia_flow_fragment{
            .flags = ALIA_FLOW_FRAGMENT_CONTROL_BASE_FLAGS,
            .kind = ALIA_FLOW_FRAGMENT_KIND_RUN_PUSH,
            .run = {.offsets = offsets}});
}

static inline void
alia_flow_emit_run_pop_control(alia_flow_fragment_emitter* emitter)
{
    alia_layout_emit_flow_fragment_raw(
        emitter,
        alia_flow_fragment{
            .flags = ALIA_FLOW_FRAGMENT_CONTROL_BASE_FLAGS,
            .kind = ALIA_FLOW_FRAGMENT_KIND_RUN_POP});
    alia_flow_emitter_run_stack_pop(emitter);
}

static inline int
alia_flow_inter_child_gap_count(int child_count)
{
    return child_count > 1 ? child_count - 1 : 0;
}

static inline int
alia_flow_context_scope_control_count(bool has_children)
{
    return has_children ? 2 : 0;
}

static inline int
alia_flow_run_scope_control_count(void)
{
    return 2;
}

static inline alia_flow_emission_counts
alia_flow_emission_counts_add_control_fragments(
    alia_flow_emission_counts counts, int control_fragment_count)
{
    counts.fragment_count += control_fragment_count;
    return counts;
}

static inline alia_flow_emission_counts
alia_flow_emission_counts_with_run_scope(alia_flow_emission_counts counts)
{
    return alia_flow_emission_counts_add_control_fragments(
        counts, alia_flow_run_scope_control_count());
}

static inline void
alia_layout_emit_flow_fragment(
    alia_flow_fragment_emitter* emitter, alia_flow_fragment fragment)
{
    ALIA_ASSERT(fragment.kind == ALIA_FLOW_FRAGMENT_KIND_CONTENT);
    alia_layout_emit_flow_fragment_raw(emitter, fragment);
}

static inline alia_flow_fragment const*
alia_layout_read_fragment_spec(alia_flow_fragment_reader const* reader)
{
    return &reader->fragments[reader->index];
}

static inline alia_flow_fragment_placement const*
alia_layout_read_fragment_placement(alia_flow_fragment_reader const* reader)
{
    return &reader->placements[reader->index];
}

static inline void
alia_layout_advance_fragment(alia_flow_fragment_reader* reader)
{
    ++reader->index;
}

static inline void
alia_layout_skip_flow_fragment_if_kind(
    alia_flow_fragment_reader* reader, alia_flow_fragment_kind kind)
{
    if (reader->index < reader->fragment_count
        && reader->fragments[reader->index].kind == kind)
    {
        alia_layout_advance_fragment(reader);
    }
}

static inline void
alia_layout_skip_flow_gap_fragment(alia_flow_fragment_reader* reader)
{
    alia_layout_skip_flow_fragment_if_kind(
        reader, ALIA_FLOW_FRAGMENT_KIND_GAP);
}

static inline void
alia_layout_skip_flow_context_push_fragment(alia_flow_fragment_reader* reader)
{
    alia_layout_skip_flow_fragment_if_kind(
        reader, ALIA_FLOW_FRAGMENT_KIND_CONTEXT_PUSH);
}

static inline void
alia_layout_skip_flow_context_pop_fragment(alia_flow_fragment_reader* reader)
{
    alia_layout_skip_flow_fragment_if_kind(
        reader, ALIA_FLOW_FRAGMENT_KIND_CONTEXT_POP);
}

static inline void
alia_layout_skip_flow_run_push_fragment(alia_flow_fragment_reader* reader)
{
    alia_layout_skip_flow_fragment_if_kind(
        reader, ALIA_FLOW_FRAGMENT_KIND_RUN_PUSH);
}

static inline void
alia_layout_skip_flow_run_pop_fragment(alia_flow_fragment_reader* reader)
{
    alia_layout_skip_flow_fragment_if_kind(
        reader, ALIA_FLOW_FRAGMENT_KIND_RUN_POP);
}

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_LAYOUT_UTILITIES_FLOW_H
