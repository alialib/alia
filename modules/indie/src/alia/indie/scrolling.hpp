#ifndef ALIA_INDIE_SCROLLING_HPP
#define ALIA_INDIE_SCROLLING_HPP

#include <alia/indie/tosort.hpp>

namespace alia { namespace indie {

struct scrollbar_metrics
{
    layout_scalar width, button_length, minimum_thumb_length;
};

// persistent data maintained for a scrollbar
struct scrollbar_data
{
    // cached metrics
    keyed_data<scrollbar_metrics> metrics;

    // the relative position of the thumb within its track, in pixels
    layout_scalar physical_position = 0;

    // While dragging, this stores the offset from the mouse cursor to the
    // top of the thumb.
    layout_scalar drag_start_delta = 0;
};

// all the parameters that are necessary to define a scrollbar
struct scrollbar_parameters
{
    scrollbar_data* data;
    unsigned axis;
    duplex<layout_scalar> const* scroll_position;
    layout_box area;
    layout_scalar content_size, window_size, line_increment, page_increment;
    internal_widget_ref widget;
    int element_id_start;
};

void
render_scrollbar(render_event& event, scrollbar_parameters const& sb);

void
hit_test_scrollbar(
    scrollbar_parameters const& sb,
    hit_test_base& test,
    vector<2, double> const& point);

void
process_scrollbar_input(event_context ctx, scrollbar_parameters const& sb);

void
refresh_scrollbar(dataless_context ctx, scrollbar_parameters const& sb);

}} // namespace alia::indie

#endif
