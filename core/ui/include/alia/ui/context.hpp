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
struct InfiniteArena;

enum class PassType
{
    Refresh,
    Draw,
    Event,
};

struct LayoutEmission
{
    InfiniteArena* arena;
    LayoutContainer* active_container;
    LayoutNode** next_ptr;
};

struct LayoutConsumption
{
    LayoutPlacementNode* next_placement;
};

struct Style
{
    float padding;
};

struct RefreshPass
{
    LayoutEmission layout_emission;
};

struct DrawPass
{
    DisplayListArena* display_list_arena;
    BoxCommandList* box_command_list;
};

struct EventPass
{
    Event* event;
};

struct Pass
{
    PassType type;
    union
    {
        RefreshPass refresh;
        DrawPass draw;
        EventPass event;
    };
};

struct Context
{
    Pass pass;
    Style* style;
    System* system;
    LayoutConsumption layout_consumption;
};

} // namespace alia
