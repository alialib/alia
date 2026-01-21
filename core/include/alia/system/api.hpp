#pragma once

#include <alia/context.hpp>
#include <functional>

// TODO: Use forward declarations once those are sorted out.
#include <alia/system/object.hpp>

namespace alia {

struct os_interface;
struct window_interface;

// Initialize the UI system.
void
initialize(
    ui_system& system,
    std::function<void(context&)> controller /*,
    external_interface* external,
    std::shared_ptr<os_interface> os,
    std::shared_ptr<window_interface> window*/);
// TODO: Add other initialization arguments...
//     // alia__shared_ptr<alia::surface> const& surface,
//     vector<2, float> const& ppi,
//     alia__shared_ptr<style_tree> const& style);

// Update the UI system.
// This detects changes in the UI contents and updates the layout of the UI.
// It also resolves what's under the mouse cursor and updates the UI
// accordingly.
void
update(ui_system& ui);

// Handle a change in the size of the window.
void
update_window_size(ui_system& ui, alia_vec2f new_size);

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

// The following are for sending keyboard events to the UI.
bool
process_text_input(
    ui_system& ui, millisecond_count time, utf8_string const& text);
bool
process_key_press(
    ui_system& ui, millisecond_count time, key_event_info const& info);
bool
process_key_release(
    ui_system& ui, millisecond_count time, key_event_info const& info);
void
process_focus_loss(ui_system& ui, millisecond_count time);
void
process_focus_gain(ui_system& ui, millisecond_count time);

#endif

// Move the keyboard focus forward and backwards through the focus order.
void
advance_focus(ui_system& ui);
void
regress_focus(ui_system& ui);

// Clear the keyboard focus (so that no widget has focus).
void
clear_focus(ui_system& ui);

#if 0

// Issue an untargeted event to the UI system.
void
issue_event(ui_system& system, ui_event& event);

// Issue a targeted event to the UI system.
void
issue_targeted_event(
    ui_system& system, ui_event& event, routable_widget_id const& target);

// Issue a refresh event to the UI system.
// Note that this is called as part of update_ui, so it's normally not
// necessary to call this separately.
void
refresh_ui(ui_system& ui);

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

// Set the element that has the mouse captured.
void
set_element_with_capture(ui_system& ui, alia_routable_element_id element);

// Set the element that's under the mouse.
void
set_hot_element(ui_system& ui, alia_routable_element_id element);

} // namespace alia
