#ifndef ALIA_SURFACE_HPP
#define ALIA_SURFACE_HPP

#include <alia/forward.hpp>
#include <alia/color.hpp>
#include <alia/matrix.hpp>
#include <alia/box.hpp>
#include <alia/font.hpp>
#include <alia/image_interface.hpp>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace alia {

// Line styles are specified in the same format as in OpenGL.
struct line_stipple
{
    line_stipple() {}
    line_stipple(int factor, uint16 pattern)
      : factor(factor), pattern(pattern)
    {}

    int factor;
    uint16 pattern;

    bool operator == (line_stipple const& other) const
    { return factor == other.factor && pattern == other.pattern; }
    bool operator != (line_stipple const& other) const
    { return factor != other.factor || pattern != other.pattern; }
};

extern line_stipple no_line, solid_line, dashed_line, dotted_line;

typedef float line_width;

struct line_style
{
    line_style() {}
    line_style(line_width width, line_stipple stipple)
      : width(width), stipple(stipple)
    {}
    line_width width;
    line_stipple stipple;
};

struct cached_image : boost::noncopyable
{
    virtual ~cached_image() {}

    virtual bool valid() const = 0;

    virtual vector2i size() const = 0;

    virtual void draw(
        point2d const& position,
        rgba8 const& color = rgba8(0xff, 0xff, 0xff, 0xff)) = 0;

    virtual void draw_region(
        point2d const& position,
        box2d const& region,
        rgba8 const& color = rgba8(0xff, 0xff, 0xff, 0xff)) = 0;
};
typedef boost::scoped_ptr<cached_image> cached_image_ptr;

inline bool is_valid(cached_image_ptr const& img)
{ return img && img->valid(); }

class cached_text : boost::noncopyable
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

    std::string const& get_text() const { return text_; }
    font const& get_font() const { return font_; }

    font_metrics const& get_metrics() const { return metrics_; }

    // Get the size of the entire block of text.
    vector2i get_size() const { return size_; }

    // Get the number of lines of text.
    virtual unsigned get_line_count() const = 0;

    typedef int offset;

    virtual offset get_line_begin(unsigned line_n) const = 0;

    virtual offset get_line_end(unsigned line_n) const = 0;

    virtual void draw(
        point2d const& position, rgba8 const& fg) const = 0;

    virtual void draw_with_highlight(
        point2d const& position, rgba8 const& fg,
        rgba8 const& highlight_bg, rgba8 const& highlight_fg,
        offset highlight_begin, offset highlight_end) const = 0;

    virtual void draw_cursor(
        point2d const& p, rgba8 const& color) const = 0;

    virtual offset get_character_at_point(point2i const& p) const = 0;

    virtual offset get_character_boundary_at_point(point2i const& p) const = 0;

    // Get the index of the line that contains the given offset.
    virtual unsigned get_line_number(offset position) const = 0;

    // Get the position of the given character within the block of text.
    virtual point2i get_character_position(offset position) const = 0;

    virtual offset shift(offset position, int shift) const = 0;

    virtual ~cached_text() {}

 protected:
    void initialize(
        std::string const& text,
        font const& font,
        vector2i const& size,
        font_metrics const& metrics);

 private:
    std::string text_;
    font font_;
    vector2i size_;
    font_metrics metrics_;
};
typedef boost::scoped_ptr<cached_text> cached_text_ptr;

enum mouse_cursor
{
    DEFAULT_CURSOR,
    CROSS_CURSOR,
    BUSY_CURSOR,
    BLANK_CURSOR,
    IBEAM_CURSOR,
    NO_ENTRY_CURSOR,
    HAND_CURSOR,
    LEFT_RIGHT_ARROW_CURSOR,
    UP_DOWN_ARROW_CURSOR,
    FOUR_WAY_ARROW_CURSOR,
};

class popup_interface
{
 public:
    virtual ~popup_interface() {}
    // TODO: clean this up
    virtual bool is_open() const = 0;
    virtual bool was_dismissed() const = 0;
    virtual void close() = 0;
};
typedef boost::scoped_ptr<popup_interface> popup_ptr;

class surface
{
 public:
    virtual ~surface() {}

  // SURFACE CHARACTERISTICS...

    // Get the size of the surface, in pixels.
    virtual vector2i get_size() const = 0;

    // Get the PPI (pixels per inch) of the surface.
    virtual vector2d get_ppi() const = 0;

    // Get the size of the display that contains the surface, in pixels.
    virtual vector2i get_display_size() const { return get_size(); }

    // Get the location of the upperleft corner of the surface on the display.
    virtual point2i get_location_on_display() const { return point2i(0, 0); }

  // STATE...

    void reset_drawing_state();

    box2i const& get_clip_region() const
    { return clip_region_; }
    void set_clip_region(box2i const& region)
    {
        clip_region_ = region;
        apply_clip_region();
    }

    matrix<3,3,double> const& get_transformation_matrix() const
    { return transformation_matrix_; }
    void set_transformation_matrix(matrix<3,3,double> const& matrix)
    {
        transformation_matrix_ = matrix;
        apply_transformation_matrix();
    }

  // DRAWING...

    // Draw a line segment.
    virtual void draw_line(rgba8 const& color, line_style const& style,
        point2d const& p1, point2d const& p2) = 0;
    virtual void draw_line(rgba8 const& color, line_style const& style,
        point2f const& p1, point2f const& p2) = 0;
    virtual void draw_line(rgba8 const& color, line_style const& style,
        point2i const& p1, point2i const& p2) = 0;

    // Draw a line strip.
    // The default implementation simply calls draw_line for each segment.
    virtual void draw_line_strip(rgba8 const& color, line_style const& style,
        point2d const* vertices, unsigned n_vertices);
    virtual void draw_line_strip(rgba8 const& color, line_style const& style,
        point2f const* vertices, unsigned n_vertices);
    virtual void draw_line_strip(rgba8 const& color, line_style const& style,
        point2i const* vertices, unsigned n_vertices);

    // Draw a line loop.
    // The default implementation simply calls draw_line for each segment.
    virtual void draw_line_loop(rgba8 const& color, line_style const& style,
        point2d const* vertices, unsigned n_vertices);
    virtual void draw_line_loop(rgba8 const& color, line_style const& style,
        point2f const* vertices, unsigned n_vertices);
    virtual void draw_line_loop(rgba8 const& color, line_style const& style,
        point2i const* vertices, unsigned n_vertices);

    // Draw a filled shape.
    virtual void draw_filled_polygon(rgba8 const& color,
        point2d const* vertices, unsigned n_vertices) = 0;
    virtual void draw_filled_polygon(rgba8 const& color,
        point2f const* vertices, unsigned n_vertices) = 0;
    virtual void draw_filled_polygon(rgba8 const& color,
        point2i const* vertices, unsigned n_vertices) = 0;

  // IMAGE RENDERING...

    // cache_image flags...

    // internal format - If omitted, this is automatically determined from
    // the image's pixel format.
    static unsigned const ALPHA_IMAGE = 1;
    static unsigned const GRAY_IMAGE = 2;
    static unsigned const RGB_IMAGE = 3;
    static unsigned const RGBA_IMAGE = 4;

    // If this is specified, the edges of the image will wrap around when
    // rendered. Otherwise, they are clamped.
    static unsigned const TILED_IMAGE = 0x10;

    virtual void cache_image(
        cached_image_ptr& data,
        image_interface const& img,
        unsigned flags = 0) = 0;

  // TEXT RENDERING...

    virtual void get_font_metrics(font_metrics* metrics, font const& font) = 0;

    static unsigned const TRUNCATE = 0x1;
    static unsigned const GREEDY = 0x2;

    virtual void cache_text(
        cached_text_ptr& data,
        font const& font,
        char const* text,
        int width = 0,
        unsigned flags = 0) = 0;

    virtual vector2i get_ascii_text_size(font const& font,
        char const* text) = 0;

    virtual void draw_ascii_text(
        point2d const& location,
        rgba8 const& color,
        font const& font,
        char const* text) = 0;

  // OTHER...

    virtual void set_mouse_cursor(mouse_cursor cursor) = 0;

    // Request a timer event for the specified ID after the specified time
    // (in milliseconds) has elapsed. If 0 is passed in, the event will occur
    // as soon as possible.
    virtual void request_timer_event(region_id id, unsigned ms = 0) = 0;

    // TODO: Some of this stuff really doesn't belong in here.

    // Get text from the clipboard.
    virtual std::string get_clipboard_text() = 0;

    // Copy text to the clipboard.
    virtual void set_clipboard_text(std::string const& text) = 0;

    // If the surface is interactive, this will close it.
    virtual void close() = 0;

    // Open a popup above the surface.
    // This will only be called on interactive surfaces.
    virtual popup_interface* open_popup(controller* controller,
        point2i const& display_location, point2i const& boundary,
        vector2i const& minimum_size = vector2i(-1, -1),
        vector2i const& maximum_size = vector2i(-1, -1),
        bool right_aligned = false) = 0;

    // Show a popup menu.
    virtual void show_popup_menu(menu_controller* controller) = 0;

    // Beep (used in response to user errors).
    virtual void beep() = 0;

    // Display a color selection dialog.
    // Initial is an (optional) initial value for the color.
    // If the user selects a color, *result will be set to the color and the
    // function will return true.  Otherwise, it returns false.
    virtual bool ask_for_color(rgb8* result, rgb8 const* initial = 0) = 0;

 private:
    virtual void apply_clip_region() = 0;
    virtual void apply_transformation_matrix() = 0;

  // DATA...

    box2i clip_region_;
    matrix<3,3,double> transformation_matrix_;
};

}

#endif
