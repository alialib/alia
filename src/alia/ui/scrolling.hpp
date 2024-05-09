#ifndef ALIA_UI_SCROLLING_HPP
#define ALIA_UI_SCROLLING_HPP

#include <alia/core/flow/events.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/layout/library.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/tosort.hpp>

namespace alia {

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
    double drag_start_delta = 0;

    // widget identity
    // TODO: Remove
    component_identity identity;

    // for timing button repeats
    timer_data timer;
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

    // index at which the scrollbar can start assigning its own IDs
    int element_id_start;
};

// maximum IDs that the scrollbar will use
int constexpr scrollbar_element_id_count = 5;

struct scrollable_view_data;

struct scoped_scrollable_view
{
    scoped_scrollable_view()
    {
    }
    scoped_scrollable_view(
        ui_context ctx,
        layout const& layout_spec = default_layout,
        unsigned scrollable_axes = 1 | 2,
        unsigned reserved_axes = 0)
    {
        begin(ctx, layout_spec, scrollable_axes, reserved_axes);
    }
    ~scoped_scrollable_view()
    {
        end();
    }

    void
    begin(
        ui_context ctx,
        layout const& layout_spec = default_layout,
        unsigned scrollable_axes = 1 | 2,
        unsigned reserved_axes = 0);
    void
    end();

 private:
    // TODO: Something else.
    ui_context* ctx_;
    scrollable_view_data* data_;
    scoped_layout_container container_;
    scoped_clip_region clip_region_;
    scoped_transformation transform_;
};

} // namespace alia

#endif
