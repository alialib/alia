#pragma once

#include <cmath>

#include <alia/geometry.h>

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

} // namespace alia
