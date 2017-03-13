#ifndef ALIA_UI_UTILITIES_REGIONS_HPP
#define ALIA_UI_UTILITIES_REGIONS_HPP

#include <alia/ui/internals.hpp>

// This file provides various utilities for working with input regions.

namespace alia {

// This will handle all region-related events for a widget that occupies a
// rectangular region on the surface.
void do_box_region(
    dataless_ui_context& ctx, widget_id id, box<2,int> const& region,
    mouse_cursor cursor = DEFAULT_CURSOR);
// same, but for double coordinates
void do_box_region(
    dataless_ui_context& ctx, widget_id id, box<2,double> const& region,
    mouse_cursor cursor = DEFAULT_CURSOR);

// Detect if the mouse is inside the given box.
bool is_mouse_inside_box(dataless_ui_context& ctx, box<2,double> const& box);

ALIA_DEFINE_FLAG_TYPE(hit_test)
ALIA_DEFINE_FLAG(hit_test, 0x1, HIT_TEST_MOUSE)
ALIA_DEFINE_FLAG(hit_test, 0x2, HIT_TEST_WHEEL)

// Handle hit testing for a rectangular region of the surface.
void hit_test_box_region(dataless_ui_context& ctx, widget_id id,
    box<2,int> const& box, hit_test_flag_set flags = HIT_TEST_MOUSE,
    mouse_cursor cursor = DEFAULT_CURSOR);
// same, but for double coordinates
void hit_test_box_region(dataless_ui_context& ctx, widget_id id,
    box<2,double> const& box, hit_test_flag_set flags = HIT_TEST_MOUSE,
    mouse_cursor cursor = DEFAULT_CURSOR);

// Respond to a make_widget_visible_event for a given widget.
void handle_region_visibility(dataless_ui_context& ctx, widget_id id,
    box<2,int> const& box);
// same as above, but for double coordinates
void handle_region_visibility(dataless_ui_context& ctx, widget_id id,
    box<2,double> const& region);

// If you want to work with non-rectangular shapes, you can do hit testing
// yourself and call this when you detect a hit.
// Note that you still need to provide a bounding box for your shape.
void
handle_mouse_hit(
    dataless_ui_context& ctx,
    widget_id id,
    box<2,double> const& bounding_box,
    hit_test_flag_set flags = HIT_TEST_MOUSE,
    mouse_cursor cursor = DEFAULT_CURSOR);

// If the widget associated with the given ID is hot or active, this will
// set the current mouse cursor to the one specified.
// Note that the events that this handles are part of the REGION_CATEGORY.
// Also note that this should be called after doing hit testing.
void override_mouse_cursor(
    dataless_ui_context& ctx, widget_id id, mouse_cursor cursor);

// Detect if the given ID has captured the mouse, meaning that a mouse
// button was pressed down when the mouse was over the widget with the
// given ID, and the mouse button is still down.
bool is_region_active(dataless_ui_context& ctx, widget_id id);

// Set the ID of the widget that has captured the mouse.
// (This is primarily intended for internal use.)
void set_active_region(ui_system& ui, routable_widget_id const& active_id);

// Detect if the mouse is over the given region.
bool is_region_hot(dataless_ui_context& ctx, widget_id id);

// Set the ID of the widget that the mouse is over.
// (This is primarily intended for internal use.)
void set_hot_region(ui_system& ui, routable_widget_id const& hot_id);

// Call this to request that a given widget be made visible.
// If the widget is scrolled off screen, this will trigger the containing
// scrollable regions to scroll such that the region is visible.
ALIA_DEFINE_FLAG_TYPE(make_widget_visible)
ALIA_DEFINE_FLAG(make_widget_visible, 1, MAKE_WIDGET_VISIBLE_ABRUPTLY)
void make_widget_visible(dataless_ui_context& ctx, widget_id id,
    make_widget_visible_flag_set flags = NO_FLAGS);

// Convert a region from the current frame of reference to the surface.
box<2,double>
region_to_surface_coordinates(dataless_ui_context& ctx,
    box<2,double> const& region);

// Set the tooltip associated with the given region ID.
// Note that this must be called AFTER hit testing has been performed on the region.
void
set_tooltip_message(
    ui_context& ctx,
    widget_id region_id,
    accessor<string> const& tooltip_message);

}

#endif
