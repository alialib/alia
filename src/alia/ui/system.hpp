#ifndef ALIA_UI_SYSTEM_HPP
#define ALIA_UI_SYSTEM_HPP

#include <alia/ui/internals.hpp>

// This files defines the top-level interface to the UI system.
// It's used by backends.

namespace alia {

// Initialize the UI system.
void initialize_ui(
    ui_system& ui,
    alia__shared_ptr<ui_controller> const& controller,
    alia__shared_ptr<alia::surface> const& surface,
    vector<2,float> const& ppi,
    alia__shared_ptr<os_interface> const& os,
    alia__shared_ptr<style_tree> const& style);

// Update the UI system.
// This detects changes in the UI contents and updates the layout of the UI.
// It also resolves what's under the mouse cursor and updates the UI
// accordingly.
// If current_cursor is not NULL, it will receive the cursor that the UI has
// requested for the current frame.
void update_ui(ui_system& ui, vector<2,unsigned> const& size,
    ui_time_type millisecond_tick_count, mouse_cursor* current_cursor = 0);

// Returns true iff there are pending timer requests.
bool has_timer_requests(ui_system& ui);

// Check expired timer requests in the UI and issue the corresponding events.
// The return value is true iff any requests were processed.
bool process_timer_requests(ui_system& system, ui_time_type now);

// Get the number of milliseconds until the UI expects to update next.
// The system can safely idle for this many milliseconds if no external events
// occur.
// If the return value is none, it means that there are no future updates
// scheduled, so the system can simply sleep until the next external event.
optional<ui_time_type>
get_time_until_next_update(ui_system& system, ui_time_type now);

// Render the UI to the associated surface.
void render_ui(ui_system& system);

// The following are for sending mouse events to the UI.
void process_mouse_move(
    ui_system& ui, ui_time_type time, vector<2,int> const& position);
void process_mouse_leave(
    ui_system& ui, ui_time_type time);
void process_mouse_press(
    ui_system& ui, ui_time_type time,
    vector<2,int> const& position, mouse_button button);
void process_mouse_release(
    ui_system& ui, ui_time_type time,
    vector<2,int> const& position, mouse_button button);
void process_double_click(
    ui_system& ui, ui_time_type time,
    vector<2,int> const& position, mouse_button button);
void process_mouse_wheel(
    ui_system& ui, ui_time_type time, float movement);

// The following are for sending keyboard events to the UI.
bool process_text_input(
    ui_system& ui, ui_time_type time, utf8_string const& text);
bool process_key_press(
    ui_system& ui, ui_time_type time, key_event_info const& info);
bool process_key_release(
    ui_system& ui, ui_time_type time, key_event_info const& info);
void process_focus_loss(
    ui_system& ui, ui_time_type time);
void process_focus_gain(
    ui_system& ui, ui_time_type time);

// process_key_press calls both of the following.
// process_focused_key_press will only pass the key to the widget with the
// keyboard focus (if any).
// process_background_key_press will pass it to any widget that's listening.
bool process_focused_key_press(
    ui_system& ui, ui_time_type time, key_event_info const& info);
bool process_background_key_press(
    ui_system& ui, ui_time_type time, key_event_info const& info);

// Move the keyboard focus forward and backwards through the focus order.
void advance_focus(ui_system& ui);
void regress_focus(ui_system& ui);

// Clear the keyboard focus (so that no widget has focus).
void clear_focus(ui_system& ui);

// Issue an untargeted event to the UI system.
void issue_event(ui_system& system, ui_event& event);

// Issue a targeted event to the UI system.
void issue_targeted_event(ui_system& system, ui_event& event,
    routable_widget_id const& target);

// Issue a refresh event to the UI system.
// Note that this is called as part of update_ui, so it's normally not
// necessary to call this separately.
void refresh_ui(ui_system& ui);

// Get the time in microseconds that the last refresh event took to process.
int get_last_refresh_duration(ui_system& ui);

// Set a new style for the UI system.
void set_system_style(ui_system& system,
    alia__shared_ptr<style_tree> const& style);

static inline void on_ui_style_change(ui_system& system)
{
    inc_version(system.style.id);
}

}

#endif
