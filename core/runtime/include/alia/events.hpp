#pragma once

namespace alia {

enum class event_type
{
    Click
};

struct click_event
{
    float x, y; // in logical (DIP) coordinates
};

struct event
{
    event_type type;
    union
    {
        click_event click;
    };
};

} // namespace alia
