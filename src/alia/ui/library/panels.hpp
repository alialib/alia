#ifndef ALIA_UI_LIBRARY_PANELS_HPP
#define ALIA_UI_LIBRARY_PANELS_HPP

#include <alia/ui/utilities.hpp>

// This file declares utilities for implementing panels.

namespace alia {

struct panel_style_info
{
    absolute_size size;
    box_border_width<float> margin, border_width, padding;
    bool is_rounded;
    rgba8 border_color, background_color, gradient_color;
    box_corner_sizes border_radii;
};

panel_style_info
get_panel_style_info(dataless_ui_context& ctx, style_search_path const* path);

void refresh_panel_style_info(
    dataless_ui_context& ctx, keyed_data<panel_style_info>& stored_info,
    accessor<string> const& substyle, widget_state state,
    add_substyle_flag_set flags = NO_FLAGS);

struct custom_panel_data
{
    caching_renderer_data rendering, focus_rendering;
};

}

#endif
