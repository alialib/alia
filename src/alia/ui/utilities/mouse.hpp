#ifndef ALIA_INDIE_UTILITIES_MOUSE_HPP
#define ALIA_INDIE_UTILITIES_MOUSE_HPP

#include <alia/core/flow/events.hpp>
#include <alia/indie/context.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/geometry.hpp>

namespace alia { namespace indie {

bool
is_element_hot(system& sys, internal_element_ref element);

inline bool
is_element_hot(event_context ctx, internal_element_ref element)
{
    return is_element_hot(get_system(ctx), element);
}

bool
element_has_capture(system& sys, internal_element_ref element);

inline bool
element_has_capture(event_context ctx, internal_element_ref element)
{
    return element_has_capture(get_system(ctx), element);
}

bool
no_element_has_capture(system& sys);

inline bool
no_element_has_capture(event_context ctx)
{
    return no_element_has_capture(get_system(ctx));
}

vector<2, double>
get_mouse_position(system& sys, widget const& widget);

vector<2, int>
get_integer_mouse_position(system& sys, widget const& widget);

bool
is_mouse_in_surface(system& sys);

inline bool
is_mouse_in_surface(event_context ctx)
{
    return is_mouse_in_surface(get_system(ctx));
}

bool
is_mouse_button_pressed(system& sys, mouse_button button);

inline bool
is_mouse_button_pressed(event_context ctx, mouse_button button)
{
    return is_mouse_button_pressed(get_system(ctx), button);
}

bool
detect_mouse_press(event_context ctx, mouse_button button);

bool
detect_mouse_press(
    event_context ctx, internal_element_ref element, mouse_button button);

bool
detect_mouse_release(event_context ctx, mouse_button button);

bool
detect_mouse_release(
    event_context ctx, internal_element_ref element, mouse_button button);

bool
detect_mouse_motion(event_context ctx, internal_element_ref element);

bool
detect_double_click(event_context ctx, mouse_button button);

bool
detect_double_click(
    event_context ctx, internal_element_ref element, mouse_button button);

bool
detect_click(
    event_context ctx, internal_element_ref element, mouse_button button);

bool
is_click_possible(system& sys, internal_element_ref element);

inline bool
is_click_possible(event_context ctx, internal_element_ref element)
{
    return is_click_possible(get_system(ctx), element);
}

bool
is_click_in_progress(
    system& sys, internal_element_ref element, mouse_button button);

inline bool
is_click_in_progress(
    event_context ctx, internal_element_ref element, mouse_button button)
{
    return is_click_in_progress(get_system(ctx), element, button);
}

bool
detect_drag(
    event_context ctx, internal_element_ref element, mouse_button button);

bool
detect_press_or_drag(
    event_context ctx, internal_element_ref element, mouse_button button);

vector<2, double>
get_mouse_motion_delta(event_context ctx, widget const& widget);

bool
is_drag_in_progress(
    system& sys, internal_element_ref element, mouse_button button);

inline bool
is_drag_in_progress(
    event_context ctx, internal_element_ref element, mouse_button button)
{
    return is_drag_in_progress(get_system(ctx), element, button);
}

}} // namespace alia::indie

#endif
