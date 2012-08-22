#ifndef ALIA_IMAGE_HPP
#define ALIA_IMAGE_HPP

#include <alia/rendering.hpp>

namespace alia {

template<class Pixel>
struct image_view
{
    Pixel* pixels;
    vector<2,int> size;
    int stride;
};

template<class Pixel>
image_view<Pixel> subimage(image_view<Pixel> const& src,
    box<2,int> const& region)
{
    image_view<Pixel> r;
    r.pixels = src.pixels + region.corner[1] * src.stride + region.corner[0];
    r.size = region.size;
    r.stride = src.stride;
    return r;
}

template<class Pixel>
void swap(image_view<Pixel>& a, image_view<Pixel>& b)
{
    image_view<Pixel> t = a;
    a = b;
    b = t;
}

#define alia_foreach_pixel(view, Pixel, iter, body) \
    { \
        image_view<Pixel> alia__v = (view); \
        Pixel* alia__row_start = alia__v.pixels; \
        for (int alia__i = 0; alia__i < alia__v.size[1]; ++alia__i) \
        { \
            Pixel* alia__p = alia__row_start; \
            alia__row_start += alia__v.stride; \
            Pixel* alia__row_end = alia__p + alia__v.size[0]; \
            for (; alia__p != alia__row_end; ++alia__p) \
            { \
                Pixel& iter = *alia__p; \
                body; \
            } \
        } \
    }

#define alia_foreach_pixel2(view1, Pixel1, iter1, view2, Pixel2, iter2, body) \
    { \
        image_view<Pixel1> alia__v1 = (view1); \
        image_view<Pixel2> alia__v2 = (view2); \
        Pixel1* alia__row_start1 = alia__v1.pixels; \
        Pixel2* alia__row_start2 = alia__v2.pixels; \
        assert(alia__v1.size == alia__v2.size); \
        for (int alia__i = 0; alia__i < alia__v1.size[1]; ++alia__i) \
        { \
            Pixel1* alia__p1 = alia__row_start1; \
            alia__row_start1 += alia__v1.stride; \
            Pixel2* alia__p2 = alia__row_start2; \
            alia__row_start2 += alia__v2.stride; \
            Pixel1* alia__row_end1 = alia__p1 + alia__v1.size[0]; \
            for (; alia__p1 != alia__row_end1; ++alia__p1, ++alia__p2) \
            { \
                Pixel1& iter1 = *alia__p1; \
                Pixel2& iter2 = *alia__p2; \
                body; \
            } \
        } \
    }

#define alia_foreach_pixel3(view1, Pixel1, iter1, view2, Pixel2, iter2, \
    view3, Pixel3, iter3, body) \
    { \
        image_view<Pixel1> alia__v1 = (view1); \
        image_view<Pixel2> alia__v2 = (view2); \
        image_view<Pixel3> alia__v3 = (view3); \
        Pixel1* alia__row_start1 = alia__v1.pixels; \
        Pixel2* alia__row_start2 = alia__v2.pixels; \
        Pixel3* alia__row_start3 = alia__v3.pixels; \
        assert(alia__v1.size == alia__v2.size); \
        assert(alia__v1.size == alia__v3.size); \
        for (int alia__i = 0; alia__i < alia__v1.size[1]; ++alia__i) \
        { \
            Pixel1* alia__p1 = alia__row_start1; \
            alia__row_start1 += alia__v1.stride; \
            Pixel2* alia__p2 = alia__row_start2; \
            alia__row_start2 += alia__v2.stride; \
            Pixel3* alia__p3 = alia__row_start3; \
            alia__row_start3 += alia__v3.stride; \
            Pixel1* alia__row_end1 = alia__p1 + alia__v1.size[0]; \
            for (; alia__p1 != alia__row_end1; ++alia__p1, ++alia__p2, \
                ++alia__p3) \
            { \
                Pixel1& iter1 = *alia__p1; \
                Pixel2& iter2 = *alia__p2; \
                Pixel3& iter3 = *alia__p3; \
                body; \
            } \
        } \
    }

template<class Pixel1, class Pixel2>
void copy_image(image_view<Pixel1> const& dst, image_view<Pixel2> const& src)
{
    alia_foreach_pixel2(dst, Pixel1, i, src, Pixel2, j, i = j)
}

template<class Pixel>
struct image : noncopyable
{
    image() { view.pixels = 0; }
    image(vector<2,int> const& size)
    {
        int n_pixels = size[0] * size[1];
        view.pixels = n_pixels != 0 ? new Pixel[n_pixels] : 0;
        view.size = size;
        view.stride = size[0];
    }
    ~image() { delete[] view.pixels; }
    image_view<Pixel> view;
};

template<class Pixel>
void swap(image<Pixel>& a, image<Pixel>& b)
{
    swap(a.view, b.view);
}

template<class Pixel>
void create_image(image<Pixel>& img, vector<2,int> const& size)
{
    image<Pixel> tmp(size);
    swap(img, tmp);
}

template<class Pixel>
struct pixel_type_format {};
template<>
struct pixel_type_format<rgb8>
{ static pixel_format const value = RGB; };
template<>
struct pixel_type_format<rgba8>
{ static pixel_format const value = RGBA; };
template<>
struct pixel_type_format<uint8_t>
{ static pixel_format const value = GRAY; };
template<class Pixel>
struct pixel_type_format<Pixel const>
 : pixel_type_format<Pixel> {};

template<class Pixel>
image_interface make_interface(image_view<Pixel> const& view)
{
    image_interface i;
    i.pixels = view.pixels;
    i.size = vector<2,unsigned>(view.size);
    i.format = pixel_type_format<Pixel>::value;
    i.stride = unsigned(view.stride);
    return i;
}

static inline image_interface make_alpha_interface(
    image_view<uint8_t> const& view)
{
    image_interface i;
    i.pixels = view.pixels;
    i.size = vector<2,unsigned>(view.size);
    i.format = ALPHA;
    i.stride = unsigned(view.stride);
    return i;
}

template<class Pixel>
Pixel* get_row_begin(image_view<Pixel> const& view, int y)
{
    return view.pixels + y * view.stride;
}
template<class Pixel>
Pixel* get_row_end(image_view<Pixel> const& view, int y)
{
    return view.pixels + y * view.stride + view.size[0];
}

}

#endif
