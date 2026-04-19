#ifndef ALIA_ABI_KERNEL_TIMING_H
#define ALIA_ABI_KERNEL_TIMING_H

#include <alia/abi/kernel/routing.h>
#include <alia/abi/prelude.h>
#include <alia/context.h>

ALIA_EXTERN_C_BEGIN

static inline alia_nanosecond_count
alia_timing_tick_count(alia_context* ctx)
{
    return ctx->tick_count;
}

static inline void
alia_timing_request_animation_refresh(alia_context* ctx)
{
    // TODO: Implement this.
}

typedef struct alia_timer_state
{
    bool active;
    alia_nanosecond_count expected_fire_time;
    // A stable identifier for the component instance that owns this timer
    // state (typically derived from the state storage address).
    alia_element_id target;
} alia_timer_state;

// Allocate (and initialize on first use) timer state associated with the
// current substrate block evaluation.
alia_timer_state*
alia_timer_use(alia_context* ctx);

// Arm a timer for `duration` after the current tick count.
void
alia_timer_start(
    alia_context* ctx,
    alia_timer_state* state,
    alia_nanosecond_count duration);

// Disarm a timer (in-flight scheduled timer events may still be dispatched
// later, but handlers will ignore them because `active == false`).
void
alia_timer_stop(alia_context* ctx, alia_timer_state* state);

// Call from within component event handling to process `ALIA_EVENT_TIMER`.
// Returns true when this timer fired for the current event.
bool
alia_timer_handle_event(alia_context* ctx, alia_timer_state* state);

ALIA_EXTERN_C_END

#endif // ALIA_ABI_KERNEL_TIMING_H
