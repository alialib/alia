#ifndef ALIA_UI_SYSTEM_HPP
#define ALIA_UI_SYSTEM_HPP

#include <alia/ui_definitions.hpp>
#include <alia/layout_system.hpp>
#include <alia/dispatch_table.hpp>
#include <alia/color.hpp>

#include <boost/optional.hpp>

namespace alia {

struct parse_error : exception
{
    parse_error(string const& message)
      : exception(message)
    {}
    ~parse_error() throw() {}
};

rgba8 parse_color_value(char const* value);

void set_style(style_tree& tree, string const& subpath,
    property_map const& properties);

// TODO: Implement style files.
//void parse_style_file(style* style, char const* file);

void set_text_magnification(ui_system& system, float magnification);

void refresh_and_layout(ui_system& system, vector<2,unsigned> const& size,
    ui_time_type millisecond_tick_count);

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

bool process_character_input(ui_system& ui, ui_time_type time, char_type c);
bool process_key_press(ui_system& ui, ui_time_type time,
    key_event_info const& info);
bool process_key_release(ui_system& ui, ui_time_type time,
    key_event_info const& info);
void process_focus_loss(ui_system& ui, ui_time_type time);
void process_focus_gain(ui_system& ui, ui_time_type time);

routable_widget_id do_hit_test(ui_system& ui,
    vector<2,double> const& position);

// Determine which widget is under the mouse and what cursor should be active.
boost::optional<mouse_cursor> update_mouse_cursor(ui_system& ui);

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
