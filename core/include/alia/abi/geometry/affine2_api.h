#ifndef ALIA_GEOMETRY_AFFINE2_API_H
#define ALIA_GEOMETRY_AFFINE2_API_H

#include <alia/abi/base/config.h>
#include <alia/abi/geometry/box_api.h>
#include <alia/abi/geometry/types.h>
#include <alia/abi/geometry/vec2_api.h>

#include <math.h>

ALIA_EXTERN_C_BEGIN

static inline alia_affine2
alia_affine2_make(float a, float b, float c, float d, float tx, float ty)
{
    return ALIA_BRACED_INIT(alia_affine2, a, b, c, d, tx, ty);
}

static inline alia_affine2
alia_affine2_identity()
{
    return alia_affine2_make(1, 0, 0, 1, 0, 0);
}

static inline alia_affine2
alia_affine2_translation(float x, float y)
{
    return alia_affine2_make(1, 0, 0, 1, x, y);
}

static inline alia_affine2
alia_affine2_scaling(float sx, float sy)
{
    return alia_affine2_make(sx, 0, 0, sy, 0, 0);
}

static inline alia_affine2
alia_affine2_rotation(float r)
{
    float s = sinf(r), c = cosf(r);
    return alia_affine2_make(c, s, -s, c, 0, 0);
}

static inline alia_affine2
alia_affine2_compose(alia_affine2 p, alia_affine2 c)
{
    return ALIA_BRACED_INIT(
        alia_affine2,
        .a = p.a * c.a + p.c * c.b,
        .b = p.b * c.a + p.d * c.b,
        .c = p.a * c.c + p.c * c.d,
        .d = p.b * c.c + p.d * c.d,
        .tx = p.a * c.tx + p.c * c.ty + p.tx,
        .ty = p.b * c.tx + p.d * c.ty + p.ty, );
}

static inline alia_affine2
alia_affine2_invert(alia_affine2 m)
{
    float det = m.a * m.d - m.b * m.c;
    ALIA_ASSERT(det != 0.0f);
    float inv_det = 1.0f / det;

    alia_affine2 r;
    r.a = m.d * inv_det;
    r.b = -m.b * inv_det;
    r.c = -m.c * inv_det;
    r.d = m.a * inv_det;
    r.tx = -(r.a * m.tx + r.c * m.ty);
    r.ty = -(r.b * m.tx + r.d * m.ty);
    return r;
}

static inline alia_vec2f
alia_affine2_transform_point(alia_affine2 m, alia_vec2f v)
{
    return ALIA_BRACED_INIT(
        alia_vec2f,
        m.a * v.x + m.c * v.y + m.tx,
        m.b * v.x + m.d * v.y + m.ty);
}

static inline alia_vec2f
alia_affine2_transform_vector(alia_affine2 m, alia_vec2f v)
{
    return ALIA_BRACED_INIT(
        alia_vec2f, m.a * v.x + m.c * v.y, m.b * v.x + m.d * v.y);
}

static inline alia_box
alia_affine2_transform_aabb(alia_affine2 m, alia_box b)
{
    alia_vec2f c
        = alia_vec2f_add(b.min, alia_vec2f_scale(b.size, 0.5f)); // center
    alia_vec2f e = alia_vec2f_scale(b.size, 0.5f); // extents

    alia_vec2f c2 = alia_affine2_transform_point(m, c);

    // e' = |M| * e
    float aa = fabsf(m.a), cc = fabsf(m.c);
    float bb = fabsf(m.b), dd = fabsf(m.d);

    alia_vec2f e2 = alia_vec2f_make(aa * e.x + cc * e.y, bb * e.x + dd * e.y);

    alia_vec2f min2 = alia_vec2f_sub(c2, e2);
    alia_vec2f max2 = alia_vec2f_add(c2, e2);

    return alia_box_make(min2, alia_vec2f_sub(max2, min2));
}

ALIA_EXTERN_C_END

#endif /* ALIA_GEOMETRY_AFFINE2_API_H */
