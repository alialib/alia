#pragma once

#include <cstdint>

#include <alia/ui/display_list.hpp>
#include <alia/ui/drawing.hpp>

namespace alia {

struct Event;
struct System;
struct LayoutNode;
struct LayoutContainer;
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
    LayoutNode** next_ptr;
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
};

using EphemeralContext = Context;

} // namespace alia
