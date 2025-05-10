#pragma once

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

} // namespace alia
