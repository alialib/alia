#include <alia/abi/kernel/timing.h>

#include <alia/abi/kernel/events.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/system/work.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/flow/traversal.h>
#include <alia/ui/system/object.h>

#include <cstring>

namespace {

uint64_t
get_current_timer_cycle(alia_ui_system* sys)
{
    // Component code uses this as a "do not dispatch in the same cycle"
    // token.
    return sys ? sys->timer_event_cycle : 0u;
}

void
enqueue_timer(
    alia_ui_system* sys,
    alia_element_id target,
    alia_nanosecond_count fire_time)
{
    ALIA_ASSERT(sys);

    sys->timer_requests.push(
        alia_ui_timer_request{
            .target = target,
            .fire_time = fire_time,
            .queued_in_cycle = get_current_timer_cycle(sys)});
}

} // namespace

extern "C" {

void
alia_timing_request_animation_refresh(alia_context* ctx)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->system);

    alia_ui_mark_dirty(ctx->system);
    if (ctx->events)
        alia::mark_animating_component(*ctx);
}

alia_timer_state*
alia_timer_use(alia_context* ctx)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->substrate);

    alia_substrate_usage_result result = alia_substrate_use_memory(
        ctx, sizeof(alia_timer_state), alignof(alia_timer_state));
    ALIA_ASSERT(result.ptr);

    auto* state = static_cast<alia_timer_state*>(result.ptr);

    // Initialize on first use (INIT / DISCOVERY). The timer state is POD, so
    // simple assignment is sufficient.
    if (result.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT
        || result.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY)
    {
        state->active = false;
        state->expected_fire_time = 0;
    }

    // A stable target identifier for matching the timer event.
    state->target = static_cast<alia_element_id>(state);

    return state;
}

void
alia_timer_start(
    alia_context* ctx, alia_timer_state* state, alia_nanosecond_count duration)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(state);
    ALIA_ASSERT(ctx->system);

    // TODO: If duration wraps, we still want wrap-safe comparisons in the
    // system dispatcher.
    state->expected_fire_time = ctx->tick_count + duration;
    state->active = true;
    ALIA_ASSERT(alia_element_id_is_valid(state->target));

    enqueue_timer(ctx->system, state->target, state->expected_fire_time);
}

void
alia_timer_stop(alia_context* ctx, alia_timer_state* state)
{
    (void) ctx;
    ALIA_ASSERT(state);

    state->active = false;
}

bool
alia_timer_handle_event(alia_context* ctx, alia_timer_state* state)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(state);
    ALIA_ASSERT(ctx->events);
    ALIA_ASSERT(ctx->events->event);

    auto* ev = ctx->events->event;
    if (ev->type != ALIA_EVENT_TIMER)
        return false;

    if (!state->active)
        return false;

    alia_timer payload;
    std::memcpy(&payload, ev->payload, sizeof(payload));

    if (!alia_element_id_equal(payload.target, state->target))
        return false;
    if (payload.fire_time != state->expected_fire_time)
        return false;

    // Timer fired; disarm it. Component code can re-arm from inside the
    // handler if desired.
    state->active = false;
    return true;
}

} // extern "C"
