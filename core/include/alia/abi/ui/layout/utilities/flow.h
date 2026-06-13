#ifndef ALIA_ABI_UI_LAYOUT_UTILITIES_FLOW_H
#define ALIA_ABI_UI_LAYOUT_UTILITIES_FLOW_H

#include <alia/abi/prelude.h>
#include <alia/abi/ui/layout/protocol.h>
#include <alia/abi/ui/style.h>

ALIA_EXTERN_C_BEGIN

static inline alia_flow_run_index
alia_flow_run_lowest_common_ancestor(
    alia_flow_run_style const* runs,
    alia_flow_run_index a,
    alia_flow_run_index b)
{
    while (a != b)
    {
        if (a > b)
            a = runs[a].parent;
        else
            b = runs[b].parent;
    }
    return a;
}

static inline float
alia_flow_run_sum_right_to_ancestor(
    alia_flow_run_style const* runs,
    alia_flow_run_index run,
    alia_flow_run_index ancestor)
{
    float total = 0.f;
    while (run != ancestor)
    {
        total += runs[run].offsets.right;
        run = runs[run].parent;
    }
    return total;
}

static inline float
alia_flow_run_sum_left_to_ancestor(
    alia_flow_run_style const* runs,
    alia_flow_run_index run,
    alia_flow_run_index ancestor)
{
    float total = 0.f;
    while (run != ancestor)
    {
        total += runs[run].offsets.left;
        run = runs[run].parent;
    }
    return total;
}

static inline float
alia_flow_run_sum_top_to_ancestor(
    alia_flow_run_style const* runs,
    alia_flow_run_index run,
    alia_flow_run_index ancestor)
{
    float total = 0.f;
    while (run != ancestor)
    {
        total += runs[run].offsets.top;
        run = runs[run].parent;
    }
    return total;
}

static inline float
alia_flow_run_sum_bottom_to_ancestor(
    alia_flow_run_style const* runs,
    alia_flow_run_index run,
    alia_flow_run_index ancestor)
{
    float total = 0.f;
    while (run != ancestor)
    {
        total += runs[run].offsets.bottom;
        run = runs[run].parent;
    }
    return total;
}

static inline float
alia_flow_run_total_left(
    alia_flow_run_style const* runs, alia_flow_run_index run)
{
    return alia_flow_run_sum_left_to_ancestor(runs, run, 0);
}

static inline float
alia_flow_run_total_right(
    alia_flow_run_style const* runs, alia_flow_run_index run)
{
    return alia_flow_run_sum_right_to_ancestor(runs, run, 0);
}

static inline float
alia_flow_run_total_top(
    alia_flow_run_style const* runs, alia_flow_run_index run)
{
    return alia_flow_run_sum_top_to_ancestor(runs, run, 0);
}

static inline float
alia_flow_run_total_bottom(
    alia_flow_run_style const* runs, alia_flow_run_index run)
{
    return alia_flow_run_sum_bottom_to_ancestor(runs, run, 0);
}

static inline float
alia_flow_run_transition_offset(
    alia_flow_run_style const* runs,
    alia_flow_run_index from,
    alia_flow_run_index to)
{
    if (from == to)
        return 0.f;
    alia_flow_run_index const ancestor
        = alia_flow_run_lowest_common_ancestor(runs, from, to);
    return alia_flow_run_sum_right_to_ancestor(runs, from, ancestor)
         + alia_flow_run_sum_left_to_ancestor(runs, to, ancestor);
}

static inline alia_line_requirements
alia_layout_line_requirements_with_run_offsets(
    alia_flow_fragment_emitter const* emitter, alia_line_requirements line)
{
    for (alia_flow_run_index run = emitter->active_run_index; run != 0;
         run = emitter->runs[run].parent)
    {
        alia_edge_offsets const offsets = emitter->runs[run].offsets;
        line.height += offsets.top + offsets.bottom;
        line.ascent += offsets.top;
        line.descent += offsets.bottom;
    }
    return line;
}

static inline void
alia_layout_emit_flow_fragment_raw(
    alia_flow_fragment_emitter* emitter, alia_flow_fragment fragment)
{
    emitter->fragments[emitter->fragment_count] = fragment;
    ++emitter->fragment_count;
}

static inline void
alia_layout_emit_flow_fragment(
    alia_flow_fragment_emitter* emitter, alia_flow_fragment fragment)
{
    for (alia_flow_run_index run = fragment.run_index; run != 0;
         run = emitter->runs[run].parent)
    {
        alia_edge_offsets const offsets = emitter->runs[run].offsets;
        fragment.height += offsets.top + offsets.bottom;
        fragment.ascent += offsets.top;
        fragment.descent += offsets.bottom;
    }
    alia_layout_emit_flow_fragment_raw(emitter, fragment);
}

// Registers a new run style and returns its index into the run table.
static inline alia_flow_run_index
alia_flow_register_run(
    alia_flow_fragment_emitter* emitter, alia_flow_run_style style)
{
    int const index = emitter->run_count;
    ALIA_ASSERT(index < emitter->run_capacity);
    emitter->runs[index] = style;
    ++emitter->run_count;
    return (alia_flow_run_index) index;
}

// Registers a child run whose parent is the emitter's active run.
static inline alia_flow_run_index
alia_flow_register_child_run(
    alia_flow_fragment_emitter* emitter, alia_flow_run_style style)
{
    style.parent = emitter->active_run_index;
    return alia_flow_register_run(emitter, style);
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

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_LAYOUT_UTILITIES_FLOW_H
