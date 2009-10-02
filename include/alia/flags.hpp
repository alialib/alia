#ifndef ALIA_FLAGS_HPP
#define ALIA_FLAGS_HPP

namespace alia {

// TODO: Fix these.
static unsigned const X_AXIS =              0x00000000;
static unsigned const Y_AXIS =              0x00000001;
static unsigned const NO_AXIS =             0x00000003;
static unsigned const AXIS_MASK =           0x00000003;

static unsigned const MANUAL_DELETE =       0x00000004;

// widgets that accept potentially large values
static unsigned const STATIC =              0x00000008;

// controls, buttons
static unsigned const DISABLED =            0x00000010;

// text controls
static unsigned const PASSWORD =            0x00000040;
static unsigned const SINGLE_LINE =         0x00000080;
static unsigned const MULTILINE =           0x00000100;
static unsigned const NO_PANEL =            0x00000200;
static unsigned const ALWAYS_EDITING =      0x00000400;

// expandables
static unsigned const INITIALLY_EXPANDED =  0x00000800;
static unsigned const INITIALLY_COLLAPSED = 0x00001000;

// panel
static unsigned const ROW_LAYOUT =          0x00002000;
// scrollable panel
static unsigned const GREEDY =              0x00004000;

// peripheral widgets
static unsigned const LEFT_SIDE =           0x00010000;
static unsigned const RIGHT_SIDE =          0x00020000;
static unsigned const TOP_SIDE =            0x00040000;
static unsigned const BOTTOM_SIDE =         0x00080000;
static unsigned const SIDE_MASK =           0x000f0000;

static unsigned const CUSTOM_FLAG_0 =       0x80000000;
static unsigned const CUSTOM_FLAG_1 =       0x40000000;
static unsigned const CUSTOM_FLAG_2 =       0x20000000;
static unsigned const CUSTOM_FLAG_3 =       0x10000000;

}

#endif
