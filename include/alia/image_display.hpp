#ifndef ALIA_IMAGE_DISPLAY_HPP
#define ALIA_IMAGE_DISPLAY_HPP

#include <alia/layout_interface.hpp>
#include <alia/surface.hpp>

namespace alia {

void do_image(
    context& ctx,
    image_interface const& img,
    layout const& layout_spec = default_layout,
    unsigned flags = 0);

}

#endif
