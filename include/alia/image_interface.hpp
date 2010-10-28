#ifndef ALIA_IMAGE_INTERFACE_HPP
#define ALIA_IMAGE_INTERFACE_HPP

#include <alia/vector.hpp>

namespace alia {

enum pixel_format
{
    GRAY,
    RGB,
    RGBA,
};

unsigned get_channel_count(pixel_format fmt);

struct image_interface
{
    void const* pixels;
    pixel_format format;
    vector2i size, step;
};

}

#endif
