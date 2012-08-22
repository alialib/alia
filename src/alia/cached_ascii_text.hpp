#ifndef ALIA_CACHED_ASCII_TEXT_HPP
#define ALIA_CACHED_ASCII_TEXT_HPP

#include <alia/ascii_text_renderer.hpp>
#include <vector>
#include <boost/scoped_array.hpp>

namespace alia {

static unsigned const TRUNCATE = 0x1;
static unsigned const GREEDY = 0x2;

class cached_text : noncopyable
{
 public:
    // Character positions are always specified using byte offsets which
    // correspond to the original string passed in.  Note that for Unicode
    // text, not all byte offsets correspond to valid character positions.
    // Since the renderer has to understand the underlying Unicode characters
    // in order to render them properly, it is also responsible for determining
    // valid byte offsets.  The only byte offsets that are guaranteed to be
    // valid are 0 and the full length of the string.  Other byte offsets
    // should be determined using this interface.

    string const& get_text() const { return text_; }
    font const& get_font() const { return font_; }

    font_metrics const& get_metrics() const { return metrics_; }

    // Get the size of the entire block of text.
    vector<2,int> get_size() const { return size_; }

    // Get the number of lines of text.
    virtual unsigned get_line_count() const = 0;

    typedef int offset;

    virtual offset get_line_begin(unsigned line_n) const = 0;

    virtual offset get_line_end(unsigned line_n) const = 0;

    virtual void draw(
        surface& surface,
        vector<2,double> const& position) = 0;

    virtual void draw_with_selection(
        surface& surface,
        vector<2,double> const& position,
        offset highlight_begin, offset highlight_end) = 0;

    virtual void draw_cursor(
        surface& surface, bool selected,
        vector<2,double> const& p) = 0;

    virtual offset get_character_at_point(vector<2,int> const& p) const = 0;

    virtual offset get_character_boundary_at_point(
        vector<2,int> const& p) const = 0;

    // Get the index of the line that contains the given offset.
    virtual unsigned get_line_number(offset position) const = 0;

    // Get the position of the given character within the block of text.
    virtual vector<2,int> get_character_position(offset position) const = 0;

    virtual offset shift(offset position, int shift) const = 0;

    virtual ~cached_text() {}

 protected:
    void initialize(
        string const& text,
        font const& font,
        vector<2,int> const& size,
        font_metrics const& metrics);

 private:
    string text_;
    font font_;
    vector<2,int> size_;
    font_metrics metrics_;
};
typedef alia__shared_ptr<cached_text> cached_text_ptr;

class cached_ascii_text : public cached_text
{
 public:
    cached_ascii_text(
        ascii_font_image const& font_image,
        ascii_font_image const& highlight_font_image,
        string const& text,
        int width = 0,
        unsigned flags = 0);

    unsigned get_line_count() const;
    offset get_line_begin(unsigned line_n) const;
    offset get_line_end(unsigned line_n) const;

    void draw(surface& surface, vector<2,double> const& p);
    void draw_with_selection(
        surface& surface, vector<2,double> const& p,
        offset highlight_begin, offset highlight_end);
    void draw_cursor(surface& surface, bool selected,
        vector<2,double> const& p);

    offset get_character_at_point(vector<2,int> const& p) const;
    offset get_character_boundary_at_point(vector<2,int> const& p) const;
    unsigned get_line_number(offset position) const;
    vector<2,int> get_character_position(offset position) const;
    offset shift(offset position, int shift) const;

 private:
    void draw_subimage(vector<2,double> const& p, box<2,double> r);
    void draw_highlight_subimage(
        vector<2,double> const& p, box<2,double> r);

    void generate_image(image<rgba8>& text_image, 
        ascii_font_image const& font_image, string const& text, int width);

    void calculate_character_widths(ascii_font_image const& font_image,
        string const& text, unsigned length);
    int calculate_line_spans(string const& text, int width);
    unsigned calculate_truncation(string const& text, int width, bool greedy);

    boost::scoped_array<int> character_widths_;
    boost::scoped_array<int> trailing_space_;

    ascii_font_image const* font_image_;
    image<rgba8> text_image_;
    cached_image_ptr display_image_;

    ascii_font_image const* highlight_font_image_;
    image<rgba8> highlight_image_;
    cached_image_ptr highlight_display_image_;

    struct line_span
    {
        offset begin, end;
    };
    std::vector<line_span> line_spans_;
};

}

#endif
