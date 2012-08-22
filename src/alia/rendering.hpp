#ifndef ALIA_RENDERING_HPP
#define ALIA_RENDERING_HPP

#include <alia/data_graph.hpp>
#include <alia/geometry.hpp>
#include <alia/color.hpp>

namespace alia {

// fonts and font metrics

struct font_flag_tag {};
typedef flag_set<font_flag_tag> font_flag_set;

ALIA_DEFINE_FLAG_CODE(font_flag_tag, 1, BOLD)
ALIA_DEFINE_FLAG_CODE(font_flag_tag, 2, ITALIC)
ALIA_DEFINE_FLAG_CODE(font_flag_tag, 4, STRIKETHROUGH)
ALIA_DEFINE_FLAG_CODE(font_flag_tag, 8, UNDERLINE)

struct font
{
    string name;
    float size;
    font_flag_set style;

    font() {}
    font(string const& name, float size, font_flag_set style = NO_FLAGS)
      : name(name), size(size), style(style)
    {}
};
static inline bool is_bold(font const& f)
{ return (f.style & BOLD) ? true : false; }
static inline bool is_italic(font const& f)
{ return (f.style & ITALIC) ? true : false; }
static inline bool is_underlined(font const& f)
{ return (f.style & UNDERLINE) ? true : false; }
static inline bool is_strikethrough(font const& f)
{ return (f.style & STRIKETHROUGH) ? true : false; }
bool operator==(font const& a, font const& b);
bool operator!=(font const& a, font const& b);
bool operator<(font const& a, font const& b);

struct font_metrics
{
    int height;
    int ascent;
    int descent;
    int average_width;
    int row_gap;
    int overhang;
};

// pixel formats supported for rendering
enum pixel_format
{
    GRAY,
    ALPHA,
    RGB,
    RGBA, // w/ premultiplied alpha
};
unsigned get_channel_count(pixel_format fmt);

// image_interface is the interface for specifying images for rendering.
struct image_interface
{
    // pointer to pixel data
    void const* pixels;

    pixel_format format;

    // dimensions of image, in pixels
    vector<2,unsigned> size;

    // distance (in units of Pixels) between the start of neighboring rows
    unsigned stride;

    image_interface() {}
    image_interface(
        void const* pixels, pixel_format format,
        vector<2,unsigned> const& size, unsigned stride)
      : pixels(pixels), format(format), size(size), stride(stride)
    {}
};

// A cached_image represents an image that has been cached on a surface.
// It provides ownership of the image. If the cached_image is destroyed, the
// image will no longer be stored on the surface.
struct cached_image : noncopyable
{
    virtual ~cached_image() {}

    // A cached_image is allowed to go invalid.
    // If that happens, this returns false, and the image needs to be recached.
    virtual bool is_valid() const = 0;

    // Get the size of the image.
    virtual vector<2,unsigned> size() const = 0;

    // Draw the image on the surface at the given position.
    // Each pixel in the image is multiplied (component-wise) by the given
    // color before display.
    // (The default value for color will make this a noop.)
    virtual void draw(
        vector<2,double> const& position,
        rgba8 const& color = rgba8(0xff, 0xff, 0xff, 0xff)) = 0;
    // Identical to above, but only a subregion of the image is displayed.
    // region is specified in pixels.
    virtual void draw_region(
        vector<2,double> const& position,
        box<2,double> const& region,
        rgba8 const& color = rgba8(0xff, 0xff, 0xff, 0xff)) = 0;
};
typedef alia__shared_ptr<cached_image> cached_image_ptr;

static inline bool is_valid(cached_image_ptr const& ptr)
{ return ptr && ptr->is_valid(); }

// Line styles are specified in the same format as in OpenGL.

struct line_stipple
{
    line_stipple() {}
    line_stipple(int factor, uint16_t pattern)
      : factor(factor), pattern(pattern)
    {}

    int factor;
    uint16_t pattern;

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
    virtual bool is_open() const = 0;
    virtual void close() = 0;
};
typedef alia__shared_ptr<popup_interface> popup_ptr;

static inline bool is_open(popup_ptr const& popup)
{ return popup && popup->is_open(); }

struct ui_controller;

// A surface represents the device onto which the UI is rendered.
// The API is designed to be fairly minimal so that it's easy to implement
// new surface types.
// Most actual rendering is done via Skia and then rendered to the surface
// as an image.

// Currently, time is represented by a simple millisecond counter.
// (It can wrap.)
typedef unsigned ui_time_type;

struct routable_widget_id;

struct cached_rendering_content
{
    virtual ~cached_rendering_content() {}

    virtual void start_recording() = 0;
    virtual void stop_recording() = 0;

    virtual void playback() const = 0;

    // A cached_image is allowed to go invalid.
    // If that happens, this returns false, and the content needs to be
    // recached.
    virtual bool is_valid() const = 0;
};
typedef alia__shared_ptr<cached_rendering_content>
    cached_rendering_content_ptr;

static inline bool is_valid(cached_rendering_content_ptr const& ptr)
{ return ptr && ptr->is_valid(); }

struct surface : geometry_context_subscriber
{
    virtual ~surface() {}

    // Get the size of the surface in pixels.
    virtual vector<2,unsigned> size() const = 0;

    // Get the size of the full display, in pixels.
    virtual vector<2,unsigned> display_size() const = 0;

    // Get the number of pixels per inch on the surface.
    virtual vector<2,float> ppi() const = 0;

    virtual void cache_image(
        cached_image_ptr& data,
        image_interface const& img) = 0;

    virtual void cache_content(
        cached_rendering_content_ptr& data) = 0;

    // Open a popup above the surface.
    virtual popup_interface* open_popup(
        ui_controller* controller,
        vector<2,int> const& primary_position,
        vector<2,int> const& boundary,
        vector<2,int> const& minimum_size = make_vector<int>(0, 0)) = 0;

    // Close existing popups.
    virtual void close_popups() = 0;

    // Request the surface to refresh again as soon as possible.
    virtual void request_refresh() = 0;

    // Request a timer event for the specified ID after the UI system's tick
    // count reaches the specified trigger time.
    virtual void request_timer_event(routable_widget_id const& id,
        ui_time_type trigger_time) = 0;

    // Get text from the clipboard.
    virtual string get_clipboard_text() = 0;

    // Copy text to the clipboard.
    virtual void set_clipboard_text(string const& text) = 0;

  // DRAWING...

    // Draw a line segment.
    virtual void draw_line(rgba8 const& color, line_style const& style,
        vector<2,double> const& p1, vector<2,double> const& p2) = 0;
    virtual void draw_line(rgba8 const& color, line_style const& style,
        vector<2,float> const& p1, vector<2,float> const& p2) = 0;
    virtual void draw_line(rgba8 const& color, line_style const& style,
        vector<2,int> const& p1, vector<2,int> const& p2) = 0;

    // Draw a line strip.
    virtual void draw_line_strip(rgba8 const& color, line_style const& style,
        vector<2,double> const* vertices, unsigned n_vertices) = 0;
    virtual void draw_line_strip(rgba8 const& color, line_style const& style,
        vector<2,float> const* vertices, unsigned n_vertices) = 0;
    virtual void draw_line_strip(rgba8 const& color, line_style const& style,
        vector<2,int> const* vertices, unsigned n_vertices) = 0;

    // Draw a line loop.
    virtual void draw_line_loop(rgba8 const& color, line_style const& style,
        vector<2,double> const* vertices, unsigned n_vertices) = 0;
    virtual void draw_line_loop(rgba8 const& color, line_style const& style,
        vector<2,float> const* vertices, unsigned n_vertices) = 0;
    virtual void draw_line_loop(rgba8 const& color, line_style const& style,
        vector<2,int> const* vertices, unsigned n_vertices) = 0;

    // Draw a filled shape.
    virtual void draw_filled_polygon(rgba8 const& color,
        vector<2,double> const* vertices, unsigned n_vertices) = 0;
    virtual void draw_filled_polygon(rgba8 const& color,
        vector<2,float> const* vertices, unsigned n_vertices) = 0;
    virtual void draw_filled_polygon(rgba8 const& color,
        vector<2,int> const* vertices, unsigned n_vertices) = 0;
};

typedef keyed_data<cached_image_ptr> caching_renderer_data;

struct caching_renderer
{
    caching_renderer() : surface_(0) {}

    template<class Context>
    caching_renderer(Context& ctx, caching_renderer_data& data,
        id_interface const& content_id, box<2,int> const& region)
    { begin(ctx, data, content_id, region); }

    ~caching_renderer() { end(); }

    template<class Context>
    void begin(Context& ctx, caching_renderer_data& data,
        id_interface const& content_id, box<2,int> const& region)
    {
        begin(data, get_surface(ctx), content_id, region);
    }

    void begin(caching_renderer_data& data, surface& surface,
        id_interface const& content_id, box<2,int> const& region);

    void end() {}

    bool needs_rendering() const { return needs_rendering_; }

    cached_image_ptr& image() { return data_->value; }

    void mark_valid() { data_->is_valid = true; }

    void draw();

 private:
    caching_renderer_data* data_;
    surface* surface_;
    vector<2,int> position_;
    bool needs_rendering_;
};

}

#endif
