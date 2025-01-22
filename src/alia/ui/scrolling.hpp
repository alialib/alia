#ifndef ALIA_UI_SCROLLING_HPP
#define ALIA_UI_SCROLLING_HPP

#include <alia/core/flow/events.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/layout/simple.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/tosort.hpp>

namespace alia {

// the metrics that define the geometry of a scrollbar - A scrollbar is defined
// as a track with two buttons at the ends and a thumb that moves along the
// track. All of these elements are the same width. Setting the length of the
// buttons to 0 disables them.
struct scrollbar_metrics
{
    // width of the scrollbar track
    layout_scalar width;
    // length of each of the buttons at the ends of the scrollbar
    layout_scalar button_length;
    // the minimum allowable length of the scrollbar thumb
    layout_scalar minimum_thumb_length;

    auto
    operator<=>(scrollbar_metrics const&) const
        = default;
};

scrollbar_metrics
get_scrollbar_metrics(dataless_ui_context ctx);

struct scrollbar_style_info
{
    rgb8 track_color;
    rgb8 track_highlight_color;

    rgb8 thumb_color;
    rgb8 thumb_highlight_color;

    rgb8 button_background_color;
    rgb8 button_foreground_color;
    rgb8 button_highlight_color;
};

scrollbar_style_info
extract_scrollbar_style_info(dataless_ui_context ctx);

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
    optional_context<ui_context> ctx_;
    scrollable_view_data* data_;
    scoped_layout_container container_;
    scoped_component_container component_;
    scoped_clip_region clip_region_;
    scoped_transformation transform_;
};

} // namespace alia

#endif
