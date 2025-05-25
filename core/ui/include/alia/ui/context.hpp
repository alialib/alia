#pragma once

#include <cstdint>

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
    std::uint32_t count;
    std::uint32_t* next;
};

struct LayoutConsumption
{
    LayoutPlacement* placements;
    std::uint32_t index;
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
