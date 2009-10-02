#include <alia/font.hpp>
#include <alia/context.hpp>
#include <alia/surface.hpp>
#include <alia/artist.hpp>

namespace alia {

font_metrics const& get_font_metrics(context& ctx, font const& f)
{
    std::map<font,font_metrics>::iterator i =
        ctx.cached_font_metrics.find(f);
    if (i == ctx.cached_font_metrics.end())
    {
        i = ctx.cached_font_metrics.insert(std::map<font,font_metrics>
            ::value_type(f, font_metrics())).first;
        ctx.surface->get_font_metrics(&i->second, f);
    }
    return i->second;
}

void set_font_size_adjustment(context& ctx, float adjustment)
{
    ctx.font_size_adjustment = adjustment;
    ctx.artist->on_font_size_adjustment_change();
}

font shrink(font const& f, float amount)
{
    font r = f;
    r.set_size(r.get_size() - amount);
    return r;
}
font enlarge(font const& f, float amount)
{
    font r = f;
    r.set_size(r.get_size() + amount);
    return r;
}

font add_style(font const& f, unsigned style)
{
    font r = f;
    r.set_style(r.get_style() | style);
    return r;
}

}
