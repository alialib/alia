#pragma once

#include <alia/abi/geometry/vec2_api.h>

#include <alia/raw/geometry/types.hpp>

namespace alia { namespace raw {

inline vec2f
operator+(vec2f a, vec2f b)
{
    return alia_vec2f_add(a, b);
}

inline vec2f&
operator+=(vec2f& a, vec2f b)
{
    alia_vec2f_add_inplace(&a, b);
    return a;
}

inline vec2f
operator-(vec2f a, vec2f b)
{
    return alia_vec2f_sub(a, b);
}

inline vec2f&
operator-=(vec2f& a, vec2f b)
{
    alia_vec2f_sub_inplace(&a, b);
    return a;
}

inline vec2f
operator*(vec2f v, float s)
{
    return alia_vec2f_scale(v, s);
}

inline vec2f&
operator*=(vec2f& v, float s)
{
    alia_vec2f_scale_inplace(&v, s);
    return v;
}

inline vec2f
operator/(vec2f v, float s)
{
    return alia_vec2f_scale(v, 1.0f / s);
}

inline vec2f&
operator/=(vec2f& v, float s)
{
    v = alia_vec2f_scale(v, 1.0f / s);
    return v;
}

inline bool
operator==(vec2f a, vec2f b)
{
    return alia_vec2f_equal(a, b);
}

inline bool
operator!=(vec2f a, vec2f b)
{
    return !(a == b);
}

}} // namespace alia::raw
