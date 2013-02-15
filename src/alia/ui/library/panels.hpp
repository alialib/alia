#ifndef ALIA_UI_LIBRARY_PANELS_HPP
#define ALIA_UI_LIBRARY_PANELS_HPP

#include <alia/ui/utilities.hpp>

// This file declares utilities for implementing panels.

namespace alia {

struct panel_style_info
{
    absolute_size size;
    resolved_box_border_width margin, border_width, padding;
    bool is_rounded;
    rgba8 border_color, background_color;
    box_corner_sizes border_radii;
};

void refresh_panel_style_info(
    ui_context& ctx, keyed_data<panel_style_info>& stored_info,
    getter<string> const& substyle, widget_state state,
    add_substyle_flag_set flags = NO_FLAGS);

struct custom_panel_data
{
    caching_renderer_data rendering, focus_rendering;
};

struct custom_panel : noncopyable
{
 public:
    custom_panel() : ctx_(0) {}
    custom_panel(
        ui_context& ctx, custom_panel_data& data,
        getter<panel_style_info> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        widget_state state = WIDGET_NORMAL)
    { begin(ctx, data, style, layout_spec, flags, id, state); }
    ~custom_panel() { end(); }
    void begin(
        ui_context& ctx, custom_panel_data& data,
        getter<panel_style_info> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        widget_state state = WIDGET_NORMAL);
    void end();
    // inner_region() is the region inside the panel's border
    layout_box inner_region() const { return inner_.padded_region(); }
    // outer_region() includes the border
    layout_box outer_region() const;
    // padded_region() includes the padding
    layout_box padded_region() const;
 private:
    ui_context* ctx_;
    panel_style_info* style_;
    bordered_layout outer_;
    linear_layout inner_;
    panel_flag_set flags_;
};

}

#endif
