#ifndef SCOPED_STATE_HPP
#define SCOPED_STATE_HPP

#include <alia/artist.hpp>
#include <boost/noncopyable.hpp>

namespace alia {

class scoped_transformation : boost::noncopyable
{
 public:
    scoped_transformation() : active_(false) {}
    scoped_transformation(context& ctx)
    { begin(ctx); }
    scoped_transformation(context& ctx,
        matrix<3,3,double> const& transformation)
    { begin(ctx); set(transformation); }
    ~scoped_transformation() { end(); }
    void begin(context& ctx);
    void set(matrix<3,3,double> const& transformation);
    void restore();
    void end();
 private:
    context* ctx_;
    matrix<3,3,double> old_matrix_;
    bool active_;
};

class scoped_surface_transformation : boost::noncopyable
{
 public:
    scoped_surface_transformation() : active_(false) {}
    scoped_surface_transformation(surface& surface)
    { begin(surface); }
    scoped_surface_transformation(surface& surface,
        matrix<3,3,double> const& transformation)
    { begin(surface); set(transformation); }
    ~scoped_surface_transformation() { end(); }
    void begin(surface& surface);
    void set(matrix<3,3,double> const& transformation);
    void restore();
    void end();
 private:
    surface* surface_;
    matrix<3,3,double> old_matrix_;
    bool active_;
};

class scoped_font : boost::noncopyable
{
 public:
    scoped_font() : active_(false) {}
    scoped_font(context& ctx)
    { begin(ctx); }
    template<class T>
    scoped_font(context& ctx, T const& arg)
    { begin(ctx); set(arg); }
    ~scoped_font() { end(); }
    void begin(context& ctx);
    void set(font const& font);
    void set(standard_font font);
    void restore();
    void end();
 private:
    context* ctx_;
    font old_font_;
    bool active_;
};

class scoped_text_color : boost::noncopyable
{
 public:
    scoped_text_color() : active_(false) {}
    scoped_text_color(context& ctx)
    { begin(ctx); }
    scoped_text_color(context& ctx, rgba8 const& color)
    { begin(ctx); set(color); }
    ~scoped_text_color() { end(); }
    void begin(context& ctx);
    void set(rgba8 const& color);
    void restore();
    void end();
 private:
    context* ctx_;
    rgba8 old_color_;
    bool active_;
};

class scoped_clip_region : boost::noncopyable
{
 public:
    scoped_clip_region() : active_(false) {}
    scoped_clip_region(context& ctx)
    { begin(ctx); }
    scoped_clip_region(context& ctx, box2i const& region)
    { begin(ctx); set(region); }
    ~scoped_clip_region() { end(); }
    void begin(context& ctx);
    void set(box2i const& region);
    void restore();
    void end();
 private:
    context* ctx_;
    box2i old_region_;
    bool active_;
};

class scoped_style : boost::noncopyable
{
 public:
    scoped_style() : active_(false) {}
    scoped_style(context& ctx)
    { begin(ctx); }
    template<class T>
    scoped_style(context& ctx, T arg)
    { begin(ctx); set(arg); }
    ~scoped_style() { end(); }
    void begin(context& ctx);
    void set(unsigned code);
    void set(style s);
    void restore();
    void end();
 private:
    context* ctx_;
    unsigned old_style_code_;
    vector2i old_padding_size_;
    font old_active_font_;
    rgba8 old_text_color_, old_bg_color_, old_selected_text_color_,
        old_selected_bg_color_;
    bool active_;
};

}

#endif
