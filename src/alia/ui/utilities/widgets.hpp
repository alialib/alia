#ifndef ALIA_UI_UTILITIES_WIDGETS_HPP
#define ALIA_UI_UTILITIES_WIDGETS_HPP

#include <alia/ui/common.hpp>
#include <alia/ui/ids.hpp>

namespace alia {

// element interaction status flags
struct interaction_status_flag_tag
{
};
typedef flag_set<interaction_status_flag_tag> interaction_status;
// DISABLED - element is disabled by app logic and can't be
// interacted with - Note that when this is set, get_interaction_status() will
// ensure that the other flags aren't set.
ALIA_DEFINE_FLAG(interaction_status, 0x01, WIDGET_DISABLED)
// HOVERED
ALIA_DEFINE_FLAG(interaction_status, 0x02, WIDGET_HOVERED)
// ACTIVE
ALIA_DEFINE_FLAG(interaction_status, 0x04, WIDGET_ACTIVE)
// FOCUSED
ALIA_DEFINE_FLAG(interaction_status, 0x08, WIDGET_FOCUSED)

inline bool
is_disabled(interaction_status state)
{
    return (state & WIDGET_DISABLED) ? true : false;
}

inline bool
is_hovered(interaction_status state)
{
    return (state & WIDGET_HOVERED) ? true : false;
}

inline bool
is_active(interaction_status state)
{
    return (state & WIDGET_ACTIVE) ? true : false;
}

inline bool
is_focused(interaction_status state)
{
    return (state & WIDGET_FOCUSED) ? true : false;
}

// Get the state of a widget by detecting if it has the focus or is being
// interacted with via the mouse.
//
// 'overrides' allows you to include special states that the function wouldn't
// otherwise be aware of. It can include any of the following.
//  * INTERACTION_DISABLED
//  * INTERACTION_DEPRESSED (e.g., if the widget was pressed using the
//    keyboard)
//
interaction_status
get_interaction_status(
    dataless_ui_context ctx,
    widget_id id,
    interaction_status overrides = NO_FLAGS);

// Widgets are identified by pointers.
// Sometimes its useful to request some dummy data just to get a unique
// pointer to use as a widget ID. When doing so, using this type can make
// things clearer.
typedef char widget_identity;

widget_id
get_widget_id(ui_context& ctx);

} // namespace alia

#endif
