#include <alia/opengl/surface.hpp>
#include <alia/opengl/gl.hpp>
#include <alia/opengl/simple_texture.hpp>
#include <alia/opengl/tiled_texture.hpp>

namespace alia { namespace opengl {

void surface::initialize_render_state()
{
    ctx_->do_pending_texture_deletions();

    glViewport(0, 0, size_[0], size_[1]);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, size_[0], size_[1], 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, size_[0], size_[1]);

    glEnable(GL_LINE_STIPPLE);

    glColor4ub(0xff, 0xff, 0xff, 0xff);
    color_ = rgba8(0xff, 0xff, 0xff, 0xff);
    glLineStipple(1, 0xffff);
    line_style_.stipple.factor = 1;
    line_style_.stipple.pattern = 0xffff;
    glLineWidth(1);
    line_style_.width = 1;

    reset_drawing_state();
}

void surface::apply_clip_region()
{
    box2i const& rectangle = get_clip_region();
    if (!(rectangle.size[0] >= 0 && rectangle.size[1] >= 0))
    assert(rectangle.size[0] >= 0 && rectangle.size[1] >= 0);
    glScissor(rectangle.corner[0],
        size_[1] - (rectangle.corner[1] + rectangle.size[1]),
        rectangle.size[0], rectangle.size[1]);
}

void surface::apply_transformation_matrix()
{
    matrix<3,3,double> const& m = get_transformation_matrix();
    double gl_matrix[16] = {
        m(0,0), m(1,0), 0, m(2,0),
        m(0,1), m(1,1), 0, m(2,1),
             0,      0, 0,      0,
        m(0,2), m(1,2), 0, m(2,2) };
    glLoadMatrixd(gl_matrix);
}

void surface::draw_line(rgba8 const& color, line_style const& style,
    point2d const& p1, point2d const& p2)
{
    glLineStipple(style.stipple.factor, style.stipple.pattern);
    glLineWidth(style.width);
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_LINES);
    glVertex2d(p1[0], p1[1]);
    glVertex2d(p2[0], p2[1]);
    glEnd();
}
void surface::draw_line(rgba8 const& color, line_style const& style,
    point2f const& p1, point2f const& p2)
{
    glLineStipple(style.stipple.factor, style.stipple.pattern);
    glLineWidth(style.width);
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_LINES);
    glVertex2f(p1[0], p1[1]);
    glVertex2f(p2[0], p2[1]);
    glEnd();
}
void surface::draw_line(rgba8 const& color, line_style const& style,
    point2i const& p1, point2i const& p2)
{
    glLineStipple(style.stipple.factor, style.stipple.pattern);
    glLineWidth(style.width);
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_LINES);
    glVertex2i(p1[0], p1[1]);
    glVertex2i(p2[0], p2[1]);
    glEnd();
}

void surface::draw_line_strip(rgba8 const& color, line_style const& style,
    point2d const* vertices, unsigned n_vertices)
{
    glLineStipple(style.stipple.factor, style.stipple.pattern);
    glLineWidth(style.width);
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_LINE_STRIP);
    point2d const* end = vertices + n_vertices;
    for (point2d const* i = vertices; i != end; ++i)
        glVertex2d((*i)[0], (*i)[1]);
    glEnd();
}
void surface::draw_line_strip(rgba8 const& color, line_style const& style,
    point2f const* vertices, unsigned n_vertices)
{
    glLineStipple(style.stipple.factor, style.stipple.pattern);
    glLineWidth(style.width);
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_LINE_STRIP);
    point2f const* end = vertices + n_vertices;
    for (point2f const* i = vertices; i != end; ++i)
        glVertex2f((*i)[0], (*i)[1]);
    glEnd();
}
void surface::draw_line_strip(rgba8 const& color, line_style const& style,
    point2i const* vertices, unsigned n_vertices)
{
    glLineStipple(style.stipple.factor, style.stipple.pattern);
    glLineWidth(style.width);
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_LINE_STRIP);
    point2i const* end = vertices + n_vertices;
    for (point2i const* i = vertices; i != end; ++i)
        glVertex2i((*i)[0], (*i)[1]);
    glEnd();
}

void surface::draw_line_loop(rgba8 const& color, line_style const& style,
    point2d const* vertices, unsigned n_vertices)
{
    glLineStipple(style.stipple.factor, style.stipple.pattern);
    glLineWidth(style.width);
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_LINE_LOOP);
    point2d const* end = vertices + n_vertices;
    for (point2d const* i = vertices; i != end; ++i)
        glVertex2d((*i)[0], (*i)[1]);
    glEnd();
}
void surface::draw_line_loop(rgba8 const& color, line_style const& style,
    point2f const* vertices, unsigned n_vertices)
{
    glLineStipple(style.stipple.factor, style.stipple.pattern);
    glLineWidth(style.width);
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_LINE_LOOP);
    point2f const* end = vertices + n_vertices;
    for (point2f const* i = vertices; i != end; ++i)
        glVertex2f((*i)[0], (*i)[1]);
    glEnd();
}
void surface::draw_line_loop(rgba8 const& color, line_style const& style,
    point2i const* vertices, unsigned n_vertices)
{
    glLineStipple(style.stipple.factor, style.stipple.pattern);
    glLineWidth(style.width);
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_LINE_LOOP);
    point2i const* end = vertices + n_vertices;
    for (point2i const* i = vertices; i != end; ++i)
        glVertex2i((*i)[0], (*i)[1]);
    glEnd();
}

void surface::draw_filled_polygon(rgba8 const& color,
    point2d const* vertices, unsigned n_vertices)
{
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_POLYGON);
    point2d const* end = vertices + n_vertices;
    for (point2d const* i = vertices; i != end; ++i)
        glVertex2d((*i)[0], (*i)[1]);
    glEnd();
}
void surface::draw_filled_polygon(rgba8 const& color,
    point2f const* vertices, unsigned n_vertices)
{
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_POLYGON);
    point2f const* end = vertices + n_vertices;
    for (point2f const* i = vertices; i != end; ++i)
        glVertex2f((*i)[0], (*i)[1]);
    glEnd();
}
void surface::draw_filled_polygon(rgba8 const& color,
    point2i const* vertices, unsigned n_vertices)
{
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_POLYGON);
    point2i const* end = vertices + n_vertices;
    for (point2i const* i = vertices; i != end; ++i)
        glVertex2i((*i)[0], (*i)[1]);
    glEnd();
}

void surface::cache_image(
    cached_image_ptr& data,
    image_interface const& img,
    unsigned flags)
{
    return ctx_->cache_image(data, img, flags);
}

}}
