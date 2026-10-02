#ifndef ALIA_ABI_UI_LAYOUT_FLAGS_H
#define ALIA_ABI_UI_LAYOUT_FLAGS_H

#include <alia/abi/prelude.h>

typedef uint32_t alia_layout_flags_t;

// layout placement flags - These control how a node sits in its parent (and
// related leaf concerns).
#define ALIA_LAYOUT_PLACEMENT_FLAGS(X)                                        \
    /* alignment flags - Omitting alignment flags invokes "default" */        \
    /* alignment, which is FILL for containers and LEFT/TOP for leaves. */    \
    /* X alignment flags */                                                   \
    X(0b00000000000000111, X_ALIGNMENT_MASK)                                  \
    X(0b00000000000000001, CENTER_X)                                          \
    X(0b00000000000000010, ALIGN_LEFT)                                        \
    X(0b00000000000000011, ALIGN_RIGHT)                                       \
    X(0b00000000000000100, FILL_X)                                            \
    /* Y alignment flags */                                                   \
    X(0b00000000000111000, Y_ALIGNMENT_MASK)                                  \
    X(0b00000000000001000, CENTER_Y)                                          \
    X(0b00000000000010000, ALIGN_TOP)                                         \
    X(0b00000000000011000, ALIGN_BOTTOM)                                      \
    X(0b00000000000100000, FILL_Y)                                            \
    X(0b00000000000101000, BASELINE_Y)                                        \
    /* cross-axis alignment flags */                                          \
    X(0b00000000111000000, CROSS_ALIGNMENT_MASK)                              \
    X(0b00000000001000000, CENTER_CROSS)                                      \
    X(0b00000000010000000, ALIGN_START)                                       \
    X(0b00000000011000000, ALIGN_END)                                         \
    X(0b00000000100000000, FILL_CROSS)                                        \
    X(0b00000000101000000, BASELINE_CROSS)                                    \
    /* combined alignment flags */                                            \
    X(0b00000000001001001, CENTER)                                            \
    X(0b00000000100100100, FILL)                                              \
    /* default alignment block - These have the same meaning as the above, */ \
    /* but are provided by component implementations as defaults. During */   \
    /* resolution, these are substituted in for missing alignment flags. */   \
    /* X default alignment flags */                                           \
    X(0b000000111000000000, DEFAULT_X_ALIGNMENT_MASK)                         \
    X(0b000000001000000000, DEFAULT_CENTER_X)                                 \
    X(0b000000010000000000, DEFAULT_ALIGN_LEFT)                               \
    X(0b000000011000000000, DEFAULT_ALIGN_RIGHT)                              \
    X(0b000000100000000000, DEFAULT_FILL_X)                                   \
    /* Y default alignment flags */                                           \
    X(0b000111000000000000, DEFAULT_Y_ALIGNMENT_MASK)                         \
    X(0b000001000000000000, DEFAULT_CENTER_Y)                                 \
    X(0b000010000000000000, DEFAULT_ALIGN_TOP)                                \
    X(0b000011000000000000, DEFAULT_ALIGN_BOTTOM)                             \
    X(0b000100000000000000, DEFAULT_FILL_Y)                                   \
    X(0b000101000000000000, DEFAULT_BASELINE_Y)                               \
    /* cross-axis default alignment flags */                                  \
    X(0b111000000000000000, DEFAULT_CROSS_ALIGNMENT_MASK)                     \
    X(0b001000000000000000, DEFAULT_CENTER_CROSS)                             \
    X(0b010000000000000000, DEFAULT_ALIGN_START)                              \
    X(0b011000000000000000, DEFAULT_ALIGN_END)                                \
    X(0b100000000000000000, DEFAULT_FILL_CROSS)                               \
    X(0b101000000000000000, DEFAULT_BASELINE_CROSS)                           \
    /* combined default alignment flags */                                    \
    X(0b001001001000000000, DEFAULT_CENTER)                                   \
    X(0b100100100000000000, DEFAULT_FILL)                                     \
    /* Setting the GROW flag sets the node's growth factor to 1.0. */         \
    X(0b100000000000000000000000, GROW)                                       \
    /* FLUSH disables spacing around a node that has spacing by default. */   \
    /* SPACED enables spacing around a node that's flush by default. */       \
    X(0b11000000000000000000000000, SPACING_MASK)                             \
    X(0b01000000000000000000000000, FLUSH)                                    \
    X(0b10000000000000000000000000, SPACED)                                   \
    /* AXIS_RELATIVE_SIZE means content.size is (length, breadth) along */    \
    /* the parent's main/cross axes, not absolute (x, y). */                  \
    X(0b1000000000000000000000000000, AXIS_RELATIVE_SIZE)

// Container-policy flags - How a container arranges its children.
#define ALIA_LAYOUT_CONTAINER_FLAGS(X)                                        \
    /* justification flags - Apply to flow and block_flow line layouts. */    \
    X(0b111000000000000000000, JUSTIFY_MASK)                                  \
    X(0b000000000000000000000, JUSTIFY_START)                                 \
    X(0b001000000000000000000, JUSTIFY_END)                                   \
    X(0b010000000000000000000, JUSTIFY_CENTER)                                \
    X(0b011000000000000000000, JUSTIFY_SPACE_BETWEEN)                         \
    X(0b100000000000000000000, JUSTIFY_SPACE_AROUND)                          \
    X(0b101000000000000000000, JUSTIFY_SPACE_EVENLY)                          \
    /* baseline group alignment flags - When nodes use baseline alignment, */ \
    /* they are constrained relative to one another, but they are not */      \
    /* necessarily constrained within the larger container space. These */    \
    /* flags control how the group of nodes with baseline alignment are */    \
    /* positioned when there is extra vertical space. */                      \
    X(0b11000000000000000000000, BASELINE_GROUP_ALIGNMENT_MASK)               \
    X(0b01000000000000000000000, BASELINE_GROUP_ALIGN_TOP)                    \
    X(0b10000000000000000000000, BASELINE_GROUP_ALIGN_BOTTOM)                 \
    X(0b11000000000000000000000, BASELINE_GROUP_ALIGN_CENTER)

#define ALIA_LAYOUT_FLAGS(X)                                                  \
    ALIA_LAYOUT_PLACEMENT_FLAGS(X)                                            \
    ALIA_LAYOUT_CONTAINER_FLAGS(X)                                            \
    /* For nodes that wouldn't normally do so, the PROVIDE_BOX flag tells */  \
    /* the container to provide its box back to the component code for */     \
    /* consumption. */                                                        \
    X(0b100000000000000000000000000, PROVIDE_BOX)

enum
{
#define ALIA_DEFINE_LAYOUT_FLAG(value, name) ALIA_##name = value,
    ALIA_LAYOUT_FLAGS(ALIA_DEFINE_LAYOUT_FLAG)
#undef ALIA_DEFINE_LAYOUT_FLAG
};

// bit offsets for groupings of flags within the set
enum
{
    ALIA_X_ALIGNMENT_BIT_OFFSET = 0,
    ALIA_Y_ALIGNMENT_BIT_OFFSET = 3,
    ALIA_CROSS_ALIGNMENT_BIT_OFFSET = 6,
    ALIA_DEFAULT_ALIGNMENT_BIT_OFFSET = 9,
    ALIA_JUSTIFICATION_BIT_OFFSET = 18,
    ALIA_BASELINE_GROUP_ALIGNMENT_BIT_OFFSET = 21
};

// mask covering the caller alignment flags
#define ALIA_ALIGNMENT_MASK 0x1ffu
// mask covering the default alignment flags
#define ALIA_DEFAULT_ALIGNMENT_MASK                                           \
    (ALIA_ALIGNMENT_MASK << ALIA_DEFAULT_ALIGNMENT_BIT_OFFSET)

// Shift a caller-block alignment value into the default alignment block.
static inline alia_layout_flags_t
alia_as_default_alignment(alia_layout_flags_t alignment)
{
    return (alignment & ALIA_ALIGNMENT_MASK)
        << ALIA_DEFAULT_ALIGNMENT_BIT_OFFSET;
}

#endif
