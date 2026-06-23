#ifndef ALIA_ABI_UI_LAYOUT_PROTOCOL_H
#define ALIA_ABI_UI_LAYOUT_PROTOCOL_H

#include <alia/abi/base/arena.h>
#include <alia/abi/base/geometry.h>

// Layout resolution consists of three phases:
// 1. Measure horizontal requirements.
// 2. Assign widths and measure vertical requirements based on those.
// 3. Assign boxes.
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

typedef struct alia_flow_emission_counts
{
    int fragment_count;
} alia_flow_emission_counts;

typedef uint16_t alia_flow_fragment_flags;

typedef uint8_t alia_flow_fragment_kind;

// Resolution within a flow layout container uses a different protocol
// involving a single stream of fragments. (Nested flows inject their fragments
// into the same stream. Non-flow containers are emitted as single fragments.)

// flow fragment kinds:
// `X(value, name)`
#define ALIA_FLOW_FRAGMENT_KINDS(X)                                           \
    /* CONTENT: a measurable/placable item (text token, leaf box, etc.) */    \
    X(0, CONTENT)                                                             \
    /* GAP: horizontal space between flow children */                         \
    X(1, GAP)                                                                 \
    /* CONTEXT_PUSH / CONTEXT_POP: push/pop - Nested flows emit these to */   \
    /* mark their scope within the stack of `line_gap` and */                 \
    /* `minimum_line_height` properties. */                                   \
    X(2, CONTEXT_PUSH)                                                        \
    X(3, CONTEXT_POP)                                                         \
    /* RUN_PUSH / RUN_POP: run scoping (to track edge offsets) */             \
    X(4, RUN_PUSH)                                                            \
    X(5, RUN_POP)

// clang-format off
enum
{
    #define X(value, name) ALIA_FLOW_FRAGMENT_KIND_##name = value,
    ALIA_FLOW_FRAGMENT_KINDS(X)
    #undef X
};
// clang-format on

// content fragment flags
// `X(value, name)`
#define ALIA_FLOW_FRAGMENT_FLAGS(X)                                           \
    X((1u << 0), OMIT_FROM_BOUNDS)                                            \
    X((1u << 1), EXPANDABLE)                                                  \
    X((1u << 2), SUPPRESS_AT_LINE_START)                                      \
    X((1u << 3), SUPPRESS_AT_LINE_END)                                        \
    X((1u << 4), BREAK_AFTER)

// clang-format off
enum
{
    #define X(value, name) ALIA_FLOW_FRAGMENT_##name = value,
    ALIA_FLOW_FRAGMENT_FLAGS(X)
    #undef X

    ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_EDGES =
        ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_START |
        ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_END
};
// clang-format on

// Non-content "control" fragments carry these flags in addition to their kind.
// Layout treats them as non-anchors and omits them from bounds scans.
#define ALIA_FLOW_FRAGMENT_CONTROL_BASE_FLAGS                                 \
    (ALIA_FLOW_FRAGMENT_OMIT_FROM_BOUNDS                                      \
     | ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_START                              \
     | ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_END)

typedef struct alia_flow_fragment_content_payload
{
    float width;
    float height;
    float ascent;
    float descent;
} alia_flow_fragment_content_payload;

typedef struct alia_flow_fragment_gap_payload
{
    float gap;
} alia_flow_fragment_gap_payload;

typedef struct alia_flow_fragment_context_payload
{
    float line_gap;
    float minimum_line_height;
} alia_flow_fragment_context_payload;

typedef struct alia_flow_fragment_run_payload
{
    alia_edge_offsets offsets;
} alia_flow_fragment_run_payload;

typedef struct alia_flow_fragment
{
    alia_flow_fragment_flags flags;
    alia_flow_fragment_kind kind;
    union
    {
        alia_flow_fragment_content_payload content;
        alia_flow_fragment_gap_payload gap;
        alia_flow_fragment_context_payload context;
        alia_flow_fragment_run_payload run;
    };
} alia_flow_fragment;

#define ALIA_FLOW_EMITTER_RUN_STACK_CAPACITY 16

typedef struct alia_flow_emitter_run_frame
{
    alia_edge_offsets offsets;
    float cumulative_top;
    float cumulative_bottom;
} alia_flow_emitter_run_frame;

typedef struct alia_flow_fragment_emitter
{
    alia_flow_fragment* fragments;
    // number of fragments emitted so far
    int fragment_count;
    alia_flow_emitter_run_frame
        run_stack[ALIA_FLOW_EMITTER_RUN_STACK_CAPACITY];
    int run_stack_depth;
    // child gap from the enclosing flow; used when emitting siblings inside
    // pass-through wrappers such as edge_offsets.
    float child_gap;
} alia_flow_fragment_emitter;

typedef uint32_t alia_flow_fragment_placement_flags;
#define ALIA_FLOW_FRAGMENT_PLACEMENT_FLAGS(X) X((1u << 0), SUPPRESSED)

// clang-format off
enum
{
    #define X(value, name) ALIA_FLOW_FRAGMENT_PLACEMENT_##name = value,
    ALIA_FLOW_FRAGMENT_PLACEMENT_FLAGS(X)
    #undef X
};
// clang-format on

typedef struct alia_flow_fragment_placement
{
    alia_flow_fragment_placement_flags flags;
    alia_vec2f position;
    float baseline;
} alia_flow_fragment_placement;

typedef struct alia_flow_fragment_reader
{
    alia_flow_fragment const* fragments;
    alia_flow_fragment_placement const* placements;
    int fragment_count;
    int index;
} alia_flow_fragment_reader;

typedef uint8_t alia_main_axis_index;
#define ALIA_MAIN_AXIS_X 3
#define ALIA_MAIN_AXIS_Y 6

typedef struct alia_layout_node_vtable
{
    alia_horizontal_requirements (*measure_horizontal)(
        alia_measurement_context* ctx, alia_layout_node* node);

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
