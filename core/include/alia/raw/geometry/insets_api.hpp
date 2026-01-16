#pragma once

#include <alia/abi/geometry/insets_api.h>

#include <alia/raw/geometry/types.hpp>

namespace alia { namespace raw {

static inline insets
insets_make_trbl(float top, float right, float bottom, float left)
{
    return alia_insets_make_trbl(top, right, bottom, left);
}

static inline insets
alia_insets_make_xy(float x, float y)
{
    return alia_insets_make_xy(x, y);
}

static inline alia_insets
insets_make_uniform(float value)
{
    return alia_insets_make_uniform(value);
}

static inline bool
insets_equal(insets a, insets b)
{
    return alia_insets_equal(a, b);
}

static inline bool
operator==(insets a, insets b)
{
    return alia_insets_equal(a, b);
}

static inline bool
operator!=(insets a, insets b)
{
    return !(a == b);
}

static inline insets
insets_add(insets a, insets b)
{
    return alia_insets_add(a, b);
}

static inline insets
operator+(insets a, insets b)
{
    return insets_add(a, b);
}

static inline void
insets_add_inplace(insets* a, insets b)
{
    alia_insets_add_inplace(a, b);
}

static inline insets&
operator+=(insets& a, insets b)
{
    insets_add_inplace(&a, b);
    return a;
}

static inline insets
insets_sub(insets a, insets b)
{
    return alia_insets_sub(a, b);
}

static inline insets
operator-(insets a, insets b)
{
    return insets_sub(a, b);
}

static inline void
insets_sub_inplace(insets* a, insets b)
{
    alia_insets_sub_inplace(a, b);
}

static inline insets&
operator-=(insets& a, insets b)
{
    insets_sub_inplace(&a, b);
    return a;
}

static inline insets
insets_scale(insets a, float s)
{
    return alia_insets_scale(a, s);
}

static inline insets
operator*(insets a, float s)
{
    return insets_scale(a, s);
}

static inline insets
operator*(float s, insets a)
{
    return insets_scale(a, s);
}

static inline void
insets_scale_inplace(insets* a, float s)
{
    alia_insets_scale_inplace(a, s);
}

static inline insets&
operator*=(insets& a, float s)
{
    insets_scale_inplace(&a, s);
    return a;
}

}} // namespace alia::raw
