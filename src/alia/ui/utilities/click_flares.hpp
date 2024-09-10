#ifndef ALIA_UI_UTILITIES_CLICK_FLARES_HPP
#define ALIA_UI_UTILITIES_CLICK_FLARES_HPP

#include <alia/ui/color.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/utilities/animation.hpp>
#include <alia/ui/utilities/widgets.hpp>

namespace alia {

constexpr millisecond_count click_flare_duration = 700;
constexpr millisecond_count click_accumulation_time = 400;

constexpr unsigned click_flare_bit_count = bits_required_for_flare * 2;

void
fire_click_flare(
    dataless_ui_context ctx,
    mouse_button button,
    unsigned& bits,
    unsigned base_index);

void
render_click_flares(
    dataless_ui_context ctx,
    unsigned& bits,
    unsigned base_index,
    widget_state state,
    layout_vector position,
    rgb8 color,
    float radius = 32);

} // namespace alia

#endif
