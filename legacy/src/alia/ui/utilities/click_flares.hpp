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

struct click_flare_bit_layout
{
    flare_bit_field click_flare;
    flare_bit_field press_and_hold_flare;
};

void
fire_click_flare(
    dataless_ui_context ctx,
    bitpack_ref<click_flare_bit_layout> bits,
    mouse_button button = mouse_button::LEFT);

void
render_click_flares(
    dataless_ui_context ctx,
    bitpack_ref<click_flare_bit_layout> bits,
    interaction_status state,
    layout_vector position,
    rgb8 color,
    float radius = 32);

} // namespace alia

#endif
