#include <alia/ui/layout.hpp>

#include <algorithm>

namespace alia {

void
layout(
    LayoutSpec const* specs,
    LayoutPlacement* placements,
    std::size_t count,
    Vec2 available_space)
{
    Vec2 current_position = {0, 0};
    float current_height = 0;
    for (std::size_t i = 0; i < count; ++i)
    {
        auto const& spec = specs[i];
        auto& placement = placements[i];
        placement.position = current_position + spec.margin;
        placement.size = spec.size;
        current_position.x += spec.size.x + spec.margin.x * 2;
        current_height
            = std::max(current_height, spec.size.y + spec.margin.y * 2);
        if (current_position.x + spec.size.x > available_space.x)
        {
            current_position.x = 0;
            current_position.y += current_height;
            current_height = 0;
        }
    }
}

} // namespace alia
