#include <alia/scoped_state.hpp>
#include <alia/context.hpp>
#include <alia/surface.hpp>
#include <alia/transformations.hpp>

namespace alia {

void scoped_transformation::begin(context& ctx)
{
    ctx_ = &ctx;
    old_matrix_ = ctx.pass_state.transformation;
    active_ = true;
}
void scoped_transformation::set(matrix<3,3,double> const& transformation)
{
    set_transformation(*ctx_, old_matrix_ * transformation);
}
void scoped_transformation::restore()
{
    set_transformation(*ctx_, old_matrix_);
}
void scoped_transformation::end()
{
    if (active_)
    {
        restore();
        active_ = false;
    }
}

void scoped_surface_transformation::begin(surface& surface)
{
    surface_ = &surface;
    old_matrix_ = surface.get_transformation_matrix();
    active_ = true;
}
void scoped_surface_transformation::set(
    matrix<3,3,double> const& transformation)
{
    surface_->set_transformation_matrix(old_matrix_ * transformation);
}
void scoped_surface_transformation::restore()
{
    surface_->set_transformation_matrix(old_matrix_);
}
void scoped_surface_transformation::end()
{
    if (active_)
    {
        restore();
        active_ = false;
    }
}

void scoped_font::begin(context& ctx)
{
    ctx_ = &ctx;
    old_font_ = ctx.pass_state.active_font;
    active_ = true;
}
void scoped_font::set(font const& font)
{
    ctx_->pass_state.active_font = font;
}
void scoped_font::set(standard_font font)
{
    ctx_->pass_state.active_font = ctx_->artist->translate_standard_font(font);
}
void scoped_font::restore()
{
    ctx_->pass_state.active_font = old_font_;
}
void scoped_font::end()
{
    if (active_)
    {
        ctx_->pass_state.active_font = old_font_;
        active_ = false;
    }
}

void scoped_text_color::begin(context& ctx)
{
    ctx_ = &ctx;
    old_color_ = ctx.pass_state.text_color;
    active_ = true;
}
void scoped_text_color::set(rgba8 const& color)
{
    ctx_->pass_state.text_color = color;
}
void scoped_text_color::restore()
{
    ctx_->pass_state.text_color = old_color_;
}
void scoped_text_color::end()
{
    if (active_)
    {
        ctx_->pass_state.text_color = old_color_;
        active_ = false;
    }
}

void scoped_clip_region::begin(context& ctx)
{
    ctx_ = &ctx;
    old_region_ = ctx.pass_state.clip_region;
    active_ = true;
}
void scoped_clip_region::set(box2i const& region)
{
    // The clip region has to be specified in surface coordinates, but it's
    // more convenient for this to accept a region in the current frame of
    // reference, so we have to transform it here.
    box2i transformed_region;
    point2d p = transform_point(ctx_->pass_state.transformation,
        point2d(region.corner));
    vector2d s = transform_vector(ctx_->pass_state.transformation,
        vector2d(region.size));
    for (int i = 0; i < 2; ++i)
    {
        transformed_region.corner[i] = int(p[i] + 0.5);
        transformed_region.size[i] = int(s[i] + 0.5);
    }
    box2i intersection;
    if (!compute_intersection(&intersection, old_region_, transformed_region))
        intersection = box2i(point2i(0, 0), vector2i(0, 0));
    set_clip_region(*ctx_, intersection);
}
void scoped_clip_region::restore()
{
    set_clip_region(*ctx_, old_region_);
}
void scoped_clip_region::end()
{
    if (active_)
    {
        restore();
        active_ = false;
    }
}

void scoped_style::begin(context& ctx)
{
    ctx_ = &ctx;
    old_style_code_ = ctx.pass_state.style_code;
    old_padding_size_ = ctx.pass_state.padding_size;
    old_active_font_ = ctx.pass_state.active_font;
    old_text_color_ = ctx.pass_state.text_color;
    old_bg_color_ = ctx.pass_state.bg_color;
    old_selected_text_color_ = ctx.pass_state.selected_text_color;
    old_selected_bg_color_ = ctx.pass_state.selected_bg_color;
    active_ = true;
}
void scoped_style::set(unsigned code)
{
    ctx_->pass_state.style_code = code;
    ctx_->artist->activate_style(code);
}
void scoped_style::set(style s)
{
    set(ctx_->artist->get_code_for_style(s, 0));
}
void scoped_style::restore()
{
    ctx_->pass_state.padding_size = old_padding_size_;
    ctx_->pass_state.active_font = old_active_font_;
    ctx_->pass_state.text_color = old_text_color_;
    ctx_->pass_state.bg_color = old_bg_color_;
    ctx_->pass_state.selected_text_color = old_selected_text_color_;
    ctx_->pass_state.selected_bg_color = old_selected_bg_color_;
    ctx_->pass_state.style_code = old_style_code_;
    ctx_->artist->restore_style(old_style_code_);
}
void scoped_style::end()
{
    if (active_)
    {
        restore();
        active_ = false;
    }
}

}
