#pragma once

#include <cstddef>

namespace alia {

struct Event;
struct DisplayList;
struct System;
struct LayoutSpec;
struct LayoutPlacement;

enum class PassType
{
    Refresh,
    Draw,
    Event,
};

struct LayoutEmission
{
    LayoutSpec* specs;
    std::size_t count;
};

struct LayoutConsumption
{
    LayoutPlacement* placements;
    std::size_t index;
};

struct Pass
{
    PassType type;
    // TODO: Sort this out.
    LayoutEmission layout_emission;
    LayoutConsumption layout_consumption;
    DisplayList* display_list;
    Event* event;
};

struct Context
{
    Pass pass;
    System* system;
};

} // namespace alia
