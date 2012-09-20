#ifndef ALIA_TEXT_RENDERER_HPP
#define ALIA_TEXT_RENDERER_HPP

#include <alia/image.hpp>

namespace alia {

struct text_renderer
{
    alia::font font() const { return font_; }
    font_metrics const& metrics() const { return metrics_; }

    // Compute the total advance of the given text.
    virtual int compute_advance(utf8_string const& text) const = 0;

    // Compute the total width of the given text.
    // (This differs from compute_advance in that it would include space
    // occupied by overhanging parts of the character, but would not include
    // extra whitespace.)
    virtual int measure_text(utf8_string const& text) const = 0;

    // Compute the amount of text that will fit in the given width, breaking
    // at a natural word boundary.
    // The return value is a pointer to the first character that will NOT fit.
    // If is_full_line is true, the width is understood to represent the full
    // width of the line. In this case, if the first word on the line will not
    // fit within the width, it will be broken across lines.
    virtual utf8_ptr break_text(utf8_string const& text, int width,
        bool is_full_line) const = 0;

    // Render the given text within the given image (no breaking is done).
    // The image supplied must be large enough to hold the text, minus any
    // trailing whitespace.
    // The entire image is filled with the requested background color, and
    // the text is drawn on top of it in the text color, top-left aligned.
    virtual void render_text(
        image_view<rgba8> const& img, utf8_string const& text,
        rgba8 text_color, rgba8 background_color) = 0;

 protected:
    void initialize(alia::font const& font, font_metrics const& metrics);

 private:
    alia::font font_;
    font_metrics metrics_;
};

}

#endif
