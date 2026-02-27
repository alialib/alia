#pragma once

#include <alia/abi/kernel/animation.h>

namespace alia { namespace impl {

void
start_transition(
    alia_context* ctx,
    alia_bitref bits,
    bool current_state,
    alia_animated_transition const& transition);

float
update_transition(
    alia_context* ctx,
    alia_bitref bits,
    bool current_state,
    alia_animated_transition const& transition);

template<class Value, class Lerp>
Value
smooth(
    alia_context* ctx,
    alia_animated_transition const& transition,
    Lerp&& lerp,
    alia_bitref bits,
    bool current_state,
    Value true_value,
    Value false_value)
{
    switch (alia_bitref_read_pair(bits))
    {
        default:
        case 0b00:
            alia_bitref_write_pair(bits, current_state ? 0b11 : 0b10);
            return current_state ? true_value : false_value;
        case 0b01: {
            float fraction
                = update_transition(ctx, bits, current_state, transition);
            return std::forward<Lerp>(lerp)(false_value, true_value, fraction);
        }
        // steady state OFF
        case 0b10:
            if (current_state)
                start_transition(ctx, bits, true, transition);
            return false_value;
        case 0b11:
            if (!current_state)
                start_transition(ctx, bits, false, transition);
            return true_value;
    }
}

}} // namespace alia::impl
