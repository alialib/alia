#ifndef ALIA_WIDGET_STATE_HPP
#define ALIA_WIDGET_STATE_HPP

#include <alia/forward.hpp>

namespace alia {

typedef unsigned widget_state;

// Note that not all states are necessarily applicable to all widgets.

namespace widget_states
{
    static widget_state const PRIMARY_STATE_MASK   = 0x0f;

    static widget_state const NORMAL               = 0x00;
    // the mouse is over the widget
    static widget_state const HOT                  = 0x01;
    // the mouse is clicked down on the widget
    static widget_state const DEPRESSED            = 0x02;

    // the widget has the keyboard focus
    static widget_state const FOCUSED              = 0x10;

    // Some widgets (e.g, text controls) can be activated or deactivated, so
    // just knowing whether or not they have the focus is insufficient.
    static widget_state const ACTIVATED            = 0x20;

    static widget_state const DISABLED             = 0x40;

    // the user has entered invalid data into the widget
    static widget_state const INVALID              = 0x80;
}

widget_state get_widget_state(context& ctx, region_id id, bool enabled = true,
    bool pressed = false);

}

#endif
