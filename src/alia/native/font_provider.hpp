#ifndef ALIA_NATIVE_FONT_PROVIDER_HPP
#define ALIA_NATIVE_FONT_PROVIDER_HPP

#include <alia/ascii_text_renderer.hpp>

namespace alia { namespace native {

void create_ascii_font_image(ascii_font_image* img,
    font const& font, rgba8 text_color, rgba8 bg_color);

}}

#endif
