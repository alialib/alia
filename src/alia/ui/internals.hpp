#ifndef ALIA_UI_INTERNALS_HPP
#define ALIA_UI_INTERNALS_HPP

#include <alia/ui/api.hpp>
#include <alia/layout/internals.hpp>
#include <alia/dispatch_table.hpp>
#include <map>

// This file includes various declarations necessary to implement the
// internals of the UI library.

namespace alia {

// Widgets are identified by pointers.
// Sometimes its useful to request some dummy data just to get a unique
// pointer to use as a widget ID. When doing so, using this type can make
// things clearer.
typedef char widget_identity;

// routable_widget_id identifies a widget with enough information that an
// event can be routed to it.
struct routable_widget_id
{
    widget_id id;
    routing_region_ptr region;

    routable_widget_id() : id(0) {}
    routable_widget_id(widget_id id, routing_region_ptr const& region)
      : id(id), region(region)
    {}
};
static routable_widget_id const null_widget_id(0, routing_region_ptr());

// Is the given routable_widget_id valid?
// (Only the null_widget_id is invalid.)
static inline bool is_valid(routable_widget_id const& id)
{ return id.id != 0; }

// base class for all UI events
struct ui_event
{
    virtual ~ui_event() {}

    ui_event_category category;
    ui_event_type type;

    ui_event(ui_event_category category, ui_event_type type)
      : category(category), type(type)
    {}
};

struct null_event : ui_event
{
    null_event() : ui_event(NO_CATEGORY, NO_EVENT) {}
};

struct refresh_event : ui_event
{
    refresh_event()
      : ui_event(REFRESH_CATEGORY, REFRESH_EVENT)
    {}
};

struct render_event : ui_event
{
    render_event()
      : ui_event(RENDER_CATEGORY, RENDER_EVENT)
    {}
};

struct input_event : ui_event
{
    bool acknowledged;
    ui_time_type time;

    input_event(ui_event_type type, ui_time_type time)
      : ui_event(INPUT_CATEGORY, type), acknowledged(false), time(time)
    {}
};

// keyboard events
struct text_input_event : input_event
{
    utf8_string text;

    text_input_event(ui_time_type time, utf8_string const& text)
      : input_event(TEXT_INPUT_EVENT, time), text(text)
    {}
};
struct key_event_info
{
    key_code code;
    key_modifiers mods;

    key_event_info() {}
    key_event_info(key_code code, key_modifiers mods)
      : code(code), mods(mods)
    {}
};
struct key_event : input_event
{
    key_event_info info;

    key_event(ui_event_type event, ui_time_type time,
        key_event_info const& info)
      : input_event(event, time), info(info)
    {}
};

// mouse events
struct mouse_button_event : input_event
{
    mouse_button button;

    mouse_button_event(ui_event_type event, ui_time_type time,
        mouse_button button)
      : input_event(event, time), button(button)
    {}
};
struct mouse_motion_event : input_event
{
    vector<2,int> last_mouse_position;
    bool mouse_was_in_window;
    mouse_motion_event(ui_time_type time,
        vector<2,int> const& last_mouse_position, bool mouse_was_in_window)
      : input_event(MOUSE_MOTION_EVENT, time)
      , last_mouse_position(last_mouse_position)
      , mouse_was_in_window(mouse_was_in_window)
    {}
};
struct mouse_notification_event : ui_event
{
    mouse_notification_event(ui_event_type type)
      : ui_event(INPUT_CATEGORY, type)
    {}
};
struct mouse_hit_test_event : ui_event
{
    routable_widget_id id;
    mouse_cursor cursor;

    mouse_hit_test_event()
      : ui_event(REGION_CATEGORY, MOUSE_HIT_TEST_EVENT),
        id(null_widget_id), cursor(DEFAULT_CURSOR)
    {}
};

struct mouse_wheel_event : input_event
{
    widget_id target;
    float movement;
    mouse_wheel_event(ui_time_type time, widget_id target, float movement)
      : input_event(MOUSE_WHEEL_EVENT, time)
      , movement(movement)
      , target(target)
    {}
};
struct wheel_hit_test_event : ui_event
{
    routable_widget_id id;

    wheel_hit_test_event()
      : ui_event(REGION_CATEGORY, WHEEL_HIT_TEST_EVENT),
        id(null_widget_id)
    {}
};

// If there is an active widget and it's not the one under the mouse cursor,
// we have to query it to see what cursor it wants.
struct mouse_cursor_query : ui_event
{
    widget_id id;
    mouse_cursor cursor;

    mouse_cursor_query(widget_id id)
      : ui_event(REGION_CATEGORY, MOUSE_CURSOR_QUERY_EVENT), id(id),
        cursor(DEFAULT_CURSOR)
    {}
};

struct make_widget_visible_event : ui_event
{
    make_widget_visible_event(widget_id id)
      : ui_event(REGION_CATEGORY, MAKE_WIDGET_VISIBLE_EVENT), id(id),
        acknowledged(false)
    {}
    widget_id id;
    box<2,double> region;
    bool acknowledged;
};

struct focus_notification_event : ui_event
{
    focus_notification_event(ui_event_type type, widget_id target)
      : ui_event(INPUT_CATEGORY, type)
      , target(target)
    {}
    widget_id target;
};

struct focus_predecessor_event : ui_event
{
    widget_id input_id;
    routable_widget_id predecessor;
    bool saw_input;

    focus_predecessor_event(widget_id input_id)
      : ui_event(INPUT_CATEGORY, FOCUS_PREDECESSOR_EVENT), input_id(input_id),
        predecessor(null_widget_id), saw_input(false)
    {}
};

struct focus_successor_event : ui_event
{
    widget_id input_id;
    routable_widget_id successor;
    bool just_saw_input;

    focus_successor_event(widget_id input_id)
      : ui_event(INPUT_CATEGORY, FOCUS_SUCCESSOR_EVENT), input_id(input_id),
        successor(null_widget_id), just_saw_input(false)
    {}
};

// timer event
struct timer_event : input_event
{
    timer_event(widget_id id, ui_time_type trigger_time, ui_time_type now)
      : input_event(TIMER_EVENT, now)
      , id(id)
      , trigger_time(trigger_time)
    {}
    widget_id id;
    ui_time_type trigger_time;
};

// property_map maps UI property names to values (both are strings).
typedef std::map<string,string> property_map;

struct style_tree
{
    std::map<string,style_tree> substyles;
    property_map properties;
};

struct style_search_path
{
    // first tree to search
    style_tree const* tree;
    // rest of the path
    style_search_path const* rest;
};

struct input_state
{
    unsigned mouse_button_state;
    bool mouse_inside_window;
    vector<2,int> mouse_position;
    routable_widget_id hot_id, active_id, focused_id;
    bool window_has_focus, keyboard_interaction;

    input_state()
      : mouse_inside_window(false), mouse_button_state(0),
        hot_id(null_widget_id), active_id(null_widget_id),
        focused_id(null_widget_id), window_has_focus(true),
        keyboard_interaction(false)
    {}
};

struct ui_style
{
    style_tree styles;
    dispatch_table theme;
    float text_magnification;

    // style_id identifies the current state of the above style elements.
    // If any of them change, style_id also changes.
    local_identity id;

    ui_style() : text_magnification(1) {}
};

// font is the specification of a font in alia.
// Note that the name is interpreted by Skia, and so it doesn't need to
// exactly match a font on the system.
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

// primary_style_properties are properties of a style that are considered
// so likely to be used that it's more efficient to just parse them up front.
struct primary_style_properties
{
    alia::font font;
    rgba8 text_color, background_color;
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

// Determine if a cached_image_ptr contains a valid image.
static inline bool is_valid(cached_image_ptr const& ptr)
{ return ptr && ptr->is_valid(); }

// A surface represents the device onto which the UI is rendered.
//
// The API is designed to be fairly minimal so that it's easy to implement
// new surface types.
// Most actual rendering is done via Skia and then rendered to the surface
// as an image.
//
// A surface is a geometry_context_subscriber, which means that it receives
// geometry commands about transformations and clipping from the layout engine.
// 
struct surface : geometry_context_subscriber
{
    virtual ~surface() {}

    // Get the size of the surface in pixels.
    virtual vector<2,unsigned> size() const = 0;

    // Get the number of pixels per inch on the surface.
    virtual vector<2,float> ppi() const = 0;

    // Cache the given image in the given cached_image_ptr.
    // If the cached_image_ptr is already initialized, it may be reused to
    // store the new image.
    virtual void cache_image(
        cached_image_ptr& data,
        image_interface const& image) = 0;

    // Draw a filled box with a solid color.
    virtual void draw_filled_box(rgba8 const& color,
        box<2,double> const& box) = 0;

    // Request the surface to refresh again very soon.
    // This should generally be called via the request_refresh(ctx) utility
    // function.
    // No specific time is given for when the refresh should occur.
    // If the greedy flag is set, then the refresh occurs as soon as possible.
    // Otherwise, it should occur soon enough to make an animation appear
    // smooth.
    virtual void request_refresh(bool greedy) = 0;

    // Request a timer event for the specified ID after the UI system's tick
    // count reaches the specified trigger time.
    virtual void request_timer_event(routable_widget_id const& id,
        ui_time_type trigger_time) = 0;

    // Some of this isn't all that related to rendering, but it's still
    // provided by the same code that ultimately provides the surface, and
    // there didn't seem to be a good reason to create a separate interface
    // for it...

    // Get text from the clipboard.
    virtual string get_clipboard_text() = 0;

    // Copy text to the clipboard.
    virtual void set_clipboard_text(string const& text) = 0;
};
static inline surface& get_surface(surface& surface) { return surface; }

// ui_system defines all the persistent state associated with an alia UI.
struct ui_system
{
    // stores all state and cached data associated with the UI
    data_graph data;

    layout_system layout;

    alia__shared_ptr<alia::surface> surface;

    alia__shared_ptr<ui_controller> controller;

    input_state input;

    alia__shared_ptr<ui_style> style;

    ui_time_type millisecond_tick_count;

    routable_widget_id overlay_id;

    // If this is valid, then there has been a request to make this widget
    // visible. It will be honored after the next layout calculation.
    routable_widget_id widget_to_make_visible;

    vector<2,unsigned> surface_size;

    ui_system() : millisecond_tick_count(0) {}
};

struct ui_caching_node
{
    ui_caching_node* parent;

    // cached layout info
    bool layout_valid;
    owned_id layout_id;
    layout_node* layout_subtree_head;
    layout_node** layout_subtree_tail;
};

}

#endif
