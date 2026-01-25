#ifndef ALIA_ABI_BASE_GEOMETRY_INSETS_H
#define ALIA_ABI_BASE_GEOMETRY_INSETS_H

#include <alia/abi/base/geometry/types.h>
#include <alia/abi/base/geometry/vec2.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

static inline alia_insets
alia_insets_make_trbl(float top, float right, float bottom, float left)
{
    return ALIA_BRACED_INIT(alia_insets, left, right, top, bottom);
}

static inline alia_insets
alia_insets_make_xy(float x, float y)
{
    return alia_insets_make_trbl(y, x, y, x);
}

static inline alia_insets
alia_insets_make_uniform(float value)
{
    return alia_insets_make_trbl(value, value, value, value);
}

static inline bool
alia_insets_equal(alia_insets a, alia_insets b)
{
    return a.left == b.left && a.right == b.right && a.top == b.top
        && a.bottom == b.bottom;
}

ALIA_DEFINE_EQUALITY_OPERATOR(alia_insets)

static inline alia_insets
alia_insets_add(alia_insets a, alia_insets b)
{
    return ALIA_BRACED_INIT(
        alia_insets,
        a.left + b.left,
        a.right + b.right,
        a.top + b.top,
        a.bottom + b.bottom);
}

static inline void
alia_insets_add_inplace(alia_insets* a, alia_insets b)
{
    a->left += b.left;
    a->right += b.right;
    a->top += b.top;
    a->bottom += b.bottom;
}

ALIA_DEFINE_PLUS_OPERATOR(alia_insets)

static inline alia_insets
alia_insets_sub(alia_insets a, alia_insets b)
{
    return ALIA_BRACED_INIT(
        alia_insets,
        a.left - b.left,
        a.right - b.right,
        a.top - b.top,
        a.bottom - b.bottom);
}

static inline void
alia_insets_sub_inplace(alia_insets* a, alia_insets b)
{
    a->left -= b.left;
    a->right -= b.right;
    a->top -= b.top;
    a->bottom -= b.bottom;
}

ALIA_DEFINE_MINUS_OPERATOR(alia_insets)

static inline alia_insets
alia_insets_scale(alia_insets a, float s)
{
    return ALIA_BRACED_INIT(
        alia_insets, a.left * s, a.right * s, a.top * s, a.bottom * s);
}

static inline void
alia_insets_scale_inplace(alia_insets* a, float s)
{
    a->left *= s;
    a->right *= s;
    a->top *= s;
    a->bottom *= s;
}

ALIA_DEFINE_SCALE_OPERATOR(alia_insets, float)

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_BASE_GEOMETRY_INSETS_H */
