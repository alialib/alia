#ifndef ALIA_INDIE_SCROLLING_HPP
#define ALIA_INDIE_SCROLLING_HPP

#include <alia/indie/layout/containers/utilities.hpp>
#include <alia/indie/tosort.hpp>

namespace alia { namespace indie {

struct scrollbar_metrics
{
    layout_scalar width, button_length, minimum_thumb_length;
};

// persistent data maintained for a scrollbar
struct scrollbar_data
{
    // the actual scroll position - If the caller wants to provide an external
    // signal, it will be sync'd with this.
    layout_scalar scroll_position;

    // If this is true, the scroll_position has changed internally and needs
    // to be communicated to the external storage.
    bool scroll_position_changed = false;

    // the smoothed version of the scroll position
    layout_scalar smoothed_scroll_position;
    // for smoothing the scroll position
    value_smoother<layout_scalar> smoother;

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
    // persistent data for the scrollbar
    scrollbar_data* data;

    // 0 for horizontal, 1 for vertical
    unsigned axis;

    // surface region assigned to the scrollbar
    layout_box area;

    // total size of the content along the scrolling axis
    layout_scalar content_size;

    // size of the window through which we view the content
    layout_scalar window_size;

    // ref to the widget that owns this scrollbar
    internal_widget_ref widget;

    // index at which the scrollbar can start assigning its own IDs
    int element_id_start;
};

// maximum IDs that the scrollbar will use
int constexpr scrollbar_element_id_count = 5;

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

struct scoped_scrollable_view
{
    scoped_scrollable_view()
    {
    }
    scoped_scrollable_view(
        indie::context ctx, layout const& layout_spec = default_layout)
    {
        begin(ctx, layout_spec);
    }
    ~scoped_scrollable_view()
    {
        end();
    }

    void
    begin(indie::context ctx, layout const& layout_spec = default_layout);
    void
    end();

 private:
    scoped_layout_container container_;
};

}} // namespace alia::indie

#endif
