#pragma once

#include <alia/prelude.hpp>

namespace alia {

// An animation_context provides access to animation-related functionality.
template<class Context>
concept animation_context = requires(Context& ctx) {
    // Get the current tick count associated with the context.
    { get_tick_count(ctx) } -> std::same_as<nanosecond_count>;

    // Mark that an animation is in progress.
    { mark_animation_in_progress(ctx) } -> std::same_as<void>;
};

template<animation_context Context>
nanosecond_count
get_animation_tick_count(Context& ctx)
{
    mark_animation_in_progress(ctx);
    return get_tick_count(ctx);
}

template<animation_context Context>
nanosecond_count
get_animation_ticks_left(Context& ctx, nanosecond_count end_time)
{
    nanosecond_count ticks_remaining = end_time - get_tick_count(ctx);
    if (ticks_remaining > 0)
    {
        mark_animation_in_progress(ctx);
        return ticks_remaining;
    }
    return 0;
}

} // namespace alia
