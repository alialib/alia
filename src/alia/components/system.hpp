#ifndef ALIA_COMPONENTS_SYSTEM_HPP
#define ALIA_COMPONENTS_SYSTEM_HPP

#include <functional>

#include <alia/components/context.hpp>
#include <alia/components/timing.hpp>
#include <alia/flow/data_graph.hpp>

namespace alia {

millisecond_count
get_default_tick_count();

struct external_interface
{
    // alia calls this every frame when an animation is in progress.
    virtual void
    request_animation_refresh()
    {
    }

    // Get the current value of the system's millisecond tick counter.
    // The default implementation of this uses std::chrono::steady_clock.
    virtual millisecond_count
    get_tick_count() const
    {
        return get_default_tick_count();
    }
};

struct system
{
    data_graph data;
    std::function<void(context)> controller;
    bool refresh_needed = false;
    external_interface* external = nullptr;
};

inline bool
system_needs_refresh(system const& sys)
{
    return sys.refresh_needed;
}

void
refresh_system(system& sys);

} // namespace alia

#endif
