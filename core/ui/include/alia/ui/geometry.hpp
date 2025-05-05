#pragma once

namespace alia {

struct Vec2
{
    float x, y;
};

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
