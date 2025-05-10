#pragma once

#include <alia/ui/geometry.hpp>

#include <cstddef>

namespace alia {

using LayoutIndex = int;

struct LayoutSpec
{
    Vec2 size;
    Vec2 margin;
    // LayoutIndex first_child;
    // LayoutIndex next_sibling;
};

struct LayoutPlacement
{
    Vec2 position;
    Vec2 size;
};

void
layout(
    LayoutSpec const* specs,
    LayoutPlacement* placements,
    std::size_t count,
    Vec2 available_space);

} // namespace alia
