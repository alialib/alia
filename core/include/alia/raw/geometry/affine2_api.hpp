#pragma once

#include <alia/abi/geometry/affine2_api.h>
#include <alia/raw/geometry/types.hpp>

#include <math.h>

namespace alia { namespace raw {

static inline affine2
affine2_make(float a, float b, float c, float d, float tx, float ty)
{
    return alia_affine2_make(a, b, c, d, tx, ty);
}

static inline affine2
affine2_identity()
{
    return alia_affine2_identity();
}

static inline affine2
affine2_translation(float x, float y)
{
    return alia_affine2_translation(x, y);
}

static inline affine2
affine2_scaling(float sx, float sy)
{
    return alia_affine2_scaling(sx, sy);
}

static inline affine2
affine2_rotation(float r)
{
    return alia_affine2_rotation(r);
}

static inline affine2
affine2_compose(affine2 p, affine2 c)
{
    return alia_affine2_compose(p, c);
}

static inline affine2
affine2_invert(affine2 m)
{
    return alia_affine2_invert(m);
}

static inline vec2f
affine2_transform_point(affine2 m, vec2f v)
{
    return alia_affine2_transform_point(m, v);
}

static inline vec2f
affine2_transform_vector(affine2 m, vec2f v)
{
    return alia_affine2_transform_vector(m, v);
}

static inline box
affine2_transform_aabb(affine2 m, box b)
{
    return alia_affine2_transform_aabb(m, b);
}

}} // namespace alia::raw
