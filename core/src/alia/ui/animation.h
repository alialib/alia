#ifndef ALIA_UI_UTILITIES_CLICK_FLARES_HPP
#define ALIA_UI_UTILITIES_CLICK_FLARES_HPP

#include <alia/prelude.hpp>

#include <alia/abi/ui/input/elements.h>
#include <alia/impl/kernel/animation.hpp>

namespace alia {

constexpr alia_nanosecond_count click_flare_duration = milliseconds(700);
constexpr alia_nanosecond_count click_accumulation_time = milliseconds(400);

struct click_flare_bit_layout
{
    impl::flare_bitfield click_flare;
    impl::flare_bitfield press_and_hold_flare;
};

void
fire_click_flare(
    alia_context* ctx,
    bitpack_ref<click_flare_bit_layout> bits,
    alia_button_t button = ALIA_BUTTON_LEFT);

void
render_click_flares(
    alia_context* ctx,
    bitpack_ref<click_flare_bit_layout> bits,
    alia_interaction_status_t state,
    alia_vec2f position,
    alia_rgb color,
    float radius = 32);

} // namespace alia

#endif
