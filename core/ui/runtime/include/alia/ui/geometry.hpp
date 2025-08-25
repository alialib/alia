#pragma once

#include <cmath>

namespace alia {

struct Vec2
{
    float x, y;
};

inline Vec2
operator+(Vec2 a, Vec2 b)
{
    return Vec2{a.x + b.x, a.y + b.y};
}

inline Vec2&
operator+=(Vec2& a, Vec2 b)
{
    a.x += b.x;
    a.y += b.y;
    return a;
}

inline Vec2
operator-(Vec2 a, Vec2 b)
{
    return Vec2{a.x - b.x, a.y - b.y};
}

inline Vec2&
operator-=(Vec2& a, Vec2 b)
{
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

inline Vec2
operator*(Vec2 v, float s)
{
    return Vec2{v.x * s, v.y * s};
}

inline Vec2&
operator*=(Vec2& v, float s)
{
    v.x *= s;
    v.y *= s;
    return v;
}

inline Vec2
operator/(Vec2 v, float s)
{
    return Vec2{v.x / s, v.y / s};
}

inline Vec2&
operator/=(Vec2& v, float s)
{
    v.x /= s;
    v.y /= s;
    return v;
}

struct Box
{
    Vec2 pos;
    Vec2 size;
};

struct Rect
{
    Vec2 min;
    Vec2 max;
};

inline Box
apply_margin(Box box, Vec2 margin)
{
    return {.pos = box.pos + margin, .size = box.size - margin * 2};
}

struct Insets
{
    float left;
    float right;
    float top;
    float bottom;
};

struct Affine2
{
    float a, b, c, d, tx, ty;
};

inline Affine2
identity_matrix()
{
    return {1, 0, 0, 1, 0, 0};
}

inline Affine2
translation_matrix(float x, float y)
{
    return {1, 0, 0, 1, x, y};
}

inline Affine2
scaling_matrix(float sx, float sy)
{
    return {sx, 0, 0, sy, 0, 0};
}

inline Affine2
rotation_matrix(float r)
{
    float s = std::sin(r), c = std::cos(r);
    return {c, s, -s, c, 0, 0};
}

inline Affine2
compose(Affine2 p, Affine2 c)
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
