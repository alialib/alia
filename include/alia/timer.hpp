#ifndef ALIA_TIMER_HPP
#define ALIA_TIMER_HPP

#include <alia/forward.hpp>

namespace alia {

// This implements a one-shot timer that can be used to schedule time-dependent
// UI events.
//
// Any UI element that includes time-dependent behavior should use the
// functions provided here.  To schedule some timed behavior, call
// start_timer() and then on each pass through the control function, poll
// is_timer_done().  Once it returns true, the given time has elapsed.
// Note that is_timer_done() can only return true on a TIMER_EVENT (which is
// in the INPUT_CATEGORY).
//
void start_timer(context& ctx, region_id id, unsigned duration);
bool is_timer_done(context& ctx, region_id id);

// restart_timer() is similar to start_timer(), but it can only be invoked
// when handling a previous event (i.e, when is_timer_done() returns true).
// It adjusts the duration so that it's relative to when the event SHOULD HAVE
// occurred, rather than when it actually occurred.  This is useful for
// scheduling repeated events on a fixed frequency without drifting.
//
void restart_timer(context& ctx, region_id id, unsigned duration);

}

#endif
