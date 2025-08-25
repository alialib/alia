#pragma once

namespace alia {

struct Color
{
    float r, g, b, a;
};

constexpr Color RED = {1.f, 0.f, 0.f, 1.f};
constexpr Color BLUE = {0.f, 0.f, 1.f, 1.f};
constexpr Color GRAY = {0.5f, 0.5f, 0.5f, 1.f};

} // namespace alia
