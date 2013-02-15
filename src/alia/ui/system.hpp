#ifndef ALIA_UI_SYSTEM_HPP
#define ALIA_UI_SYSTEM_HPP

#include <alia/ui/internals.hpp>

// This files defines the top-level interface to the UI system.
// It's used by backends.

namespace alia {

float get_text_magnification(ui_system& system);

void set_text_magnification(ui_system& system, float magnification);

void refresh_ui(ui_system& ui);

void update_ui(ui_system& ui, vector<2,unsigned> const& size,
    ui_time_type millisecond_tick_count, mouse_cursor* current_cursor = 0);

void render_ui(ui_system& system);

void issue_event(ui_system& system, ui_event& event);

void issue_targeted_event(ui_system& system, ui_event& event,
    routable_widget_id const& target);

void process_mouse_move(ui_system& ui, ui_time_type time,
    vector<2,int> const& position);
void process_mouse_leave(ui_system& ui, ui_time_type time);
void process_mouse_press(ui_system& ui, ui_time_type time,
    vector<2,int> const& position, mouse_button button);
void process_mouse_release(ui_system& ui, ui_time_type time,
    vector<2,int> const& position, mouse_button button);
void process_double_click(ui_system& ui, ui_time_type time,
    vector<2,int> const& position, mouse_button button);
void process_mouse_wheel(ui_system& ui, ui_time_type time, float movement);

bool process_text_input(ui_system& ui, ui_time_type time,
    utf8_string const& text);
bool process_key_press(ui_system& ui, ui_time_type time,
    key_event_info const& info);
bool process_key_release(ui_system& ui, ui_time_type time,
    key_event_info const& info);
void process_focus_loss(ui_system& ui, ui_time_type time);
void process_focus_gain(ui_system& ui, ui_time_type time);

// Determine the minimum size of the initial UI defined by the given
// controller, style, and surface.
layout_vector measure_initial_ui(
    alia__shared_ptr<ui_controller> const& controller,
    alia__shared_ptr<ui_style> const& style,
    alia__shared_ptr<surface> const& surface);

void clear_focus(ui_system& ui);

void advance_focus(ui_system& ui);
void regress_focus(ui_system& ui);

// Call this after the context is initialized to set the focus to the first
// widget in the context.
void set_initial_focus(ui_context& ctx);

}

#endif
