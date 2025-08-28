#pragma once

namespace alia {

struct color
{
    float r, g, b, a;
};

constexpr color RED = {1.f, 0.f, 0.f, 1.f};
constexpr color BLUE = {0.f, 0.f, 1.f, 1.f};
constexpr color GRAY = {0.5f, 0.5f, 0.5f, 1.f};

} // namespace alia
