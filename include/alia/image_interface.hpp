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
    unsigned version;
    pixel_format format;
    vector2i size, step;
};

static inline bool operator==(image_interface const& a,
    image_interface const& b)
{
    return a.pixels == b.pixels && a.version == b.version &&
        a.format == b.format && a.size == b.size && a.step == b.step;
}

static inline bool operator!=(image_interface const& a,
    image_interface const& b)
{
    return !(a == b);
}

}

#endif
