#pragma once

#include <alia/abi/kernel/animation.h>

// TODO: Use a hand-rolled hash map instead of `std::unordered_map`.
#include <unordered_map>

namespace alia {

struct flare_group_animation_data
{
    // TODO: Support a variable flare capacity.
    static constexpr unsigned capacity = 7;
    unsigned flare_count = 0;
    alia_nanosecond_count flares[capacity];
};

using flare_map
    = std::unordered_map<alia_animation_id, flare_group_animation_data>;

struct transition_animation_data
{
    bool direction;
    alia_nanosecond_count transition_end;
};

using transition_animation_map
    = std::unordered_map<alia_animation_id, transition_animation_data>;

struct animation_system
{
    // active transitions
    transition_animation_map transitions;
    // active flare groups
    flare_map flares;
};

} // namespace alia
