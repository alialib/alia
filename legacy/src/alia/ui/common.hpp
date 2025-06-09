#ifndef ALIA_UI_COMMON_HPP
#define ALIA_UI_COMMON_HPP

#include <alia/core/common.hpp>

namespace alia {

// Combine two hash values.
size_t inline combine_hashes(size_t a, size_t b)
{
    return a ^ (0x9e37'79b9 + (a << 6) + (a >> 2) + b);
}

#endif
