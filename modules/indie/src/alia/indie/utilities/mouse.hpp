#ifndef ALIA_INDIE_UTILITIES_MOUSE_HPP
#define ALIA_INDIE_UTILITIES_MOUSE_HPP

#include <alia/core/flow/events.hpp>
#include <alia/indie/context.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/geometry.hpp>

namespace alia { namespace indie {

bool
is_component_hot(system& sys, component_id id);

inline bool
is_component_hot(dataless_context ctx, component_id id)
{
    return is_component_hot(get_system(ctx), id);
}

bool
component_has_capture(system& sys, component_id id);

inline bool
component_has_capture(dataless_context ctx, component_id id)
{
    return component_has_capture(get_system(ctx), id);
}

bool
no_component_has_capture(system& sys);

inline bool
no_component_has_capture(dataless_context ctx)
{
    return no_component_has_capture(get_system(ctx));
}

vector<2, double>
get_mouse_position(dataless_context ctx);

bool
is_mouse_in_surface(system& sys);

inline bool
is_mouse_in_surface(dataless_context ctx)
{
    return is_mouse_in_surface(get_system(ctx));
}

bool
is_mouse_button_pressed(system& sys, mouse_button button);

inline bool
is_mouse_button_pressed(dataless_context ctx, mouse_button button)
{
    return is_mouse_button_pressed(get_system(ctx), button);
}

bool
detect_mouse_press(dataless_context ctx, mouse_button button);

bool
detect_mouse_press(dataless_context ctx, component_id id, mouse_button button);

bool
detect_mouse_release(dataless_context ctx, mouse_button button);

bool
detect_mouse_release(
    dataless_context ctx, component_id id, mouse_button button);

bool
detect_mouse_motion(dataless_context ctx, component_id id);

bool
detect_double_click(dataless_context ctx, mouse_button button);

bool
detect_double_click(
    dataless_context ctx, component_id id, mouse_button button);

bool
detect_click(dataless_context ctx, component_id id, mouse_button button);

bool
is_click_possible(system& sys, component_id id);

inline bool
is_click_possible(dataless_context ctx, component_id id)
{
    return is_click_possible(get_system(ctx), id);
}

bool
is_click_in_progress(system& sys, component_id id, mouse_button button);

inline bool
is_click_in_progress(
    dataless_context ctx, component_id id, mouse_button button)
{
    return is_click_in_progress(get_system(ctx), id, button);
}

}} // namespace alia::indie

#endif
