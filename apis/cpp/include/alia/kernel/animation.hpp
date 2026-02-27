#pragma once

#include <alia/impl/base/bit_packing.hpp>
#include <alia/kernel/animation.h>

namespace alia {

struct flare_bitfield : bitfield<1>
{
};

struct smoothing_bitfield : bitfield<2>
{
};

} // namespace alia
