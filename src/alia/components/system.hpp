#ifndef ALIA_COMPONENTS_SYSTEM_HPP
#define ALIA_COMPONENTS_SYSTEM_HPP

#include <functional>

#include <alia/components/context.hpp>
#include <alia/flow/data_graph.hpp>

namespace alia {

struct external_interface
{
    // alia calls this every frame when an animation is in progress.
    virtual void
    request_animation_refresh()
    {
    }
};

struct system
{
    data_graph data;
    std::function<void(context)> controller;
    millisecond_count tick_counter = 0;
    bool automatic_time_updates = true;
    bool refresh_needed = false;
    external_interface* external = nullptr;
};

void
refresh_system(system& sys);

void
refresh_system_time(system& sys);

inline void
set_automatic_time_updates(system& sys, bool enabled)
{
    sys.automatic_time_updates = enabled;
}

inline void
set_millisecond_tick_counter(system& sys, millisecond_count count)
{
    sys.tick_counter = count;
}

inline bool
system_needs_refresh(system const& sys)
{
    return sys.refresh_needed;
}

} // namespace alia

#endif
