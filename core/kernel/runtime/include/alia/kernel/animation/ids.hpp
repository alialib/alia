#pragma once

#include <bit>
#include <climits>
#include <cstdint>

#include <alia/kernel/bit_packing.hpp>

namespace alia {

using animation_id = uintptr_t;

template<class Tag>
inline animation_id
make_animation_id(bitref<Tag>& ref)
{
    return std::bit_cast<animation_id>(&ref.storage)
         | (animation_id(ref.index)
            << (sizeof(animation_id) * CHAR_BIT - CHAR_BIT));
}

} // namespace alia
