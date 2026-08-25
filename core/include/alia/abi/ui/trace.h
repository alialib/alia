#ifndef ALIA_ABI_UI_TRACE_H
#define ALIA_ABI_UI_TRACE_H

#include <alia/abi/kernel/events.h>
#include <alia/abi/kernel/routing.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// Monotonic trace clock tick. Independent of `ui->tick_count`. Convert
// deltas with `alia_trace_ticks_to_ns` for display.
typedef int64_t alia_trace_tick;

alia_trace_tick
alia_trace_now(void);

// Convert a tick delta to nanoseconds.
int64_t
alia_trace_ticks_to_ns(alia_trace_tick ticks);

typedef enum alia_trace_pass_kind
{
    ALIA_TRACE_PASS_REFRESH = 0,
    ALIA_TRACE_PASS_EVENT,
    ALIA_TRACE_PASS_LAYOUT,
    ALIA_TRACE_PASS_DRAW_RECORD,
    ALIA_TRACE_PASS_DRAW_EXECUTE,
} alia_trace_pass_kind;

// One timed unit of work within a presented frame.
typedef struct alia_trace_pass
{
    alia_trace_pass_kind kind;
    alia_trace_tick start;
    alia_trace_tick end;
    union
    {
        struct
        {
            // 0-based index among REFRESH passes of the enclosing refresh
            uint8_t index;
            bool incomplete;
        } refresh;
        struct
        {
            alia_event_type type;
            alia_element_id target;
        } event;
        struct
        {
            uint16_t target_count;
            uint16_t bucket_count;
        } draw;
    };
} alia_trace_pass;

// optional tracing sink installed on an `alia_ui_system` - All callbacks may
// be NULL. When the sink pointer on the system is NULL, tracing is off and
// the core skips timestamp reads.
typedef struct alia_ui_trace_sink
{
    void* user;
    void (*frame_begin)(void* user);
    void (*frame_end)(void* user);
    void (*pass)(void* user, alia_trace_pass const* pass);
} alia_ui_trace_sink;

// Install or clear the trace sink. `sink` may be NULL. The pointed-to sink
// must remain valid as long as it's installed.
void
alia_ui_set_trace_sink(alia_ui_system* ui, alia_ui_trace_sink const* sink);

alia_ui_trace_sink const*
alia_ui_get_trace_sink(alia_ui_system const* ui);

// Invocation helpers for the trace sink callbacks...

void
alia_ui_trace_frame_begin(alia_ui_system* ui);

void
alia_ui_trace_frame_end(alia_ui_system* ui);

void
alia_ui_trace_emit_pass(alia_ui_system* ui, alia_trace_pass const* pass);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_TRACE_H */
