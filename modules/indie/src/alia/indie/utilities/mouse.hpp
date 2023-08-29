#ifndef ALIA_INDIE_UTILITIES_MOUSE_HPP
#define ALIA_INDIE_UTILITIES_MOUSE_HPP

#include <alia/core/flow/events.hpp>
#include <alia/indie/context.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/geometry.hpp>

namespace alia { namespace indie {

bool
is_widget_hot(system& sys, widget const* widget);

inline bool
is_widget_hot(dataless_context ctx, widget const* widget)
{
    return is_widget_hot(get_system(ctx), widget);
}

bool
widget_has_capture(system& sys, widget const* widget);

inline bool
widget_has_capture(dataless_context ctx, widget const* widget)
{
    return widget_has_capture(get_system(ctx), widget);
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
detect_mouse_press(
    dataless_context ctx, widget const* widget, mouse_button button);

bool
detect_mouse_release(dataless_context ctx, mouse_button button);

bool
detect_mouse_release(
    dataless_context ctx, widget const* widget, mouse_button button);

bool
detect_mouse_motion(dataless_context ctx, widget const* widget);

bool
detect_double_click(dataless_context ctx, mouse_button button);

bool
detect_double_click(
    dataless_context ctx, widget const* widget, mouse_button button);

bool
detect_click(dataless_context ctx, widget const* widget, mouse_button button);

bool
is_click_possible(system& sys, widget const* widget);

inline bool
is_click_possible(dataless_context ctx, widget const* widget)
{
    return is_click_possible(get_system(ctx), widget);
}

bool
is_click_in_progress(system& sys, widget const* widget, mouse_button button);

inline bool
is_click_in_progress(
    dataless_context ctx, widget const* widget, mouse_button button)
{
    return is_click_in_progress(get_system(ctx), widget, button);
}

}} // namespace alia::indie

#endif
