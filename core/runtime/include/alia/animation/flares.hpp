#pragma once

#include <cstdint>
#include <map>

#include <alia/animation/context.hpp>
#include <alia/animation/ids.hpp>
#include <alia/base.hpp>
#include <alia/bit_packing.hpp>

namespace alia {

struct flare_group_animation_data
{
    static constexpr unsigned capacity = 7;
    unsigned flare_count = 0;
    nanosecond_count flares[capacity];
};

using flare_map = std::map<uintptr_t, flare_group_animation_data>;

inline void
pop_flare(flare_group_animation_data& group)
{
    ALIA_ASSERT(group.flare_count > 0);
    --group.flare_count;
    for (unsigned i = 0; i != group.flare_count; ++i)
        group.flares[i] = group.flares[i + 1];
}

inline void
push_flare(flare_group_animation_data& group, nanosecond_count end_time)
{
    if (group.flare_count == flare_group_animation_data::capacity)
        pop_flare(group);
    group.flares[group.flare_count] = end_time;
    ++group.flare_count;
}

struct flare_bit_field : bit_field<1>
{
};

template<class Context>
concept flare_animation_context
    = animation_context<Context> && requires(Context& ctx) {
          { get_animation_system(ctx).flares } -> std::same_as<flare_map&>;
      };

template<flare_animation_context Context>
void
fire_flare(
    Context& ctx, bitref<flare_bit_field> bit, nanosecond_count duration)
{
    push_flare(
        get_animation_system(ctx).flares[make_animation_id(bit)],
        get_animation_tick_count(ctx) + duration);
    set_bit(bit);
}

template<flare_animation_context Context, class Processor>
void
process_flares(
    Context& ctx, bitref<flare_bit_field> bit, Processor&& processor)
{
    if (is_set(bit))
    {
        auto& group = get_animation_system(ctx).flares[make_animation_id(bit)];
        unsigned flare_index = 0;
        while (flare_index < group.flare_count)
        {
            nanosecond_count ticks_left
                = get_animation_ticks_left(ctx, group.flares[flare_index]);
            if (ticks_left > 0)
            {
                // The flare has time left, so process it and move on to the
                // next one.
                std::forward<Processor>(processor)(ticks_left);
                ++flare_index;
            }
            else
            {
                // The flare is done, so remove it.
                // All flares in a group should have the same duration and
                // should be sorted from oldest to youngest, so we should
                // always be removing them from the front.
                ALIA_ASSERT(flare_index == 0);
                pop_flare(group);
            }
        }
        if (flare_index == 0)
        {
            // All flares were popped, so remove this group.
            get_animation_system(ctx).flares.erase(make_animation_id(bit));
            clear_bit(bit);
        }
    }
}

} // namespace alia
