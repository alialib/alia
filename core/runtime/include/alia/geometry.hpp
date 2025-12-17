#pragma once

#include <cmath>

#include <alia/geometry.h>

#include <alia/base.hpp>

namespace alia {

using vec2 = alia_vec2;

inline vec2
operator+(vec2 a, vec2 b)
{
    return vec2{a.x + b.x, a.y + b.y};
}

inline vec2&
operator+=(vec2& a, vec2 b)
{
    a.x += b.x;
    a.y += b.y;
    return a;
}

inline vec2
operator-(vec2 a, vec2 b)
{
    return vec2{a.x - b.x, a.y - b.y};
}

inline vec2&
operator-=(vec2& a, vec2 b)
{
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

inline vec2
operator*(vec2 v, float s)
{
    return vec2{v.x * s, v.y * s};
}

inline vec2&
operator*=(vec2& v, float s)
{
    v.x *= s;
    v.y *= s;
    return v;
}

inline vec2
operator/(vec2 v, float s)
{
    return vec2{v.x / s, v.y / s};
}

inline vec2&
operator/=(vec2& v, float s)
{
    v.x /= s;
    v.y /= s;
    return v;
}

inline bool
operator==(vec2 a, vec2 b)
{
    return a.x == b.x && a.y == b.y;
}

inline bool
operator!=(vec2 a, vec2 b)
{
    return !(a == b);
}

using box = alia_box;
using rect = alia_rect;

inline box
apply_margin(box box, vec2 margin)
{
    return {.pos = box.pos + margin, .size = box.size - margin * 2};
}

struct insets
{
    float left;
    float right;
    float top;
    float bottom;
};

struct affine2
{
    float a, b, c, d, tx, ty;
};

inline affine2
identity_matrix()
{
    return {1, 0, 0, 1, 0, 0};
}

inline affine2
translation_matrix(float x, float y)
{
    return {1, 0, 0, 1, x, y};
}

inline affine2
scaling_matrix(float sx, float sy)
{
    return {sx, 0, 0, sy, 0, 0};
}

inline affine2
rotation_matrix(float r)
{
    float s = std::sin(r), c = std::cos(r);
    return {c, s, -s, c, 0, 0};
}

inline affine2
compose(affine2 p, affine2 c)
{
    return {
        .a = p.a * c.a + p.c * c.b,
        .b = p.b * c.a + p.d * c.b,
        .c = p.a * c.c + p.c * c.d,
        .d = p.b * c.c + p.d * c.d,
        .tx = p.a * c.tx + p.c * c.ty + p.tx,
        .ty = p.b * c.tx + p.d * c.ty + p.ty,
    };
}

static inline affine2
invert_affine2(affine2 m)
{
    float det = m.a * m.d - m.b * m.c;

    ALIA_ASSERT(det != 0.0f);

    float inv_det = 1.0f / det;

    affine2 r;
    r.a = m.d * inv_det;
    r.b = -m.b * inv_det;
    r.c = -m.c * inv_det;
    r.d = m.a * inv_det;
    r.tx = -(r.a * m.tx + r.c * m.ty);
    r.ty = -(r.b * m.tx + r.d * m.ty);

    return r;
}

static inline vec2
transform_point(affine2 m, vec2 v)
{
    return {m.a * v.x + m.c * v.y + m.tx, m.b * v.x + m.d * v.y + m.ty};
}

static inline vec2
transform_vector(affine2 m, vec2 v)
{
    return {m.a * v.x + m.c * v.y, m.b * v.x + m.d * v.y};
}

} // namespace alia
