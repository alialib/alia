#ifndef ALIA_UI_UTILITIES_REGIONS_HPP
#define ALIA_UI_UTILITIES_REGIONS_HPP

#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>

// This file provides various utilities for working with input regions.

namespace alia {

// This will handle all region-related events for a widget that occupies a
// rectangular region on the surface.
void
do_box_region(
    dataless_ui_context ctx,
    widget_id id,
    box<2, float> const& region,
    mouse_cursor cursor = mouse_cursor::DEFAULT);
// same, but for double coordinates
void
do_box_region(
    dataless_ui_context ctx,
    widget_id id,
    box<2, double> const& region,
    mouse_cursor cursor = mouse_cursor::DEFAULT);

// Detect if the mouse is inside the given box.
bool
is_mouse_inside_box(dataless_ui_context ctx, box<2, double> const& box);

ALIA_DEFINE_FLAG_TYPE(hit_test)
ALIA_DEFINE_FLAG(hit_test, 0x1, HIT_TEST_MOUSE)
ALIA_DEFINE_FLAG(hit_test, 0x2, HIT_TEST_WHEEL)

// Handle hit testing for a rectangular region of the surface.
void
hit_test_box_region(
    dataless_ui_context ctx,
    widget_id id,
    box<2, float> const& box,
    hit_test_flag_set flags = HIT_TEST_MOUSE,
    mouse_cursor cursor = mouse_cursor::DEFAULT);
// same, but for double coordinates
void
hit_test_box_region(
    dataless_ui_context ctx,
    widget_id id,
    box<2, double> const& box,
    hit_test_flag_set flags = HIT_TEST_MOUSE,
    mouse_cursor cursor = mouse_cursor::DEFAULT);

// Respond to a make_widget_visible_event for a given widget.
void
handle_region_visibility(
    dataless_ui_context ctx, widget_id id, box<2, int> const& box);
// same as above, but for double coordinates
void
handle_region_visibility(
    dataless_ui_context ctx, widget_id id, box<2, double> const& region);

// If you want to work with non-rectangular shapes, you can do hit testing
// yourself and call this when you detect a hit.
// Note that you still need to provide a bounding box for your shape.
void
handle_mouse_hit(
    dataless_ui_context ctx,
    widget_id id,
    box<2, double> const& bounding_box,
    hit_test_flag_set flags = HIT_TEST_MOUSE,
    mouse_cursor cursor = mouse_cursor::DEFAULT);

// If the widget associated with the given ID is hot or active, this will
// set the current mouse cursor to the one specified.
// Note that the events that this handles are part of the REGION_CATEGORY.
// Also note that this should be called after doing hit testing.
void
override_mouse_cursor(
    dataless_ui_context ctx, widget_id id, mouse_cursor cursor);

// Call this to request that a given widget be made visible.
// If the widget is scrolled off screen, this will trigger the containing
// scrollable regions to scroll such that the region is visible.
void
make_widget_visible(
    dataless_ui_context ctx,
    widget_id id,
    widget_visibility_request_flag_set flags = NO_FLAGS);

// Convert a region from the current frame of reference to the surface.
box<2, double>
region_to_surface_coordinates(
    dataless_ui_context ctx, box<2, double> const& region);

// Set the tooltip associated with the given region ID.
// Note that this must be called AFTER hit testing has been performed on the
// region.
void
set_tooltip_message(
    ui_context& ctx,
    widget_id region_id,
    readable<std::string> const& tooltip_message);

} // namespace alia

#endif
