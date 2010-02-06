#ifndef ALIA_CACHED_ASCII_TEXT_HPP
#define ALIA_CACHED_ASCII_TEXT_HPP

#include <alia/ascii_text_renderer.hpp>
#include <vector>
#include <boost/scoped_array.hpp>

namespace alia {

class cached_ascii_text : public cached_text
{
 public:
    cached_ascii_text(surface& surface,
        ascii_text_renderer const& renderer,
        std::string const& text,
        int width,
        unsigned flags);

    unsigned get_line_count() const;
    offset get_line_begin(unsigned line_n) const;
    offset get_line_end(unsigned line_n) const;

    void draw(point2d const& p, rgba8 const& fg) const;
    void draw_with_highlight(point2d const& p, rgba8 const& fg,
        rgba8 const& highlight_bg, rgba8 const& highlight_fg,
        offset highlight_begin, offset highlight_end) const;
    void draw_cursor(point2d const& p, rgba8 const& color) const;

    offset get_character_at_point(point2i const& p) const;
    offset get_character_boundary_at_point(point2i const& p) const;
    unsigned get_line_number(offset position) const;
    point2i get_character_position(offset position) const;
    offset shift(offset position, int shift) const;

 private:
    void draw_subimage(point2d const& p, box2d r, rgba8 const& bg,
        rgba8 const& fg) const;
    void draw_subimage(point2d const& p, box2d r, rgba8 const& fg) const;

    void calculate_character_widths(ascii_text_renderer const& renderer,
        std::string const& text, unsigned length);
    int calculate_line_spans(std::string const& text, int width);
    unsigned calculate_truncation(std::string const& text, int width,
        bool greedy);

    surface& surface_;

    image<uint8> text_image_;
    boost::scoped_array<int> character_widths_;
    boost::scoped_array<int> trailing_space_;

    mutable cached_image_ptr display_image_;

    struct line_span
    {
        offset begin, end;
    };
    std::vector<line_span> line_spans_;
};

}

#endif
