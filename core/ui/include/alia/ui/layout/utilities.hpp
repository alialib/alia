#pragma once

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/flags.hpp>
#include <alia/ui/layout/node.hpp>

namespace alia {

WrappingRequirements
default_measure_wrapped_vertical(
    MeasurementContext* ctx,
    LayoutNode* node,
    float current_x_offset,
    float line_width);

struct LayoutAxisPlacement
{
    float offset;
    float size;
};

LayoutAxisPlacement
resolve_horizontal_assignment(
    LayoutFlagSet flags, float assigned_size, float required_size);

LayoutAxisPlacement
resolve_vertical_assignment(
    LayoutFlagSet flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent);

Box
resolve_assignment(
    LayoutFlagSet flags,
    Vec2 assigned_size,
    float baseline,
    Vec2 required_size,
    float ascent);

LayoutAxisPlacement
resolve_padded_horizontal_assignment(
    LayoutFlagSet flags,
    float assigned_size,
    float required_size,
    float padding);

LayoutAxisPlacement
resolve_padded_vertical_assignment(
    LayoutFlagSet flags,
    float assigned_size,
    float baseline,
    float required_size,
    float ascent,
    float padding);

Box
resolve_padded_assignment(
    LayoutFlagSet flags,
    Vec2 assigned_size,
    float baseline,
    Vec2 required_size,
    float ascent,
    float padding);

inline float
resolve_growth_factor(LayoutFlagSet flags)
{
    return flags & GROW ? 1.0f : 0.0f;
}

} // namespace alia
