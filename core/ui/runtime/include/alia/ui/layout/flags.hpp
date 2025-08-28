#pragma once

#include <alia/kernel/flags.hpp>

namespace alia {

// The following flags are used to specify various aspects of a node's layout,
// including how it is placed within the space allocated to it (its alignment).
// Omitting these flags invokes the default behavior for the node, which
// depends on the type of node.

ALIA_DEFINE_FLAG_TYPE(std::uint16_t, layout)

// X alignment flags
unsigned const X_ALIGNMENT_BIT_OFFSET = 0;
ALIA_DEFINE_FLAG(layout, 0b00'000'000'000'111, X_ALIGNMENT_MASK)
ALIA_DEFINE_FLAG(layout, 0b00'000'000'000'001, CENTER_X)
ALIA_DEFINE_FLAG(layout, 0b00'000'000'000'010, ALIGN_LEFT)
ALIA_DEFINE_FLAG(layout, 0b00'000'000'000'011, ALIGN_RIGHT)
ALIA_DEFINE_FLAG(layout, 0b00'000'000'000'100, FILL_X)

// Y alignment flags
unsigned const Y_ALIGNMENT_BIT_OFFSET = 3;
ALIA_DEFINE_FLAG(layout, 0b00'000'000'111'000, Y_ALIGNMENT_MASK)
ALIA_DEFINE_FLAG(layout, 0b00'000'000'001'000, CENTER_Y)
ALIA_DEFINE_FLAG(layout, 0b00'000'000'010'000, ALIGN_TOP)
ALIA_DEFINE_FLAG(layout, 0b00'000'000'011'000, ALIGN_BOTTOM)
ALIA_DEFINE_FLAG(layout, 0b00'000'000'100'000, FILL_Y)
ALIA_DEFINE_FLAG(layout, 0b00'000'000'101'000, BASELINE_Y)

// cross-axis alignment flags
unsigned const CROSS_ALIGNMENT_BIT_OFFSET = 6;
ALIA_DEFINE_FLAG(layout, 0b00'000'111'000'000, CROSS_ALIGNMENT_MASK)
ALIA_DEFINE_FLAG(layout, 0b00'000'001'000'000, CENTER_CROSS)
ALIA_DEFINE_FLAG(layout, 0b00'000'010'000'000, ALIGN_START)
ALIA_DEFINE_FLAG(layout, 0b00'000'011'000'000, ALIGN_END)
ALIA_DEFINE_FLAG(layout, 0b00'000'100'000'000, FILL_CROSS)
ALIA_DEFINE_FLAG(layout, 0b00'000'101'000'000, BASELINE_CROSS)

// justification flags
ALIA_DEFINE_FLAG(layout, 0b00'111'000'000'000, JUSTIFY_MASK)
ALIA_DEFINE_FLAG(layout, 0b00'000'000'000'000, JUSTIFY_START)
ALIA_DEFINE_FLAG(layout, 0b00'001'000'000'000, JUSTIFY_END)
ALIA_DEFINE_FLAG(layout, 0b00'010'000'000'000, JUSTIFY_CENTER)
ALIA_DEFINE_FLAG(layout, 0b00'011'000'000'000, JUSTIFY_SPACE_BETWEEN)
ALIA_DEFINE_FLAG(layout, 0b00'100'000'000'000, JUSTIFY_SPACE_AROUND)
ALIA_DEFINE_FLAG(layout, 0b00'101'000'000'000, JUSTIFY_SPACE_EVENLY)

// combined alignment flags
layout_flag_set const CENTER = CENTER_X | CENTER_Y | CENTER_CROSS;
layout_flag_set const FILL = FILL_X | FILL_Y | FILL_CROSS;

// Setting the GROW flag sets the node's growth factor to 1.0.
ALIA_DEFINE_FLAG(layout, 0b01'000'000'000'000, GROW)

// For nodes that support padding, the UNPADDED flag disables padding.
ALIA_DEFINE_FLAG(layout, 0b10'000'000'000'000, UNPADDED)

} // namespace alia
