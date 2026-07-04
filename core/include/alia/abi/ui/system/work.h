#ifndef ALIA_ABI_UI_SYSTEM_WORK_H
#define ALIA_ABI_UI_SYSTEM_WORK_H

#include <alia/abi/kernel/events.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// Typical host frame: enqueue input from the platform, then:
//   alia_ui_system_begin_update(ui);
//   while (alia_ui_work_step(ui) != ALIA_UI_WORK_STEP_IDLE) { }
//   alia_ui_system_end_update(ui);
// Or call alia_ui_system_update(ui) for the same behavior in one call.

void
alia_ui_system_poll_clock(alia_ui_system* ui);

void
alia_ui_system_begin_update(alia_ui_system* ui);

typedef enum alia_ui_work_step_kind
{
    ALIA_UI_WORK_STEP_IDLE = 0,
    ALIA_UI_WORK_STEP_INPUT,
    ALIA_UI_WORK_STEP_TIMER,
} alia_ui_work_step_kind;

alia_ui_work_step_kind
alia_ui_work_step(alia_ui_system* ui);

void
alia_ui_system_end_update(alia_ui_system* ui);

// Does the UI need to issue a frame immediately? This is true if there is any
// pending input, an event timer is due to be processed, the UI has been
// marked as dirty, or any animations are active.
bool
alia_ui_needs_tick(alia_ui_system* ui);

// Next absolute steady-clock time (nanoseconds) the UI suggests waking, or
// false if no timer is scheduled (host may still wait on external input).
// When the event queue is non-empty, returns the current tick_count (wake
// now). Animation-driven wake is not included yet.
bool
alia_ui_next_wake_ns(alia_ui_system* ui, alia_nanosecond_count* out_wake_ns);

void
alia_ui_enqueue_event(alia_ui_system* ui, alia_event const* event);

void
alia_ui_mark_dirty(alia_ui_system* ui);

typedef enum alia_ui_refresh_hook_policy
{
    ALIA_UI_REFRESH_NEVER = 0,
    ALIA_UI_REFRESH_IF_DIRTY = 1,
    ALIA_UI_REFRESH_ALWAYS = 2,
} alia_ui_refresh_hook_policy;

typedef struct alia_ui_refresh_policy
{
    alia_ui_refresh_hook_policy before_input;
    alia_ui_refresh_hook_policy before_draw;
} alia_ui_refresh_policy;

void
alia_ui_set_refresh_policy(
    alia_ui_system* ui, alia_ui_refresh_policy const* policy);

typedef enum alia_ui_work_kind
{
    ALIA_UI_WORK_LAYOUT_RESOLVE,
    ALIA_UI_WORK_EVENT_DISPATCH,
} alia_ui_work_kind;

typedef struct alia_ui_work_item
{
    alia_ui_work_kind kind;
    alia_event event; // for ALIA_UI_WORK_EVENT_DISPATCH
} alia_ui_work_item;

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_SYSTEM_WORK_H */
