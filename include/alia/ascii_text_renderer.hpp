#ifndef ALIA_ASCII_TEXT_RENDERER_HPP
#define ALIA_ASCII_TEXT_RENDERER_HPP

#include <alia/surface.hpp>
#include <alia/image.hpp>
#include <alia/typedefs.hpp>

namespace alia {

class ascii_text_renderer : boost::noncopyable
{
 public:
    void initialize(surface& surface, font const& font,
        image_view<uint8> const* image, int const* widths,
        font_metrics const& metrics);
    font get_font() const { return font_; }
    int get_char_width(char c) const;
    int get_string_width(std::string const& s) const;
    font_metrics const& get_metrics() const { return metrics_; }
    image_view<uint8> const& get_image() const { return *image_; }
    void get_character_image(image_view<uint8>* r, char c) const;
    void write(point2d const& position, rgba8 const& color,
        std::string const& text) const;
 private:
    surface* surface_;
    font font_;
    image_view<uint8> const* image_;
    int const* widths_;
    mutable cached_image_ptr cached_image_;
    vector2i cell_size_;
    font_metrics metrics_;
};

}

#endif
