#ifndef ALIA_UI_UTILITIES_REGIONS_HPP
#define ALIA_UI_UTILITIES_REGIONS_HPP

#include <alia/ui/internals.hpp>

// This file provides various utilities for working with input regions.

namespace alia {

// This will handle all region-related events for a widget that occupies a
// rectangular region on the surface.
void do_box_region(
    ui_context& ctx, widget_id id, box<2,int> const& box,
    mouse_cursor cursor = DEFAULT_CURSOR);

// Detect if the mouse is inside the given box.
bool mouse_is_inside_box(ui_context& ctx, box<2,double> const& box);

struct hit_test_flag_tag {};
typedef flag_set<hit_test_flag_tag> hit_test_flag_set;
ALIA_DEFINE_FLAG_CODE(hit_test_flag_tag, 0x1, HIT_TEST_MOUSE)
ALIA_DEFINE_FLAG_CODE(hit_test_flag_tag, 0x2, HIT_TEST_WHEEL)

// Handle hit testing for a rectangular region of the surface.
void hit_test_box_region(ui_context& ctx, widget_id id,
    box<2,int> const& box, hit_test_flag_set flags = HIT_TEST_MOUSE,
    mouse_cursor cursor = DEFAULT_CURSOR);

// Respond to a make_widget_visible_event event for a given widget.
void do_region_visibility(ui_context& ctx, widget_id id,
    box<2,int> const& box);

// If you want to work with non-rectangular shapes, you can do hit testing
// yourself and call this when you detect a hit.
void handle_mouse_hit(ui_context& ctx, widget_id id,
    hit_test_flag_set flags = HIT_TEST_MOUSE,
    mouse_cursor cursor = DEFAULT_CURSOR);

// If the widget associated with the given ID is hot or active, this will
// set the current mouse cursor to the one specified.
// Note that the events that this handles are part of the REGION_CATEGORY.
// Also note that this should be called after doing hit testing.
void override_mouse_cursor(ui_context& ctx, widget_id id, mouse_cursor cursor);

// Detect if the given ID has captured the mouse, meaning that a mouse
// button was pressed down when the mouse was over the widget with the
// given ID, and the mouse button is still down.
bool is_region_active(ui_context& ctx, widget_id id);

// Detect if the mouse is over the given region.
bool is_region_hot(ui_context& ctx, widget_id id);

// Call this to request that a given region on the canvas be made visible.
// If the region is scrolled off screen, this will trigger the containing
// scrollable regions to scroll such that the region is visible.
void make_widget_visible(ui_context& ctx, widget_id id);

}

#endif
