#pragma once

#include <cstdint>

#include <alia/ui/display_list.hpp>
#include <alia/ui/drawing.hpp>

namespace alia {

struct Event;
struct System;
struct LayoutNode;
struct LayoutContainer;
struct LayoutPlacementNode;
struct HeterogeneousInfiniteArena;

enum class PassType
{
    Refresh,
    Draw,
    Event,
};

struct LayoutEmission
{
    HeterogeneousInfiniteArena* arena;
    LayoutContainer* active_container;
    LayoutNode** next_ptr;
};

struct LayoutConsumption
{
    LayoutPlacementNode* next_placement;
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
