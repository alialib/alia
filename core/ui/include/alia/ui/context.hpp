#pragma once

#include <alia/ui/display_list.hpp>

namespace alia {

struct Event;
struct DisplayList;
struct System;

enum class PassType
{
    Draw,
    Event,
};

struct Pass
{
    PassType type;
    DisplayList* display_list;
    Event* event;
};

struct Context
{
    Pass pass;
    System* system;
};

} // namespace alia
