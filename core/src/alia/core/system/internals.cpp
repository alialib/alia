#include <alia/core/system/internals.hpp>

#include <chrono>

#include <alia/core/flow/top_level.hpp>

namespace alia {

millisecond_count
external_interface::get_tick_count() const
{
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<
               std::chrono::duration<millisecond_count, std::milli>>(
               now - start)
        .count();
}

void
default_external_interface::schedule_timer_event(
    external_component_id component, millisecond_count time)
{
    schedule_event(owner.scheduler, component, time);
}

void
default_external_interface::schedule_asynchronous_update(
    std::function<void()> update)
{
    update();
    refresh_system(owner);
}

void
initialize_standalone_system(
    system& sys,
    std::function<void(context)> const& controller,
    external_interface* external)
{
    initialize_core_system(sys, external);
    sys.controller = controller;
}

} // namespace alia
