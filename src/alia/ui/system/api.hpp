#ifndef ALIA_UI_SYSTEM_API_HPP
#define ALIA_UI_SYSTEM_API_HPP

#include <alia/core/flow/events.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>

namespace alia {

struct ui_system;
struct os_interface;
struct window_interface;

// Initialize the UI system.
void
initialize(
    ui_system& system,
    std::function<void(ui_context)> controller,
    std::shared_ptr<os_interface> os,
    std::shared_ptr<window_interface> window);

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
update_window_size(ui_system& ui, vector<2, unsigned> const& new_size);

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

// process_key_press calls both of the following.
// process_focused_key_press will only pass the key to the widget with the
// keyboard focus (if any).
// process_background_key_press will pass it to any widget that's listening.
bool
process_focused_key_press(
    ui_system& ui, millisecond_count time, key_event_info const& info);
bool
process_background_key_press(
    ui_system& ui, millisecond_count time, key_event_info const& info);

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

// Set the widget that has the mouse captured.
void
set_widget_with_capture(ui_system& ui, routable_widget_id widget);

// Set the widget that's under the mouse.
void
set_hot_widget(ui_system& ui, routable_widget_id widget);

} // namespace alia

#endif
