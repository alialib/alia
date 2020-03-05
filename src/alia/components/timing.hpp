#ifndef ALIA_COMPONENTS_TIMING_HPP
#define ALIA_COMPONENTS_TIMING_HPP

#include <alia/components/context.hpp>

namespace alia {

// Currently, alia's only sense of time is that of a monotonically increasing
// millisecond counter. It's understood to have an arbitrary start point and is
// allowed to wrap around, so 'unsigned' is considered sufficient.
typedef unsigned millisecond_count;

struct timing_component
{
    millisecond_count tick_counter = 0;
};

// Request that the UI context refresh again quickly enough for smooth
// animation.
void
request_animation_refresh(dataless_context ctx);

// Get the value of the millisecond tick counter associated with the given
// UI context. This counter is updated every refresh pass, so it's consistent
// within a single frame.
// When this is called, it's assumed that something is currently animating, so
// it also requests a refresh.
millisecond_count
get_raw_animation_tick_count(dataless_context ctx);

// Same as above, but returns a signal rather than a raw integer.
value_signal<millisecond_count>
get_animation_tick_count(dataless_context ctx);

// Get the number of ticks remaining until the given end time.
// If the time has passed, this returns 0.
// This ensures that the UI context refreshes until the end time is reached.
millisecond_count
get_raw_animation_ticks_left(dataless_context ctx, millisecond_count end_tick);

} // namespace alia

#endif