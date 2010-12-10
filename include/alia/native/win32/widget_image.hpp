#ifndef ALIA_NATIVE_WIN32_WIDGET_IMAGE_HPP
#define ALIA_NATIVE_WIN32_WIDGET_IMAGE_HPP

#include <alia/surface.hpp>
#include <alia/native/win32/bitmap_dc.hpp>
#include <boost/shared_ptr.hpp>
#include <cassert>

namespace alia { namespace native {

template<class Image>
struct static_widget_image_data
{
    Image image;
    cached_image_ptr cached_image;
};

template<class Image>
void draw_widget(surface& surface, point2i const& position,
    static_widget_image_data<Image>& data)
{
    if (!is_valid(data.cached_image))
    {
        surface.cache_image(data.cached_image,
            make_interface(data.image.view, 0));
    }
    data.cached_image->draw(point2d(position));
}

template<class Image>
void invalidate(static_widget_image_data<Image>& data)
{
    data.cached_image.reset();
}

template<typename State, class Image>
struct stateful_widget_image_data
{
    stateful_widget_image_data() : version(0) {}
    Image image;
    unsigned version;
    cached_image_ptr cached_image;
    State state;
};

template<class Creator, class Image, class State>
void draw_stateful_widget(stateful_widget_image_data<State,Image>& data,
    surface& surface, point2i const& position, State const& state,
    Creator const& creator)
{
    if (!data.version || data.state != state || !is_valid(data.cached_image))
    {
        creator(&data.image, state);
        data.state = state;
        ++data.version;
        surface.cache_image(data.cached_image,
            make_interface(data.image.view, data.version));
    }
    data.cached_image->draw(point2d(position));
}

template<typename State, class Image>
void invalidate(stateful_widget_image_data<State,Image>& data)
{ data.version = 0; }

template<class Painter>
void capture_image(image<rgba8>* image, vector2i const& size,
    Painter const& painter)
{
    assert(size[0] != 0 && size[1] != 0);

    bitmap_dc white_dc, black_dc;

    white_dc.create(size, 255);
    black_dc.create(size, 0);

    painter(white_dc.get_dc());
    painter(black_dc.get_dc());

    create_image(*image, size);

    uint8 const* wp = white_dc.get_pixels();
    uint8 const* bp = black_dc.get_pixels();
    alia_foreach_pixel(image->view, rgba8, i,
        uint8 alpha = 255 - (*wp - *bp);
        if (alpha == 0 || alpha == 255)
        {
            i.b = *bp++;
            i.g = *bp++;
            i.r = *bp++;
        }
        else
        {
            i.b = uint8(int(*bp++) * 255 / alpha);
            i.g = uint8(int(*bp++) * 255 / alpha);
            i.r = uint8(int(*bp++) * 255 / alpha);
        }
        i.a = alpha;
        ++bp;
        wp += 4;
    )
};

template<class Painter>
void capture_image(image<rgb8>* image, vector2i const& size,
    Painter const& painter)
{
    assert(size[0] != 0 && size[1] != 0);
    bitmap_dc dc;
    dc.create(size);
    painter(dc.get_dc());
    create_image(*image, size);
    uint8 const* p = dc.get_pixels();
    alia_foreach_pixel(image->view, rgb8, i,
        i.b = *p++;
        i.g = *p++;
        i.r = *p++;
        ++p;
    )
};

}}

#endif
