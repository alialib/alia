#ifndef ALIA_UI_UTILITIES_WIDGETS_HPP
#define ALIA_UI_UTILITIES_WIDGETS_HPP

#include <alia/abi/ids.h>
#include <alia/abi/ui/elements.h>
#include <alia/context.hpp>
#include <alia/flags.hpp>

namespace alia {

// element interaction status flags
struct interaction_status_flag_tag
{
    using storage_type = alia_interaction_status_t;
};
typedef flag_set<interaction_status_flag_tag, alia_interaction_status_t>
    interaction_status;

#define X(code, NAME)                                                         \
    ALIA_DEFINE_FLAG(interaction_status, code, ELEMENT_##NAME);
ALIA_INTERACTION_STATUS_LIST(X)
#undef X

inline bool
is_disabled(interaction_status state)
{
    return (state & ELEMENT_DISABLED) ? true : false;
}

inline bool
is_hovered(interaction_status state)
{
    return (state & ELEMENT_HOVERED) ? true : false;
}

inline bool
is_active(interaction_status state)
{
    return (state & ELEMENT_ACTIVE) ? true : false;
}

inline bool
is_focused(interaction_status state)
{
    return (state & ELEMENT_FOCUSED) ? true : false;
}

// Get the state of an element by detecting if it has the focus or is being
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
    ephemeral_context& ctx,
    alia_element_id id,
    interaction_status overrides = NO_FLAGS);

// Elements are identified by pointers.
// Sometimes it's useful to request some dummy data just to get a unique
// pointer to use as an element ID. When doing so, using this type can make
// things clearer.
typedef char element_identity;

element_identity
get_element_identity(ephemeral_context& ctx);

} // namespace alia

#endif
