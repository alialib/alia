#ifndef ALIA_ABI_UI_LAYOUT_FLAGS_H
#define ALIA_ABI_UI_LAYOUT_FLAGS_H

#include <alia/abi/prelude.h>

typedef uint32_t alia_layout_flags_t;

// bit offsets for groupings of flags within the set
alia_layout_flags_t const ALIA_X_ALIGNMENT_BIT_OFFSET = 0;
alia_layout_flags_t const ALIA_Y_ALIGNMENT_BIT_OFFSET = 3;
alia_layout_flags_t const ALIA_CROSS_ALIGNMENT_BIT_OFFSET = 6;
alia_layout_flags_t const ALIA_JUSTIFICATION_BIT_OFFSET = 9;
alia_layout_flags_t const ALIA_BASELINE_GROUP_ALIGNMENT_BIT_OFFSET = 12;

#define ALIA_LAYOUT_FLAGS(X)                                                  \
    /* alignment flags - Omitting alignment flags invokes "default" */        \
    /* alignment, which is FILL for containers and LEFT/TOP for leaves. */    \
    /* X alignment flags */                                                   \
    X(0b0000000000000111, X_ALIGNMENT_MASK)                                   \
    X(0b0000000000000001, CENTER_X)                                           \
    X(0b0000000000000010, ALIGN_LEFT)                                         \
    X(0b0000000000000011, ALIGN_RIGHT)                                        \
    X(0b0000000000000100, FILL_X)                                             \
    /* Y alignment flags */                                                   \
    X(0b0000000000111000, Y_ALIGNMENT_MASK)                                   \
    X(0b0000000000001000, CENTER_Y)                                           \
    X(0b0000000000010000, ALIGN_TOP)                                          \
    X(0b0000000000011000, ALIGN_BOTTOM)                                       \
    X(0b0000000000100000, FILL_Y)                                             \
    X(0b0000000000101000, BASELINE_Y)                                         \
    /* cross-axis alignment flags - It is the app's responsibility to */      \
    /* ensure that the cross-axis alignment doesn't contradict the X/Y */     \
    /* alignment flags. */                                                    \
    X(0b0000000111000000, CROSS_ALIGNMENT_MASK)                               \
    X(0b0000000001000000, CENTER_CROSS)                                       \
    X(0b0000000010000000, ALIGN_START)                                        \
    X(0b0000000011000000, ALIGN_END)                                          \
    X(0b0000000100000000, FILL_CROSS)                                         \
    X(0b0000000101000000, BASELINE_CROSS)                                     \
    /* combined alignment flags */                                            \
    X(0b0000000001001001, CENTER)                                             \
    X(0b0000000100100100, FILL)                                               \
    /* justification flags - Apply to flow and block_flow line layouts. */    \
    X(0b0000111000000000, JUSTIFY_MASK)                                       \
    X(0b0000000000000000, JUSTIFY_START)                                      \
    X(0b0000001000000000, JUSTIFY_END)                                        \
    X(0b0000010000000000, JUSTIFY_CENTER)                                     \
    X(0b0000011000000000, JUSTIFY_SPACE_BETWEEN)                              \
    X(0b0000100000000000, JUSTIFY_SPACE_AROUND)                               \
    X(0b0000101000000000, JUSTIFY_SPACE_EVENLY)                               \
    /* baseline group alignment flags - When nodes use baseline alignment, */ \
    /* they are constrained relative to one another, but they are not */      \
    /* necessarily constrained within the larger container space. These */    \
    /* flags control how the group of nodes with baseline alignment are */    \
    /* positioned when there is extra vertical space. */                      \
    X(0b0011000000000000, BASELINE_GROUP_ALIGNMENT_MASK)                      \
    X(0b0001000000000000, BASELINE_GROUP_ALIGN_TOP)                           \
    X(0b0010000000000000, BASELINE_GROUP_ALIGN_BOTTOM)                        \
    X(0b0011000000000000, BASELINE_GROUP_ALIGN_CENTER)                        \
    /* Setting the GROW flag sets the node's growth factor to 1.0. */         \
    X(0b0100000000000000, GROW)                                               \
    /* For leaf nodes, the FLUSH flag disables spacing around the node. */    \
    X(0b1000000000000000, FLUSH)

enum
{
#define ALIA_DEFINE_LAYOUT_FLAG(value, name) ALIA_##name = value,
    ALIA_LAYOUT_FLAGS(ALIA_DEFINE_LAYOUT_FLAG)
#undef ALIA_DEFINE_LAYOUT_FLAG
};

#endif
