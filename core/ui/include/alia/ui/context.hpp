#pragma once

#include <cstdint>

#include <alia/ui/display_list.hpp>
#include <alia/ui/drawing.hpp>

namespace alia {

struct Event;
struct System;
struct LayoutNode;
struct LayoutPlacement;

enum class PassType
{
    Refresh,
    Draw,
    Event,
};

struct LayoutEmission
{
    LayoutNode* nodes;
    std::uint32_t count;
    LayoutNode* active_container;
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
    DisplayListArena* display_list_arena;
    BoxCommandList* box_command_list;
    Event* event;
};

struct Context
{
    Pass pass;
    System* system;
};

} // namespace alia
