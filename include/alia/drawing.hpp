#ifndef ALIA_DRAWING_HPP
#define ALIA_DRAWING_HPP

#include <alia/surface.hpp>

namespace alia {

// These require data caching. You can either pass in a pointer to the cached
// data directly as the final parameter, or leave it as 0 and the functions
// will request data from the context.

// Note that the draw_image functions take a version parameter. Image contents
// are too large to check every frame for changes, so the version parameter
// indicates whether or not the contents have changed.

template<class Version>
struct draw_image_data
{
    cached_image_ptr cached_image;
    Version version;
};

template<class Version>
void draw_image(
    context& ctx,
    point2d const& position,
    image_interface const& img,
    Version const& version,
    rgba8 const& color = rgba8(0xff, 0xff, 0xff, 0xff),
    unsigned flags = 0,
    draw_image_data<Version>* data = 0)
{
    if (!data)
        data = get_data<draw_image_data<Version> >(ctx);
    if (ctx.event->type == RENDER_EVENT)
    {
        if (!is_valid(data->cached_image) || data->version != version)
        {
            ctx.surface->cache_image(data->cached_image, img, flags);
            data->version = version;
        }
        data->cached_image->draw(position, color);
    }
}

template<class Version>
void draw_image_region(
    context& ctx,
    point2d const& position,
    image_interface const& img,
    Version const& version,
    box2d const& region,
    rgba8 const& color = rgba8(0xff, 0xff, 0xff, 0xff),
    unsigned flags = 0,
    draw_image_data<Version>* data = 0)
{
    if (!data)
        data = get_data<draw_image_data<Version> >(ctx);
    if (ctx.event->type == RENDER_EVENT)
    {
        if (!is_valid(data->cached_image) || data->version != version)
        {
            ctx.surface->cache_image(data->cached_image, img, flags);
            data->version = version;
        }
        data->cached_image->draw_region(position, region, color);
    }
}

typedef cached_text_ptr draw_text_data;
void draw_text(
    context& ctx,
    point2d const& position,
    std::string const& text,
    font const& font,
    rgba8 const& color,
    draw_text_data* data = 0);

// These don't cache any data...

void clear_surface(
    context& ctx,
    rgb8 const& color);

template<class T>
void draw_poly(
    context& ctx,
    rgba8 const& color,
    line_style const& style,
    point<2,T>* vertices,
    unsigned n_vertices)
{
    if (ctx.event->type == RENDER_EVENT)
        ctx.surface->draw_line_loop(color, style, vertices, n_vertices);
}

template<class T>
void draw_filled_poly(
    context& ctx,
    rgba8 const& color,
    point<2,T>* vertices,
    unsigned n_vertices)
{
    if (ctx.event->type == RENDER_EVENT)
        ctx.surface->draw_filled_polygon(color, vertices, n_vertices);
}

template<class T>
void draw_box(
    context& ctx,
    rgba8 const& color,
    line_style const& style,
    box<2,T> const& box)
{
    if (ctx.event->type == RENDER_EVENT)
    {
        point<2,T> poly[4];
        make_polygon(poly, box);
        draw_poly(ctx, color, style, poly, 4);
    }
}

template<class T>
void draw_filled_box(
    context& ctx,
    rgba8 const& color,
    box<2,T> const& box)
{
    if (ctx.event->type == RENDER_EVENT)
    {
        point<2,T> poly[4];
        make_polygon(poly, box);
        draw_filled_poly(ctx, color, poly, 4);
    }
}

}

#endif
