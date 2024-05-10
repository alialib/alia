#ifndef ALIA_UI_UTILITIES_WIDGETS_HPP
#define ALIA_UI_UTILITIES_WIDGETS_HPP

#include <alia/ui/common.hpp>
#include <alia/ui/ids.hpp>

namespace alia {

// widget state flags
struct widget_state_flag_tag
{
};
typedef flag_set<widget_state_flag_tag> widget_state;
// primary state
ALIA_DEFINE_FLAG(widget_state, 0x01, WIDGET_NORMAL)
ALIA_DEFINE_FLAG(widget_state, 0x02, WIDGET_DISABLED)
ALIA_DEFINE_FLAG(widget_state, 0x03, WIDGET_HOT)
ALIA_DEFINE_FLAG(widget_state, 0x04, WIDGET_DEPRESSED)
ALIA_DEFINE_FLAG(widget_state, 0x05, WIDGET_SELECTED)
ALIA_DEFINE_FLAG(widget_state, 0x0f, WIDGET_PRIMARY_STATE_MASK)
// additional (independent) states
ALIA_DEFINE_FLAG(widget_state, 0x10, WIDGET_FOCUSED)

// Get the state of a widget by detecting if it has the focus or is being
// interacted with via the mouse.
//
// 'overrides' allows you to include special states that the function wouldn't
// otherwise be aware of. It can include any of the following.
//  * WIDGET_SELECTED
//  * WIDGET_DISABLED
//  * WIDGET_DEPRESSED (e.g., if the widget was pressed using the keyboard)
//
widget_state
get_widget_state(
    dataless_ui_context ctx, widget_id id, widget_state overrides = NO_FLAGS);

// Widgets are identified by pointers.
// Sometimes its useful to request some dummy data just to get a unique
// pointer to use as a widget ID. When doing so, using this type can make
// things clearer.
typedef char widget_identity;

widget_id
get_widget_id(ui_context& ctx);

} // namespace alia

#endif
