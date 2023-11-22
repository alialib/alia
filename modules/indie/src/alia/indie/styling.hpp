#ifndef ALIA_INDIE_STYLING_HPP
#define ALIA_INDIE_STYLING_HPP

#include <alia/indie/common.hpp>

namespace alia { namespace indie {

// widget state flags
struct element_state_flag_tag
{
};
typedef flag_set<element_state_flag_tag> element_state;
// primary state
ALIA_DEFINE_FLAG(element_state, 0x01, ELEMENT_NORMAL)
ALIA_DEFINE_FLAG(element_state, 0x02, ELEMENT_DISABLED)
ALIA_DEFINE_FLAG(element_state, 0x03, ELEMENT_HOT)
ALIA_DEFINE_FLAG(element_state, 0x04, ELEMENT_DEPRESSED)
ALIA_DEFINE_FLAG(element_state, 0x05, ELEMENT_SELECTED)
ALIA_DEFINE_FLAG(element_state, 0x0f, ELEMENT_PRIMARY_STATE_MASK)
// additional (independent) states
ALIA_DEFINE_FLAG(element_state, 0x10, ELEMENT_FOCUSED)

}} // namespace alia::indie

#endif
