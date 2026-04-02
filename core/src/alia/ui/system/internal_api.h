#pragma once

#include <alia/context.h>
#include <functional>

#include <alia/ui/system/object.h>

extern "C" {
// TODO: This isn't technically correct.
typedef struct alia_ui_system alia_ui_system;
} // extern "C"

namespace alia {

struct os_interface;
struct window_interface;

#if 0

// Get the number of milliseconds until the UI expects to update next.
// The system can safely idle for this many milliseconds if no external events
// occur.
// If the return value is none, it means that there are no future updates
// scheduled, so the system can simply sleep until the next external event.
std::optional<millisecond_count>
get_time_until_next_update(ui_system& system, millisecond_count now);

// Render the UI to the associated surface.
void
render_ui(ui_system& system);

#endif

// Move the keyboard focus forward and backwards through the focus order.
void
advance_focus(ui_system& ui);
void
regress_focus(ui_system& ui);

// Clear the keyboard focus (so that no widget has focus).
void
clear_focus(ui_system& ui);

// Issue a refresh event to the UI system.
// Note that this is called as part of update_ui, so it's normally not
// necessary to call this separately.
void
refresh_system(ui_system& ui);

#if 0

// Issue an untargeted event to the UI system.
void
issue_event(ui_system& system, ui_event& event);

// Issue a targeted event to the UI system.
void
issue_targeted_event(
    ui_system& system, ui_event& event, routable_widget_id const& target);

// Get the time in microseconds that the last refresh event took to process.
int
get_last_refresh_duration(ui_system& ui);

// Set a new style for the UI system.
void
set_system_style(ui_system& system, alia__shared_ptr<style_tree> const& style);

inline void
on_ui_style_change(ui_system& system)
{
    inc_version(system.style.id);
}

#endif

void
set_focus(ui_system& sys, alia_element_id element);

// Set the element that has the mouse captured.
void
set_element_with_capture(ui_system& ui, alia_element_id element);

// Set the element that's under the mouse.
void
set_hot_element(ui_system& ui, alia_element_id element);

} // namespace alia
