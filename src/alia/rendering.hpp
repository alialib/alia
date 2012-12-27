#ifndef ALIA_RENDERING_HPP
#define ALIA_RENDERING_HPP

#include <alia/data_graph.hpp>
#include <alia/layout_interface.hpp>
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

struct surface;

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

    // Draw a portion of the image over the given surface region.
    // Each pixel in the image is multiplied (component-wise) by the given
    // color before display.
    // (The default value for color will make this a noop.)
    virtual void draw(
        surface& surface,
        box<2,double> const& surface_region,
        box<2,double> const& image_region,
        rgba8 const& color = rgba8(0xff, 0xff, 0xff, 0xff)) = 0;
};
typedef alia__shared_ptr<cached_image> cached_image_ptr;

// Given a cached image, this draws the full image at a particular position
// on a particular surface. The surface region is constructed to be the same
// size as the image. (The surface's transformation matrix is still applied,
// so this doesn't necessarily imply a 1-to-1 mapping of image pixels to
// surface pixels.)
void draw_full_image(surface& surface, cached_image_ptr const& image,
    vector<2,double> const& position);

static inline bool is_valid(cached_image_ptr const& ptr)
{ return ptr && ptr->is_valid(); }

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

    virtual void playback(surface& surface) const = 0;

    // cached_rendering_content is allowed to go invalid.
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

    // Get the number of pixels per inch on the surface.
    virtual vector<2,float> ppi() const = 0;

    virtual void cache_image(
        cached_image_ptr& data,
        image_interface const& img) = 0;

    virtual void cache_content(
        cached_rendering_content_ptr& data) = 0;

    // Set the layer in which drawing should occur.
    virtual void set_layer_z(double z) = 0;

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

    virtual void draw_filled_box(rgba8 const& color,
        box<2,double> const& box) = 0;
};
static inline surface& get_surface(surface& surface) { return surface; }

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
        begin(data, get_surface(ctx), get_geometry_context(ctx),
            content_id, region);
    }

    void begin(caching_renderer_data& data, surface& surface,
        geometry_context& geometry,
        id_interface const& content_id, box<2,int> const& region);

    void end() {}

    bool needs_rendering() const { return needs_rendering_; }

    cached_image_ptr& image() { return data_->value; }

    void mark_valid() { data_->is_valid = true; }

    void draw();

 private:
    caching_renderer_data* data_;
    surface* surface_;
    box<2,int> region_;
    bool needs_rendering_;
};

}

#endif
