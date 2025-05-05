#pragma once

namespace alia {

enum class EventType
{
    Click
};

struct ClickEvent
{
    float x, y; // in logical (DIP) coordinates
};

struct Event
{
    EventType type;
    union
    {
        ClickEvent click;
    };
};

} // namespace alia
