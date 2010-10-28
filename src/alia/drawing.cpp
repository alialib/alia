#include <alia/drawing.hpp>
#include <alia/data.hpp>
#include <alia/context.hpp>

namespace alia {

void draw_text(
    context& ctx,
    point2d const& position,
    std::string const& text,
    rgba8 const& color,
    font const& font,
    draw_text_data* data)
{
    if (!data) data = get_data<draw_text_data>(ctx);
    if (ctx.event->type == RENDER_EVENT)
    {
        surface& s = *ctx.surface;
        s.cache_text(*data, font, text.c_str());
        (*data)->draw(position, color);
    }
}

void clear_surface(
    context& ctx,
    rgb8 const& color)
{
    if (ctx.event->type == RENDER_EVENT)
    {
        point2d poly[4];
        make_polygon(poly, ctx.pass_state.untransformed_clip_region);
        ctx.surface->draw_filled_polygon(color, poly, 4);
    }
}

}
