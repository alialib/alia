#ifndef ALIA_GEOMETRY_VEC2_API_H
#define ALIA_GEOMETRY_VEC2_API_H

#include <alia/abi/base/config.h>
#include <alia/abi/geometry/types.h>

#include <math.h>

ALIA_EXTERN_C_BEGIN

/* CONSTRUCTORS */

static inline alia_vec2i
alia_vec2i_make(int32_t x, int32_t y)
{
    return ALIA_BRACED_INIT(alia_vec2i, x, y);
}

static inline alia_vec2f
alia_vec2f_make(float x, float y)
{
    return ALIA_BRACED_INIT(alia_vec2f, x, y);
}

/* COMPARISON */

static inline bool
alia_vec2i_equal(alia_vec2i a, alia_vec2i b)
{
    return a.x == b.x && a.y == b.y;
}

static inline bool
alia_vec2f_equal(alia_vec2f a, alia_vec2f b)
{
    return a.x == b.x && a.y == b.y;
}

static inline bool
alia_vec2f_near(alia_vec2f a, alia_vec2f b, float epsilon)
{
    return fabsf(a.x - b.x) <= epsilon && fabsf(a.y - b.y) <= epsilon;
}

/* EXPLICIT CONVERSIONS */

static inline alia_vec2f
alia_vec2i_to_vec2f(alia_vec2i v)
{
    return alia_vec2f_make((float) v.x, (float) v.y);
}

static inline alia_vec2i
alia_vec2f_to_vec2i_trunc(alia_vec2f v)
{
    return alia_vec2i_make((int32_t) v.x, (int32_t) v.y);
}

static inline alia_vec2i
alia_vec2f_floor(alia_vec2f v)
{
    return alia_vec2i_make((int32_t) floorf(v.x), (int32_t) floorf(v.y));
}

static inline alia_vec2i
alia_vec2f_ceil(alia_vec2f v)
{
    return alia_vec2i_make((int32_t) ceilf(v.x), (int32_t) ceilf(v.y));
}

static inline alia_vec2i
alia_vec2f_round(alia_vec2f v)
{
    return alia_vec2i_make((int32_t) roundf(v.x), (int32_t) roundf(v.y));
}

/* BASIC ARITHMETIC - FLOAT */

static inline alia_vec2f
alia_vec2f_add(alia_vec2f a, alia_vec2f b)
{
    return alia_vec2f_make(a.x + b.x, a.y + b.y);
}

static inline void
alia_vec2f_add_inplace(alia_vec2f* a, alia_vec2f b)
{
    a->x += b.x;
    a->y += b.y;
}

static inline alia_vec2f
alia_vec2f_sub(alia_vec2f a, alia_vec2f b)
{
    return alia_vec2f_make(a.x - b.x, a.y - b.y);
}

static inline void
alia_vec2f_sub_inplace(alia_vec2f* a, alia_vec2f b)
{
    a->x -= b.x;
    a->y -= b.y;
}

static inline alia_vec2f
alia_vec2f_scale(alia_vec2f v, float s)
{
    return alia_vec2f_make(v.x * s, v.y * s);
}

static inline void
alia_vec2f_scale_inplace(alia_vec2f* v, float s)
{
    v->x *= s;
    v->y *= s;
}

static inline float
alia_vec2f_dot(alia_vec2f a, alia_vec2f b)
{
    return a.x * b.x + a.y * b.y;
}

static inline float
alia_vec2f_length_sq(alia_vec2f v)
{
    return alia_vec2f_dot(v, v);
}

static inline float
alia_vec2f_length(alia_vec2f v)
{
    return sqrtf(alia_vec2f_length_sq(v));
}

/* BASIC ARITHMETIC - INTEGER */

static inline alia_vec2i
alia_vec2i_add(alia_vec2i a, alia_vec2i b)
{
    return alia_vec2i_make(a.x + b.x, a.y + b.y);
}

static inline void
alia_vec2i_add_inplace(alia_vec2i* a, alia_vec2i b)
{
    a->x += b.x;
    a->y += b.y;
}

static inline alia_vec2i
alia_vec2i_sub(alia_vec2i a, alia_vec2i b)
{
    return alia_vec2i_make(a.x - b.x, a.y - b.y);
}

static inline void
alia_vec2i_sub_inplace(alia_vec2i* a, alia_vec2i b)
{
    a->x -= b.x;
    a->y -= b.y;
}

static inline alia_vec2i
alia_vec2i_min(alia_vec2i a, alia_vec2i b)
{
    return alia_vec2i_make(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y);
}

static inline alia_vec2i
alia_vec2i_max(alia_vec2i a, alia_vec2i b)
{
    return alia_vec2i_make(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y);
}

ALIA_EXTERN_C_END

#endif /* ALIA_GEOMETRY_VEC2_API_H */
