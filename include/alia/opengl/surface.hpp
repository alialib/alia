#ifndef ALIA_OPENGL_SURFACE_HPP
#define ALIA_OPENGL_SURFACE_HPP

#include <alia/surface.hpp>

namespace alia { namespace opengl {

class context;

class surface : public alia::surface
{
 public:
    void set_opengl_context(context& ctx) { ctx_ = &ctx; }

    void initialize_render_state();

    void set_size(vector2i const& size) { size_ = size; }
    vector2i get_size() const { return size_; }

 private:
    void apply_clip_region();
    void apply_transformation_matrix();
    void apply_color();
    void apply_line_stipple();
    void apply_line_width();

    void draw_line(rgba8 const& color, line_style const& style,
        point2d const& p1, point2d const& p2);
    void draw_line(rgba8 const& color, line_style const& style,
        point2f const& p1, point2f const& p2);
    void draw_line(rgba8 const& color, line_style const& style,
        point2i const& p1, point2i const& p2);

    void draw_line_strip(rgba8 const& color, line_style const& style,
        point2d const* vertices, unsigned n_vertices);
    void draw_line_strip(rgba8 const& color, line_style const& style,
        point2f const* vertices, unsigned n_vertices);
    void draw_line_strip(rgba8 const& color, line_style const& style,
        point2i const* vertices, unsigned n_vertices);

    void draw_line_loop(rgba8 const& color, line_style const& style,
        point2d const* vertices, unsigned n_vertices);
    void draw_line_loop(rgba8 const& color, line_style const& style,
        point2f const* vertices, unsigned n_vertices);
    void draw_line_loop(rgba8 const& color, line_style const& style,
        point2i const* vertices, unsigned n_vertices);

    void draw_filled_polygon(rgba8 const& color,
        point2d const* vertices, unsigned n_vertices);
    void draw_filled_polygon(rgba8 const& color,
        point2f const* vertices, unsigned n_vertices);
    void draw_filled_polygon(rgba8 const& color,
        point2i const* vertices, unsigned n_vertices);

    void cache_image(
        cached_image_ptr& data,
        image_interface const& img,
        unsigned flags);

 private:
    context* ctx_;
    vector2i size_;
    // current OpenGL state
    rgba8 color_;
    line_style line_style_;
};

}}

#endif
