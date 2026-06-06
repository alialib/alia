#ifndef ALIA_ABI_UI_LAYOUT_PROTOCOL_H
#define ALIA_ABI_UI_LAYOUT_PROTOCOL_H

#include <alia/abi/base/arena.h>
#include <alia/abi/base/geometry.h>

// Layout resolution consists of four phases:
// 1. Measure horizontal requirements.
// 2. Assign widths.
// 3. Measure vertical requirements based on the assigned widths.
// 4. Assign boxes.
//
// Note that #2 and #3 are typically combined into a single pass, but a
// separate interface is provided for specifically assigning widths in case it
// is useful for responsive layouts. (It isn't currently used or even fully
// implemented.)
//
// All passes start at the root node and recurse through descendants. The
// scratch space is used to store intermediate results. Each pass starts with
// the scratch allocator reset to the start of the scratch space, but the data
// itself is preserved across passes within a single layout resolution. Each
// pass is required to walk the tree in the same order as the previous pass.
// (i.e., Individual nodes must (re-)allocate the same data every pass, in the
// same order, and they must recursively visit each child in the same order.)

ALIA_EXTERN_C_BEGIN

typedef struct alia_layout_node_vtable alia_layout_node_vtable;

typedef struct alia_layout_node
{
    alia_layout_node_vtable* vtable;
    alia_layout_node* next_sibling;
} alia_layout_node;

typedef struct alia_horizontal_requirements
{
    float min_size;
    float growth_factor;
} alia_horizontal_requirements;

typedef struct alia_vertical_requirements
{
    float min_size;
    float growth_factor;
    float ascent;
    float descent;
} alia_vertical_requirements;

typedef struct alia_placement_context
{
    alia_bump_allocator scratch;
    alia_bump_allocator arena;
} alia_placement_context;

typedef struct alia_measurement_context
{
    alia_bump_allocator scratch;
} alia_measurement_context;

typedef struct alia_line_requirements
{
    float height;
    float ascent;
    float descent;
} alia_line_requirements;

typedef struct alia_vertical_assignment
{
    float line_height;
    float baseline_offset;
} alia_vertical_assignment;

typedef struct alia_flow_emission_counts
{
    int fragment_count;
    int run_count;
} alia_flow_emission_counts;

typedef struct alia_flow_run_style
{
    alia_insets padding;
} alia_flow_run_style;

typedef uint16_t alia_flow_run_index;

typedef uint8_t alia_flow_fragment_kind;
#define ALIA_FLOW_FRAGMENT_CONTENT 0
#define ALIA_FLOW_FRAGMENT_SPACER 1
#define ALIA_FLOW_FRAGMENT_BREAK 2

typedef uint16_t alia_flow_fragment_flags;
#define ALIA_FLOW_FRAGMENT_FIXED_WIDTH (1u << 0)
#define ALIA_FLOW_FRAGMENT_COLLAPSE_EDGE (1u << 1)

typedef struct alia_flow_fragment
{
    alia_flow_fragment_kind kind;
    alia_flow_fragment_flags flags;
    // index into the flow run table (0 is a reserved default run)
    alia_flow_run_index run_index;
    float width;
    float height;
    float ascent;
    float descent;
} alia_flow_fragment;

typedef struct alia_flow_fragment_emitter
{
    alia_flow_fragment* fragments;
    // number of fragments emitted so far
    int fragment_count;
    alia_flow_run_style* runs;
    // TODO: This is only used for bounds checking. It could be conditionally
    // included for debug builds. We should also track the fragment capacity.
    int run_capacity;
    // number of runs registered so far
    int run_count;
    uint16_t active_run_index;
} alia_flow_fragment_emitter;

typedef struct alia_flow_fragment_placement
{
    alia_vec2f position;
    float baseline;
} alia_flow_fragment_placement;

typedef struct alia_flow_fragment_reader
{
    alia_flow_fragment const* fragments;
    alia_flow_fragment_placement const* placements;
    int index;
} alia_flow_fragment_reader;

typedef uint8_t alia_main_axis_index;
#define ALIA_MAIN_AXIS_X 3
#define ALIA_MAIN_AXIS_Y 6

typedef struct alia_layout_node_vtable
{
    alia_horizontal_requirements (*measure_horizontal)(
        alia_measurement_context* ctx, alia_layout_node* node);

    void (*assign_widths)(
        alia_placement_context* ctx,
        alia_main_axis_index main_axis,
        alia_layout_node* node,
        float assigned_width);

    alia_vertical_requirements (*measure_vertical)(
        alia_measurement_context* ctx,
        alia_main_axis_index main_axis,
        alia_layout_node* node,
        float assigned_width);

    void (*assign_boxes)(
        alia_placement_context* ctx,
        alia_main_axis_index main_axis,
        alia_layout_node* node,
        alia_box box,
        float baseline);

    // NOTE: This shouldn't move the scratch pointer.
    alia_flow_emission_counts (*count_flow_emissions)(
        alia_measurement_context* ctx, alia_layout_node* node);

    void (*emit_flow_fragments)(
        alia_measurement_context* ctx,
        alia_layout_node* node,
        alia_flow_fragment_emitter* emitter);

    void (*read_fragment_placements)(
        alia_placement_context* ctx,
        alia_layout_node* node,
        alia_flow_fragment_reader* reader);

} alia_layout_node_vtable;

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_LAYOUT_PROTOCOL_H */
