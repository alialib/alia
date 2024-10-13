#ifndef ALIA_UI_UTILITIES_ANIMATION_HPP
#define ALIA_UI_UTILITIES_ANIMATION_HPP

// TODO: Integrate some/all of this into core and refactor wrt timing module.

#include <bit>

#include <alia/core/bit_packing.hpp>
#include <alia/core/timing/smoothing.hpp>
#include <alia/core/timing/ticks.hpp>

namespace alia {

template<class Tag>
inline uintptr_t
make_animation_id(bitref<Tag>& ref)
{
    return std::bit_cast<uintptr_t>(&ref.storage)
           | (uintptr_t(ref.index) << (sizeof(uintptr_t) * 8 - 8));
}

struct transition_animation_data
{
    bool direction;
    millisecond_count transition_end;
};

struct flare_group_animation_data
{
    static constexpr unsigned capacity = 7;
    unsigned flare_count = 0;
    millisecond_count flares[capacity];
};

inline void
pop_flare(flare_group_animation_data& group)
{
    assert(group.flare_count > 0);
    --group.flare_count;
    for (unsigned i = 0; i != group.flare_count; ++i)
        group.flares[i] = group.flares[i + 1];
}

inline void
push_flare(flare_group_animation_data& group, millisecond_count end_time)
{
    if (group.flare_count == flare_group_animation_data::capacity)
        pop_flare(group);
    group.flares[group.flare_count] = end_time;
    ++group.flare_count;
}

struct animation_tracking_system
{
    // active transitions
    std::map<uintptr_t, transition_animation_data> transitions;
    // active flare groups
    std::map<uintptr_t, flare_group_animation_data> flares;
};

static animation_tracking_system the_animation_system;

struct flare_bit_field : bit_field<1>
{
};

inline void
fire_flare(
    dataless_core_context ctx,
    bitref<flare_bit_field> bit,
    millisecond_count duration)
{
    push_flare(
        the_animation_system.flares[make_animation_id(bit)],
        get_raw_animation_tick_count(ctx) + duration);
    set_bit(bit);
}

template<class Processor>
void
process_flares(
    dataless_core_context ctx,
    bitref<flare_bit_field> bit,
    Processor&& processor)
{
    if (is_set(bit))
    {
        auto& group = the_animation_system.flares[make_animation_id(bit)];
        unsigned flare_index = 0;
        while (flare_index != group.flare_count)
        {
            millisecond_count ticks_left
                = get_raw_animation_ticks_left(ctx, group.flares[flare_index]);
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
                assert(flare_index == 0);
                pop_flare(group);
            }
        }
        if (flare_index == 0)
        {
            // All flares were popped, so remove this group.
            the_animation_system.flares.erase(make_animation_id(bit));
            clear_bit(bit);
        }
    }
}

struct smoothing_bit_field : bit_field<2>
{
};

inline void
init_animation(
    dataless_core_context ctx,
    bitref<smoothing_bit_field> bits,
    bool current_state,
    animated_transition const& transition)
{
    auto& animation
        = the_animation_system.transitions[make_animation_id(bits)];
    animation.direction = current_state;
    animation.transition_end
        = get_raw_animation_tick_count(ctx) + transition.duration;
    write_bitref(bits, 0b01);
}

template<class Value>
Value
smooth_between_values(
    dataless_core_context ctx,
    bitref<smoothing_bit_field> bits,
    bool current_state,
    Value true_value,
    Value false_value,
    animated_transition const& transition = default_transition)
{
    auto internal_state = read_bitref(bits);
    switch (internal_state)
    {
        default:
        case 0b00:
            write_bitref(bits, current_state ? 0b11 : 0b10);
            return current_state ? true_value : false_value;
        case 0b01: {
            auto& animation
                = the_animation_system.transitions[make_animation_id(bits)];
            millisecond_count ticks_left
                = get_raw_animation_ticks_left(ctx, animation.transition_end);
            if (ticks_left > 0)
            {
                double fraction = eval_curve_at_x(
                    transition.curve,
                    1. - double(ticks_left) / transition.duration,
                    1. / transition.duration);
                Value current_value = interpolate(
                    animation.direction ? false_value : true_value,
                    animation.direction ? true_value : false_value,
                    fraction);
                if (current_state != animation.direction)
                {
                    // Go back in the same amount of time it took to get here.
                    // In order to do this, we have to solve for time it will
                    // take to get back here.
                    animation.transition_end
                        = get_raw_animation_tick_count(ctx)
                          + millisecond_count(
                              transition.duration
                              * (1
                                 - eval_curve_at_x(
                                     unit_cubic_bezier{
                                         1 - transition.curve.p1x,
                                         1 - transition.curve.p1y,
                                         1 - transition.curve.p2x,
                                         1 - transition.curve.p2y},
                                     1 - fraction,
                                     1. / transition.duration)));
                    animation.direction = current_state;
                }
                return current_value;
            }
            else
            {
                auto end_state = animation.direction;
                the_animation_system.transitions.erase(
                    make_animation_id(bits));
                write_bitref(bits, end_state ? 0b11 : 0b10);
                return end_state ? true_value : false_value;
            }
        }
        case 0b10:
            if (current_state)
                init_animation(ctx, bits, true, transition);
            return false_value;
        case 0b11:
            if (!current_state)
                init_animation(ctx, bits, false, transition);
            return true_value;
    }
}

} // namespace alia

#endif
