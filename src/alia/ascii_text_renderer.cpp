#include <alia/ascii_text_renderer.hpp>

namespace alia {

void ascii_text_renderer::initialize(
    surface& surface, font const& font, image_view<uint8> const* image,
    int const* widths, int const* trailing_space, font_metrics const& metrics)
{
    surface_ = &surface;
    font_ = font;
    image_ = image;
    widths_ = widths;
    trailing_space_ = trailing_space;
    cell_size_ = image->size / 16;
    metrics_ = metrics;
}

void ascii_text_renderer::write(
    point2d const& location,
    rgba8 const& color,
    std::string const& text) const
{
    point2d p = location;
    p[0] -= metrics_.overhang;
    surface_->cache_image(cached_image_, make_interface(*image_, 0),
        surface::ALPHA_IMAGE);
    for (unsigned i = 0; i < text.length(); i++)
    {
        uint8 c = uint8(text[i]);
        int character_width = widths_[c];
        assert(character_width + metrics_.overhang * 2 <= cell_size_[0]);
        box2d r(
            point2d((c % 16) * cell_size_[0], (c / 16) * cell_size_[1]),
            vector2d(character_width + metrics_.overhang * 2, cell_size_[1]));
        cached_image_->draw_region(p, r, color);
        p[0] += character_width;
    }
}

int ascii_text_renderer::get_char_width(char c) const
{
    return widths_[uint8(c)];
}

int ascii_text_renderer::get_trailing_space(char c) const
{
    return trailing_space_[uint8(c)];
}

int ascii_text_renderer::get_string_width(std::string const& text) const
{
    int width = 0;
    for (unsigned i = 0; i < text.length(); ++i)
        width += widths_[uint8(text[i])];
    if (text.length() > 0)
        width -= trailing_space_[uint8(text[text.length() - 1])];
    return width;
}

void ascii_text_renderer::get_character_image(
    image_view<uint8>* r, char c) const
{
    uint8 index = uint8(c);
    assert(widths_[index] + metrics_.overhang * 2 <= cell_size_[0]);
    *r = subimage(*image_, box2i(
        point2i((index % 16) * cell_size_[0], (index / 16) * cell_size_[1]),
        vector2i(widths_[index] + metrics_.overhang * 2, cell_size_[1])));
}

}
