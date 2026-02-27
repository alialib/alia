#include <alia/kernel/animation.h>
#include <alia/system/object.hpp>

namespace alia {

inline void
pop_flare(flare_group_animation_data& group)
{
    ALIA_ASSERT(group.flare_count > 0);
    --group.flare_count;
    for (unsigned i = 0; i != group.flare_count; ++i)
        group.flares[i] = group.flares[i + 1];
}

inline void
push_flare(flare_group_animation_data& group, alia_nanosecond_count end_time)
{
    if (group.flare_count == flare_group_animation_data::capacity)
        pop_flare(group);
    group.flares[group.flare_count] = end_time;
    ++group.flare_count;
}

} // namespace alia

ALIA_EXTERN_C_BEGIN

void
alia_fire_flare(
    alia_context* ctx, alia_bitref bit, alia_nanosecond_count duration)
{
    push_flare(
        ctx->system->animation.flares[alia_make_animation_id(bit)],
        alia_animation_tick_count(ctx) + duration);
    alia_bitref_set(bit);
}

unsigned
alia_process_flares(
    alia_context* ctx, alia_bitref bit, alia_nanosecond_count* tick_counts)
{
    if (!alia_bitref_is_set(bit))
        return 0;

    auto& group = ctx->system->animation.flares[alia_make_animation_id(bit)];
    unsigned flare_index = 0;
    while (flare_index < group.flare_count)
    {
        alia_nanosecond_count ticks_left
            = alia_animation_ticks_left(ctx, group.flares[flare_index]);
        if (ticks_left > 0)
        {
            // The flare has time left, so process it and move on to the
            // next one.
            tick_counts[flare_index] = ticks_left;
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
        ctx->system->animation.flares.erase(alia_make_animation_id(bit));
        alia_bitref_clear(bit);
    }
    return flare_index;
}

ALIA_EXTERN_C_END
