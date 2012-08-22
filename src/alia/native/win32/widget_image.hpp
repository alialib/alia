#ifndef ALIA_NATIVE_WIN32_WIDGET_IMAGE_HPP
#define ALIA_NATIVE_WIN32_WIDGET_IMAGE_HPP

#include <alia/rendering.hpp>
#include <alia/native/win32/bitmap_dc.hpp>
#include <cassert>

namespace alia { namespace native {

template<class Image>
struct static_widget_image_data
{
    Image image;
    cached_image_ptr cached_image;
};

template<class Image>
void draw_widget(surface& surface, vector<2,int> const& position,
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
    surface& surface, vector<2,int> const& position, State const& state,
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

namespace {

rgba8 calculate_rgba8_value(uint8_t const* wp, uint8_t const* bp)
{
    rgba8 r;
    uint8_t alpha = 255 - (*wp - *bp);
    r.b = *bp++;
    r.g = *bp++;
    r.r = *bp++;
    r.a = alpha;
    return r;
}

}

template<class Painter>
void capture_image(image<rgba8>* image, vector<2,int> const& size,
    Painter const& painter)
{
    assert(size[0] != 0 && size[1] != 0);

    bitmap_dc white_dc, black_dc;

    white_dc.create(size, 255);
    black_dc.create(size, 0);

    painter(white_dc.dc());
    painter(black_dc.dc());

    create_image(*image, size);

    uint8_t const* wp = white_dc.pixels();
    uint8_t const* bp = black_dc.pixels();
    alia_foreach_pixel(image->view, rgba8, i,
        i = calculate_rgba8_value(wp, bp);
        wp += 4; bp += 4;)
};

template<class Painter>
void capture_image(image<rgb8>* image, vector<2,int> const& size,
    Painter const& painter)
{
    assert(size[0] != 0 && size[1] != 0);
    bitmap_dc dc;
    dc.create(size);
    painter(dc.dc());
    create_image(*image, size);
    uint8_t const* p = dc.pixels();
    alia_foreach_pixel(image->view, rgb8, i,
        i.b = *p++;
        i.g = *p++;
        i.r = *p++;
        ++p;
    )
};

}}

#endif
