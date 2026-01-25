#pragma once

#include <alia/abi/ui/input/constants.h>
#include <alia/abi/ui/input/regions.h>

#include <alia/context.hpp>
#include <alia/events.hpp>
#include <alia/geometry.hpp>

// This file provides various utilities for working with input regions.

namespace alia {

// This will handle all region-related events for a widget that occupies a
// rectangular region on the surface.
void
box_region(
    ephemeral_context& ctx,
    alia_element_id id,
    alia_box const& region,
    alia_cursor_t cursor = ALIA_CURSOR_DEFAULT);

// Detect if the mouse is inside the given box.
bool
is_mouse_inside_box(ephemeral_context& ctx, alia_box const& box);

// Handle hit testing for a rectangular region of the surface.
void
hit_test_box_region(
    ephemeral_context& ctx,
    alia_element_id id,
    alia_box const& box,
    alia_hit_test_flags_t flags = ALIA_HIT_TEST_MOUSE,
    alia_cursor_t cursor = ALIA_CURSOR_DEFAULT);

// Respond to a make_widget_visible_event for a given widget.
void
handle_region_visibility(
    ephemeral_context& ctx, alia_element_id id, alia_box const& region);

// If you want to work with non-rectangular shapes, you can do hit testing
// yourself and call this when you detect a hit.
// Note that you still need to provide a bounding box for your shape.
void
handle_mouse_hit(
    ephemeral_context& ctx,
    alia_element_id id,
    alia_box const& bounding_box,
    alia_hit_test_flags_t flags = ALIA_HIT_TEST_MOUSE,
    alia_cursor_t cursor = ALIA_CURSOR_DEFAULT);

// If the widget associated with the given ID is hot or active, this will
// set the current mouse cursor to the one specified.
// Note that the events that this handles are part of the REGION_CATEGORY.
// Also note that this should be called after doing hit testing.
void
override_cursor(
    ephemeral_context& ctx, alia_element_id id, alia_cursor_t cursor);

#if 0

// Call this to request that a given widget be made visible.
// If the widget is scrolled off screen, this will trigger the containing
// scrollable regions to scroll such that the region is visible.
void
make_widget_visible(
    ephemeral_context& ctx,
    alia_element_id id,
    widget_visibility_request_flag_set flags = NO_FLAGS);

// Convert a region from the current frame of reference to the surface.
box<2, double>
region_to_surface_coordinates(
    ephemeral_context& ctx, box<2, double> const& region);

// Set the tooltip associated with the given region ID.
// Note that this must be called AFTER hit testing has been performed on the
// region.
void
set_tooltip_message(
    ephemeral_context& ctx,
    alia_element_id region_id,
    readable<std::string> const& tooltip_message);

#endif

} // namespace alia
