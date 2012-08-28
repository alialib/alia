#ifndef ALIA_ASCII_TEXT_RENDERER_HPP
#define ALIA_ASCII_TEXT_RENDERER_HPP

#include <alia/text_renderer.hpp>

namespace alia {

struct ascii_font_image
{
    // parameters that created this image
    alia::font font;
    rgba8 text_color, bg_color;

    // image of ASCII characters (arranged in a 16x16 grid of cells)
    alia::image<rgba8> image;

    // the size of the character cells
    vector<2,int> cell_size;
    
    font_metrics metrics;

    // advance[c] is the amount of space to advance the X coordinate after
    // rendering character c.
    int advance[256];

    // trailing_space[c] is the amount of trailing whitespace that's part of
    // c's advance value.
    int trailing_space[256];
};

struct ascii_text_renderer : text_renderer
{
    ascii_text_renderer(ascii_font_image const& image)
      : image_(&image)
    {}

    // implementation of text_renderer interface
    int compute_advance(utf8_string const& text) const;
    int measure_text(utf8_string const& text) const;
    utf8_ptr break_text(utf8_string const& text, int width,
        bool is_full_line) const;
    void render_text(
        image_view<rgba8> const& img, utf8_string const& text,
        rgba8 text_color, rgba8 background_color);

 private:
    ascii_font_image const* image_;
};

void draw_ascii_text(
    surface& surface, ascii_font_image const* font_image,
    cached_image_ptr& display_image, vector<2,double> const& position,
    char const* text, rgba8 const& color);

}

#endif
