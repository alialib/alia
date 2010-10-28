#include <alia/surface.hpp>

namespace alia {

line_stipple no_line(1, 0), solid_line(1, 0xffff), dashed_line(10, 0x5555),
    dotted_line(3, 0x5555);

void cached_text::initialize(
    std::string const& text,
    font const& font,
    vector2i const& size,
    font_metrics const& metrics)
{
    text_ = text;
    font_ = font;
    size_ = size;
    metrics_ = metrics;
}

void surface::reset_drawing_state()
{
    set_clip_region(box2i(point2i(0, 0), get_size()));
    set_transformation_matrix(identity_matrix<3,double>());
    set_mouse_cursor(DEFAULT_CURSOR);
}

void surface::draw_line_strip(rgba8 const& color, line_style const& style,
    point2d const* vertices, unsigned n_vertices)
{
    point2d const* end = vertices + (n_vertices - 1);
    for (point2d const* i = vertices; i != end; ++i)
        draw_line(color, style, *i, *(i + 1));
}
void surface::draw_line_strip(rgba8 const& color, line_style const& style,
    point2f const* vertices, unsigned n_vertices)
{
    point2f const* end = vertices + (n_vertices - 1);
    for (point2f const* i = vertices; i != end; ++i)
        draw_line(color, style, *i, *(i + 1));
}
void surface::draw_line_strip(rgba8 const& color, line_style const& style,
    point2i const* vertices, unsigned n_vertices)
{
    point2i const* end = vertices + (n_vertices - 1);
    for (point2i const* i = vertices; i != end; ++i)
        draw_line(color, style, *i, *(i + 1));
}

void surface::draw_line_loop(rgba8 const& color, line_style const& style,
    point2d const* vertices, unsigned n_vertices)
{
    point2d const* end = vertices + n_vertices;
    point2d const* last_p = end - 1;
    for (point2d const* i = vertices; i != end; ++i)
    {
        draw_line(color, style, *last_p, *i);
        last_p = i;
    }
}
void surface::draw_line_loop(rgba8 const& color, line_style const& style,
    point2f const* vertices, unsigned n_vertices)
{
    point2f const* end = vertices + n_vertices;
    point2f const* last_p = end - 1;
    for (point2f const* i = vertices; i != end; ++i)
    {
        draw_line(color, style, *last_p, *i);
        last_p = i;
    }
}
void surface::draw_line_loop(rgba8 const& color, line_style const& style,
    point2i const* vertices, unsigned n_vertices)
{
    point2i const* end = vertices + n_vertices;
    point2i const* last_p = end - 1;
    for (point2i const* i = vertices; i != end; ++i)
    {
        draw_line(color, style, *last_p, *i);
        last_p = i;
    }
}

}
