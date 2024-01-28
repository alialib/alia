#ifndef ALIA_UI_UTILITIES_MOUSE_HPP
#define ALIA_UI_UTILITIES_MOUSE_HPP

#include <alia/core/flow/events.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events/input.hpp>
#include <alia/ui/geometry.hpp>

namespace alia {

bool
is_element_hot(ui_system& sys, internal_element_ref element);

inline bool
is_element_hot(ui_event_context ctx, internal_element_ref element)
{
    return is_element_hot(get_system(ctx), element);
}

bool
element_has_capture(ui_system& sys, internal_element_ref element);

inline bool
element_has_capture(ui_event_context ctx, internal_element_ref element)
{
    return element_has_capture(get_system(ctx), element);
}

bool
no_element_has_capture(ui_system& sys);

inline bool
no_element_has_capture(ui_event_context ctx)
{
    return no_element_has_capture(get_system(ctx));
}

vector<2, double>
get_mouse_position(ui_system& sys, widget const& widget);

vector<2, int>
get_integer_mouse_position(ui_system& sys, widget const& widget);

bool
is_mouse_in_surface(ui_system& sys);

inline bool
is_mouse_in_surface(ui_event_context ctx)
{
    return is_mouse_in_surface(get_system(ctx));
}

bool
is_mouse_button_pressed(ui_system& sys, mouse_button button);

inline bool
is_mouse_button_pressed(ui_event_context ctx, mouse_button button)
{
    return is_mouse_button_pressed(get_system(ctx), button);
}

bool
detect_mouse_press(ui_event_context ctx, mouse_button button);

bool
detect_mouse_press(
    ui_event_context ctx, internal_element_ref element, mouse_button button);

bool
detect_mouse_release(ui_event_context ctx, mouse_button button);

bool
detect_mouse_release(
    ui_event_context ctx, internal_element_ref element, mouse_button button);

bool
detect_mouse_motion(ui_event_context ctx, internal_element_ref element);

bool
detect_double_click(ui_event_context ctx, mouse_button button);

bool
detect_double_click(
    ui_event_context ctx, internal_element_ref element, mouse_button button);

bool
detect_click(
    ui_event_context ctx, internal_element_ref element, mouse_button button);

bool
is_click_possible(ui_system& sys, internal_element_ref element);

inline bool
is_click_possible(ui_event_context ctx, internal_element_ref element)
{
    return is_click_possible(get_system(ctx), element);
}

bool
is_click_in_progress(
    ui_system& sys, internal_element_ref element, mouse_button button);

inline bool
is_click_in_progress(
    ui_event_context ctx, internal_element_ref element, mouse_button button)
{
    return is_click_in_progress(get_system(ctx), element, button);
}

bool
detect_drag(
    ui_event_context ctx, internal_element_ref element, mouse_button button);

bool
detect_press_or_drag(
    ui_event_context ctx, internal_element_ref element, mouse_button button);

vector<2, double>
get_mouse_motion_delta(ui_event_context ctx, widget const& widget);

bool
is_drag_in_progress(
    ui_system& sys, internal_element_ref element, mouse_button button);

inline bool
is_drag_in_progress(
    ui_event_context ctx, internal_element_ref element, mouse_button button)
{
    return is_drag_in_progress(get_system(ctx), element, button);
}

} // namespace alia

#endif
