#ifndef ALIA_UI_UTILITIES_MOUSE_HPP
#define ALIA_UI_UTILITIES_MOUSE_HPP

#include <alia/core/flow/events.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/geometry.hpp>

namespace alia {

// TODO: Move elsewhere.
inline matrix<3, 3, double> const&
get_transformation(dataless_ui_context ctx)
{
    return get_geometry_context(ctx).transformation_matrix;
}

// Get the mouse position in the current frame of reference.
vector<2, double>
get_mouse_position(dataless_ui_context ctx);
// Same, but rounded to integer coordinates.
vector<2, int>
get_integer_mouse_position(dataless_ui_context ctx);

bool
is_element_hot(ui_system& sys, widget_id id);

inline bool
is_element_hot(dataless_ui_context ctx, widget_id id)
{
    return is_element_hot(get_system(ctx), id);
}

bool
element_has_capture(ui_system& sys, widget_id id);

inline bool
element_has_capture(dataless_ui_context ctx, widget_id id)
{
    return element_has_capture(get_system(ctx), id);
}

bool
no_element_has_capture(ui_system& sys);

inline bool
no_element_has_capture(dataless_ui_context ctx)
{
    return no_element_has_capture(get_system(ctx));
}

bool
is_mouse_in_surface(ui_system& sys);

inline bool
is_mouse_in_surface(dataless_ui_context ctx)
{
    return is_mouse_in_surface(get_system(ctx));
}

bool
is_mouse_button_pressed(ui_system& sys, mouse_button button);

inline bool
is_mouse_button_pressed(dataless_ui_context ctx, mouse_button button)
{
    return is_mouse_button_pressed(get_system(ctx), button);
}

bool
detect_mouse_press(dataless_ui_context ctx, mouse_button button);

bool
detect_mouse_press(dataless_ui_context ctx, widget_id id, mouse_button button);

bool
detect_mouse_release(dataless_ui_context ctx, mouse_button button);

bool
detect_mouse_release(
    dataless_ui_context ctx, widget_id id, mouse_button button);

bool
detect_mouse_motion(dataless_ui_context ctx, widget_id id);

bool
detect_double_click(dataless_ui_context ctx, mouse_button button);

bool
detect_double_click(
    dataless_ui_context ctx, widget_id id, mouse_button button);

bool
detect_click(dataless_ui_context ctx, widget_id id, mouse_button button);

bool
is_click_possible(ui_system& sys, widget_id id);

inline bool
is_click_possible(dataless_ui_context ctx, widget_id id)
{
    return is_click_possible(get_system(ctx), id);
}

bool
is_click_in_progress(ui_system& sys, widget_id id, mouse_button button);

inline bool
is_click_in_progress(
    dataless_ui_context ctx, widget_id id, mouse_button button)
{
    return is_click_in_progress(get_system(ctx), id, button);
}

bool
detect_drag(dataless_ui_context ctx, widget_id id, mouse_button button);

bool
detect_press_or_drag(
    dataless_ui_context ctx, widget_id id, mouse_button button);

vector<2, double>
get_mouse_motion_delta(dataless_ui_context ctx, widget const& widget);

bool
is_drag_in_progress(ui_system& sys, widget_id id, mouse_button button);

inline bool
is_drag_in_progress(dataless_ui_context ctx, widget_id id, mouse_button button)
{
    return is_drag_in_progress(get_system(ctx), id, button);
}

std::optional<vector<2, double>>
detect_scroll(dataless_ui_context ctx, widget_id id);

} // namespace alia

#endif
