#ifndef ALIA_FLAGS_HPP
#define ALIA_FLAGS_HPP

namespace alia {

struct flag_set
{
    unsigned code;
    flag_set() {}
    explicit flag_set(unsigned code) : code(code) {}
    // allows use within if statements without other unintended conversions
    typedef unsigned flag_set::* unspecified_bool_type;
    operator unspecified_bool_type() const
    { return code != 0 ? &flag_set::code : 0; }
};

static inline flag_set operator|(flag_set a, flag_set b)
{ return flag_set(a.code | b.code); }
static inline flag_set& operator|=(flag_set& a, flag_set b)
{ a.code |= b.code; return a; }
static inline flag_set operator&(flag_set a, flag_set b)
{ return flag_set(a.code & b.code); }
static inline flag_set& operator&=(flag_set& a, flag_set b)
{ a.code &= b.code; return a; }
static inline bool operator==(flag_set a, flag_set b)
{ return a.code == b.code; }

static flag_set const NO_FLAGS(0);

static unsigned const HORIZONTAL_CODE =                             0x00000001;
static flag_set const HORIZONTAL(HORIZONTAL_CODE);
static unsigned const VERTICAL_CODE =                               0x00000002;
static flag_set const VERTICAL(VERTICAL_CODE);
static unsigned const AXIS_MASK_CODE =                              0x00000003;
static flag_set const AXIS_MASK(AXIS_MASK_CODE);

static unsigned const MANUAL_DELETE_CODE =                          0x00000004;
static flag_set const MANUAL_DELETE(MANUAL_DELETE_CODE);

// widgets that accept potentially large values
static unsigned const STATIC_CODE =                                 0x00000008;
static flag_set const STATIC(STATIC_CODE);

// controls, buttons
static unsigned const DISABLED_CODE =                               0x00000010;
static flag_set const DISABLED(DISABLED_CODE);

// text controls
static unsigned const PASSWORD_CODE =                               0x00000040;
static flag_set const PASSWORD(PASSWORD_CODE);
static unsigned const SINGLE_LINE_CODE =                            0x00000080;
static flag_set const SINGLE_LINE(SINGLE_LINE_CODE);
static unsigned const MULTILINE_CODE =                              0x00000100;
static flag_set const MULTILINE(MULTILINE_CODE);
static unsigned const NO_PANEL_CODE =                               0x00000200;
static flag_set const NO_PANEL(NO_PANEL_CODE);
static unsigned const ALWAYS_EDITING_CODE =                         0x00000400;
static flag_set const ALWAYS_EDITING(ALWAYS_EDITING_CODE);

// expandables
static unsigned const INITIALLY_EXPANDED_CODE =                     0x00000800;
static flag_set const INITIALLY_EXPANDED(INITIALLY_EXPANDED_CODE);
static unsigned const INITIALLY_COLLAPSED_CODE =                    0x00001000;
static flag_set const INITIALLY_COLLAPSED(INITIALLY_COLLAPSED_CODE);

// panel
static unsigned const INTERACTIVE_CODE =                            0x00002000;
static flag_set const INTERACTIVE(INTERACTIVE_CODE);

// scrollable panel
static unsigned const GREEDY_CODE =                                 0x00002000;
static flag_set const GREEDY(GREEDY_CODE);

// peripheral widgets
static unsigned const LEFT_SIDE_CODE =                              0x00010000;
static flag_set const LEFT_SIDE(LEFT_SIDE_CODE);
static unsigned const RIGHT_SIDE_CODE =                             0x00020000;
static flag_set const RIGHT_SIDE(RIGHT_SIDE_CODE);
static unsigned const TOP_SIDE_CODE =                               0x00040000;
static flag_set const TOP_SIDE(TOP_SIDE_CODE);
static unsigned const BOTTOM_SIDE_CODE =                            0x00080000;
static flag_set const BOTTOM_SIDE(BOTTOM_SIDE_CODE);
static unsigned const SIDE_MASK_CODE =                              0x000f0000;
static flag_set const SIDE_MASK(SIDE_MASK_CODE);

static unsigned const NO_FOCUS_BORDER_CODE =                        0x00100000;
static flag_set const NO_FOCUS_BORDER(NO_FOCUS_BORDER_CODE);

static unsigned const FLIPPED_CODE =                                0x00200000;
static flag_set const FLIPPED(FLIPPED_CODE);

static unsigned const CUSTOM_FLAG_0_CODE =                          0x10000000;
static unsigned const CUSTOM_FLAG_1_CODE =                          0x20000000;
static unsigned const CUSTOM_FLAG_2_CODE =                          0x40000000;
static unsigned const CUSTOM_FLAG_3_CODE =                          0x80000000;

}

#endif
