#ifndef ALIA_UI_LIBRARY_SLIDER_HPP
#define ALIA_UI_LIBRARY_SLIDER_HPP

#include <alia/ui/common.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/layout/specification.hpp>

namespace alia {

ALIA_DEFINE_FLAG_TYPE(slider)
ALIA_DEFINE_FLAG(slider, 0x0, SLIDER_HORIZONTAL)
ALIA_DEFINE_FLAG(slider, 0x1, SLIDER_VERTICAL)

struct slider_data;

struct slider_style_info
{
    rgb8 highlight_color;
    rgb8 track_color;
    rgb8 thumb_color;
};

slider_style_info
extract_slider_style_info(dataless_ui_context ctx);

void
draw_track(
    dataless_ui_context ctx,
    unsigned axis,
    layout_vector const& track_position,
    layout_scalar track_length,
    slider_style_info const& style);

void
do_slider(
    ui_context ctx,
    duplex<double> value,
    double minimum,
    double maximum,
    double step,
    layout const& layout_spec = default_layout,
    slider_flag_set flags = NO_FLAGS);

slider_data&
get_slider_data(ui_context ctx);

void
do_slider(
    ui_context ctx,
    slider_data& data,
    duplex<double> value,
    double minimum,
    double maximum,
    double step,
    layout const& layout_spec = default_layout,
    slider_flag_set flags = NO_FLAGS);

layout_vector
get_track_position(dataless_ui_context ctx, slider_data& data, unsigned axis);

layout_scalar
get_track_length(dataless_ui_context ctx, slider_data& data, unsigned axis);

layout_vector
get_thumb_position(
    dataless_ui_context ctx,
    slider_data& data,
    unsigned axis,
    double minimum,
    double maximum,
    readable<double> value);

layout_box
get_thumb_region(
    dataless_ui_context ctx,
    slider_data& data,
    unsigned axis,
    double minimum,
    double maximum,
    readable<double> value);

void
draw_track(
    dataless_ui_context ctx,
    unsigned axis,
    layout_vector const& track_position,
    layout_scalar track_length,
    slider_style_info const& style);

} // namespace alia

#endif
