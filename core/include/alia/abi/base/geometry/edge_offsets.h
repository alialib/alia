#ifndef ALIA_ABI_BASE_GEOMETRY_EDGE_OFFSETS_H
#define ALIA_ABI_BASE_GEOMETRY_EDGE_OFFSETS_H

#include <alia/abi/base/geometry/types.h>
#include <alia/abi/base/geometry/vec2.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

static inline alia_edge_offsets
alia_edge_offsets_make_trbl(float top, float right, float bottom, float left)
{
    return ALIA_BRACED_INIT(alia_edge_offsets, left, right, top, bottom);
}

static inline alia_edge_offsets
alia_edge_offsets_make_xy(float x, float y)
{
    return alia_edge_offsets_make_trbl(y, x, y, x);
}

static inline alia_edge_offsets
alia_edge_offsets_make_uniform(float value)
{
    return alia_edge_offsets_make_trbl(value, value, value, value);
}

static inline bool
alia_edge_offsets_equal(alia_edge_offsets a, alia_edge_offsets b)
{
    return a.left == b.left && a.right == b.right && a.top == b.top
        && a.bottom == b.bottom;
}

ALIA_DEFINE_EQUALITY_OPERATOR(alia_edge_offsets)

static inline alia_edge_offsets
alia_edge_offsets_add(alia_edge_offsets a, alia_edge_offsets b)
{
    return ALIA_BRACED_INIT(
        alia_edge_offsets,
        a.left + b.left,
        a.right + b.right,
        a.top + b.top,
        a.bottom + b.bottom);
}

static inline void
alia_edge_offsets_add_inplace(alia_edge_offsets* a, alia_edge_offsets b)
{
    a->left += b.left;
    a->right += b.right;
    a->top += b.top;
    a->bottom += b.bottom;
}

ALIA_DEFINE_PLUS_OPERATOR(alia_edge_offsets)

static inline alia_edge_offsets
alia_edge_offsets_sub(alia_edge_offsets a, alia_edge_offsets b)
{
    return ALIA_BRACED_INIT(
        alia_edge_offsets,
        a.left - b.left,
        a.right - b.right,
        a.top - b.top,
        a.bottom - b.bottom);
}

static inline void
alia_edge_offsets_sub_inplace(alia_edge_offsets* a, alia_edge_offsets b)
{
    a->left -= b.left;
    a->right -= b.right;
    a->top -= b.top;
    a->bottom -= b.bottom;
}

ALIA_DEFINE_MINUS_OPERATOR(alia_edge_offsets)

static inline alia_edge_offsets
alia_edge_offsets_scale(alia_edge_offsets a, float s)
{
    return ALIA_BRACED_INIT(
        alia_edge_offsets, a.left * s, a.right * s, a.top * s, a.bottom * s);
}

static inline void
alia_edge_offsets_scale_inplace(alia_edge_offsets* a, float s)
{
    a->left *= s;
    a->right *= s;
    a->top *= s;
    a->bottom *= s;
}

ALIA_DEFINE_SCALE_OPERATOR(alia_edge_offsets, float)

static inline alia_edge_offsets
alia_edge_offsets_invert(alia_edge_offsets a)
{
    return ALIA_BRACED_INIT(
        alia_edge_offsets, -a.left, -a.right, -a.top, -a.bottom);
}

static inline void
alia_edge_offsets_invert_inplace(alia_edge_offsets* a)
{
    a->left = -a->left;
    a->right = -a->right;
    a->top = -a->top;
    a->bottom = -a->bottom;
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_BASE_GEOMETRY_EDGE_OFFSETS_H */
