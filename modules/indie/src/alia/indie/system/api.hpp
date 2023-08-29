#ifndef ALIA_INDIE_SYSTEM_API_HPP
#define ALIA_INDIE_SYSTEM_API_HPP

#include <alia/core/flow/events.hpp>
#include <alia/indie/widget.hpp>

namespace alia { namespace indie {

struct system;
struct os_interface;
struct window_interface;

// Initialize the UI system.
void
initialize(
    indie::system& system,
    std::function<void(indie::context)> controller,
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
update(system& ui);

// Handle a change in the size of the window.
void
update_window_size(system& ui, vector<2, unsigned> const& new_size);

#if 0

// Returns true iff there are pending timer requests.
bool
has_timer_requests(system& ui);

// Check expired timer requests in the UI and issue the corresponding events.
// The return value is true iff any requests were processed.
bool
process_timer_requests(system& system, millisecond_count now);

// Get the number of milliseconds until the UI expects to update next.
// The system can safely idle for this many milliseconds if no external events
// occur.
// If the return value is none, it means that there are no future updates
// scheduled, so the system can simply sleep until the next external event.
std::optional<millisecond_count>
get_time_until_next_update(system& system, millisecond_count now);

// Render the UI to the associated surface.
void
render_ui(system& system);

// The following are for sending keyboard events to the UI.
bool
process_text_input(
    system& ui, millisecond_count time, utf8_string const& text);
bool
process_key_press(
    system& ui, millisecond_count time, key_event_info const& info);
bool
process_key_release(
    system& ui, millisecond_count time, key_event_info const& info);
void
process_focus_loss(system& ui, millisecond_count time);
void
process_focus_gain(system& ui, millisecond_count time);

// process_key_press calls both of the following.
// process_focused_key_press will only pass the key to the widget with the
// keyboard focus (if any).
// process_background_key_press will pass it to any widget that's listening.
bool
process_focused_key_press(
    system& ui, millisecond_count time, key_event_info const& info);
bool
process_background_key_press(
    system& ui, millisecond_count time, key_event_info const& info);

#endif

// Move the keyboard focus forward and backwards through the focus order.
void
advance_focus(system& ui);
void
regress_focus(system& ui);

// Clear the keyboard focus (so that no widget has focus).
void
clear_focus(system& ui);

#if 0

// Issue an untargeted event to the UI system.
void
issue_event(system& system, ui_event& event);

// Issue a targeted event to the UI system.
void
issue_targeted_event(
    system& system, ui_event& event, routable_widget_id const& target);

// Issue a refresh event to the UI system.
// Note that this is called as part of update_ui, so it's normally not
// necessary to call this separately.
void
refresh_ui(system& ui);

// Get the time in microseconds that the last refresh event took to process.
int
get_last_refresh_duration(system& ui);

// Set a new style for the UI system.
void
set_system_style(system& system, alia__shared_ptr<style_tree> const& style);

static inline void
on_ui_style_change(system& system)
{
    inc_version(system.style.id);
}

#endif

// Set the widget that has the mouse captured.
void
set_widget_with_capture(system& ui, external_widget_handle widget);

// Set the widget that's under the mouse.
void
set_hot_widget(system& ui, external_widget_handle widget);

}} // namespace alia::indie

#endif
