#ifndef ALIA_UI_LIBRARY_PANELS_HPP
#define ALIA_UI_LIBRARY_PANELS_HPP

#include <alia/core/common.hpp>
#include <alia/ui/color.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/layout/simple.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/utilities/widgets.hpp>

namespace alia {

// This section declares utilities for implementing panels.

struct panel_style_info
{
    // absolute_size size;
    box_border_width<float> margin, border_width, padding;
    // bool is_rounded;
    // rgba8 border_color, background_color, gradient_color;
    // TODO
    // box_corner_sizes border_radii;

    auto
    operator<=>(panel_style_info const&) const
        = default;
};

// Currently all panel types share the same flag set, but some flags obviously
// only apply to certain types.
ALIA_DEFINE_FLAG_TYPE(panel)
ALIA_DEFINE_FLAG(panel, 0x00001, PANEL_HORIZONTAL)
ALIA_DEFINE_FLAG(panel, 0x00002, PANEL_VERTICAL)
ALIA_DEFINE_FLAG(panel, 0x00004, PANEL_HIDE_FOCUS)
ALIA_DEFINE_FLAG(panel, 0x00010, PANEL_SELECTED)
ALIA_DEFINE_FLAG(panel, 0x00020, PANEL_NO_INTERNAL_PADDING)
ALIA_DEFINE_FLAG(panel, 0x00040, PANEL_NO_CLICK_DETECTION)
ALIA_DEFINE_FLAG(panel, 0x00080, PANEL_IGNORE_STYLE_PADDING)
ALIA_DEFINE_FLAG(panel, 0x00100, PANEL_NO_REGION)
ALIA_DEFINE_FLAG(panel, 0x00200, PANEL_UNSAFE_CLICK_DETECTION)
// scrolling only
ALIA_DEFINE_FLAG(panel, 0x01000, PANEL_NO_HORIZONTAL_SCROLLING)
ALIA_DEFINE_FLAG(panel, 0x02000, PANEL_NO_VERTICAL_SCROLLING)
ALIA_DEFINE_FLAG(panel, 0x04000, PANEL_RESERVE_HORIZONTAL_SCROLLBAR)
ALIA_DEFINE_FLAG(panel, 0x08000, PANEL_RESERVE_VERTICAL_SCROLLBAR)
// clickable only
ALIA_DEFINE_FLAG(panel, 0x10000, PANEL_DISABLED)

struct panel_data;

struct panel : noncopyable
{
 public:
    panel()
    {
    }
    panel(
        ui_context ctx,
        readable<panel_style_info> style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        interaction_status state = NO_FLAGS)
    {
        begin(ctx, style, layout_spec, flags, id, state);
    }
    ~panel()
    {
        end();
    }
    void
    begin(
        ui_context ctx,
        readable<panel_style_info> style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        interaction_status state = NO_FLAGS);
    void
    end();
    // // inner_region() is the region inside the panel's border
    // layout_box
    // inner_region() const
    // {
    //     return inner_.padded_region();
    // }
    // // outer_region() includes the border
    // layout_box
    // outer_region() const;
    // // padded_region() includes the padding
    // layout_box
    // padded_region() const;

 private:
    optional_context<ui_context> ctx_;
    // panel_data* data_;
    bordered_layout outer_;
    // scoped_substyle substyle_;
    column_layout inner_;
    panel_flag_set flags_;
};

#if 0

class clickable_panel : noncopyable
{
 public:
    clickable_panel()
    {
    }
    clickable_panel(
        ui_context& ctx,
        accessor<string> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        widget_id id = auto_id)
    {
        begin(ctx, style, layout_spec, flags, id);
    }
    void
    begin(
        ui_context& ctx,
        accessor<string> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        widget_id id = auto_id);
    void
    end()
    {
        panel_.end();
    }
    layout_box
    inner_region() const
    {
        return panel_.inner_region();
    }
    layout_box
    outer_region() const
    {
        return panel_.outer_region();
    }
    layout_box
    padded_region() const
    {
        return panel_.padded_region();
    }
    bool
    clicked() const
    {
        return clicked_;
    }

 private:
    panel panel_;
    bool clicked_;
};

struct scrolling_data;

struct scrollable_region : noncopyable
{
    scrollable_region() : ctx_(0)
    {
    }
    scrollable_region(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        unsigned scrollable_axes = 1 | 2,
        widget_id id = auto_id,
        optional_storage<layout_vector> const& scroll_position_storage = none,
        unsigned reserved_axes = 0)
    {
        begin(
            ctx,
            layout_spec,
            scrollable_axes,
            id,
            scroll_position_storage,
            reserved_axes);
    }
    ~scrollable_region()
    {
        end();
    }

    void
    begin(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        unsigned scrollable_axes = 1 | 2,
        widget_id id = auto_id,
        optional_storage<layout_vector> const& scroll_position_storage = none,
        unsigned reserved_axes = 0);
    void
    end();

 private:
    ui_context* ctx_;
    scrolling_data* data_;
    widget_id id_;
    scoped_clip_region scr_;
    scoped_transformation transform_;
    scoped_layout_container slc_;
    scoped_routing_region srr_;
};

struct scrollable_panel : noncopyable
{
 public:
    scrollable_panel()
    {
    }
    scrollable_panel(
        ui_context& ctx,
        accessor<string> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        optional_storage<layout_vector> const& scroll_position_storage = none)
    {
        begin(ctx, style, layout_spec, flags, scroll_position_storage);
    }
    ~scrollable_panel()
    {
        end();
    }
    void
    begin(
        ui_context& ctx,
        accessor<string> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        optional_storage<layout_vector> const& scroll_position_storage = none);
    void
    end();

 private:
    bordered_layout outer_;
    scoped_substyle substyle_;
    scrollable_region region_;
    bordered_layout padding_border_;
    linear_layout inner_;
};

#endif

} // namespace alia

#endif
