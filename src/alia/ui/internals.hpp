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

// Currently, time is represented by a simple millisecond counter.
// (It can wrap.)
typedef unsigned ui_time_type;

// codes for all the keyboard keys recognized by alia.
enum key_code
{
    KEY_UNKNOWN         = 0,

    // ASCII keys...
    KEY_BACKSPACE       = 8,
    KEY_TAB	        = 9,
    KEY_CLEAR		= 12,
    KEY_ENTER		= 13,
    KEY_PAUSE		= 19,
    KEY_ESCAPE		= 27,
    KEY_SPACE		= 32,
    KEY_EXCLAIM		= 33,
    KEY_QUOTEDBL	= 34,
    KEY_HASH		= 35,
    KEY_DOLLAR		= 36,
    KEY_AMPERSAND	= 38,
    KEY_QUOTE		= 39,
    KEY_LEFTPAREN	= 40,
    KEY_RIGHTPAREN	= 41,
    KEY_ASTERISK	= 42,
    KEY_PLUS		= 43,
    KEY_COMMA		= 44,
    KEY_MINUS		= 45,
    KEY_PERIOD		= 46,
    KEY_SLASH		= 47,
    KEY_COLON		= 58,
    KEY_SEMICOLON	= 59,
    KEY_LESS		= 60,
    KEY_EQUALS		= 61,
    KEY_GREATER		= 62,
    KEY_QUESTION	= 63,
    KEY_AT		= 64,
    // no uppercase letters
    KEY_LEFTBRACKET	= 91,
    KEY_BACKSLASH	= 92,
    KEY_RIGHTBRACKET	= 93,
    KEY_CARET		= 94,
    KEY_UNDERSCORE	= 95,
    KEY_BACKQUOTE	= 96,
    KEY_DELETE		= 127,

    // numeric keypad 
    KEY_NUMPAD_0,
    KEY_NUMPAD_1,
    KEY_NUMPAD_2,
    KEY_NUMPAD_3,
    KEY_NUMPAD_4,
    KEY_NUMPAD_5,
    KEY_NUMPAD_6,
    KEY_NUMPAD_7,
    KEY_NUMPAD_8,
    KEY_NUMPAD_9,
    KEY_NUMPAD_PERIOD,
    KEY_NUMPAD_DIVIDE,
    KEY_NUMPAD_MULTIPLY,
    KEY_NUMPAD_SUBTRACT,
    KEY_NUMPAD_ADD,
    KEY_NUMPAD_ENTER,

    // arrows + home/end pad
    KEY_UP,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_LEFT,
    KEY_INSERT,
    KEY_HOME,
    KEY_END,
    KEY_PAGEUP,
    KEY_PAGEDOWN,

    // function keys
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_F21,
    KEY_F22,
    KEY_F23,
    KEY_F24,

    // key state modifier keys
    KEY_NUMLOCK,
    KEY_CAPSLOCK,
    KEY_SCROLLOCK,
    KEY_RSHIFT,
    KEY_LSHIFT,
    KEY_RCTRL,
    KEY_LCTRL,
    KEY_RALT,
    KEY_LALT,
    KEY_RMETA,
    KEY_LMETA,

    // miscellaneous function keys
    KEY_HELP,
    KEY_PRINT,
    KEY_PRINT_SCREEN,
    KEY_BREAK,
    KEY_MENU,
};

// keyboard modifier keys
struct kmod_flag_tag {};
typedef flag_set<kmod_flag_tag> key_modifiers;
ALIA_DEFINE_FLAG_CODE(kmod_flag_tag, 0x00, KMOD_NONE)
ALIA_DEFINE_FLAG_CODE(kmod_flag_tag, 0x01, KMOD_SHIFT)
ALIA_DEFINE_FLAG_CODE(kmod_flag_tag, 0x02, KMOD_CTRL)
ALIA_DEFINE_FLAG_CODE(kmod_flag_tag, 0x04, KMOD_ALT)
ALIA_DEFINE_FLAG_CODE(kmod_flag_tag, 0x08, KMOD_WIN)
ALIA_DEFINE_FLAG_CODE(kmod_flag_tag, 0x10, KMOD_META)

enum mouse_button
{
    LEFT_BUTTON,
    MIDDLE_BUTTON,
    RIGHT_BUTTON
};

// standard mouse cursors that are expected to be supplied by the backend
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

enum ui_event_category
{
    NO_CATEGORY,
    REFRESH_CATEGORY,
    REGION_CATEGORY,
    INPUT_CATEGORY,
    RENDER_CATEGORY,
};

enum ui_event_type
{
    NO_EVENT,

    REFRESH_EVENT,

    // rendering
    RENDER_EVENT,

    // regions
    MAKE_WIDGET_VISIBLE_EVENT,
    MOUSE_HIT_TEST_EVENT,
    WHEEL_HIT_TEST_EVENT,

    // keyboard
    TEXT_INPUT_EVENT,
    BACKGROUND_TEXT_INPUT_EVENT,
    KEY_PRESS_EVENT,
    BACKGROUND_KEY_PRESS_EVENT,
    KEY_RELEASE_EVENT,
    BACKGROUND_KEY_RELEASE_EVENT,

    // focus notifications
    FOCUS_GAIN_EVENT,
    FOCUS_LOSS_EVENT,

    // focus queries
    FOCUS_PREDECESSOR_EVENT,
    FOCUS_SUCCESSOR_EVENT,
    FOCUS_RECOVERY_EVENT,

    // mouse
    MOUSE_PRESS_EVENT,
    DOUBLE_CLICK_EVENT,
    MOUSE_RELEASE_EVENT,
    MOUSE_MOTION_EVENT,
    MOUSE_WHEEL_EVENT,
    MOUSE_CURSOR_QUERY_EVENT,
    MOUSE_GAIN_EVENT,
    MOUSE_LOSS_EVENT,

    // uncategorized events
    WRAPPED_EVENT,
    SET_VALUE_EVENT,
    TIMER_EVENT,
    INITIAL_VISIBILITY_EVENT,
    CUSTOM_EVENT,
};

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
struct mouse_notification_event : ui_event
{
    mouse_notification_event(ui_event_type type)
      : ui_event(INPUT_CATEGORY, type)
    {}
};

struct wheel_hit_test_event : ui_event
{
    routable_widget_id id;
    double hit_z;

    wheel_hit_test_event()
      : ui_event(REGION_CATEGORY, WHEEL_HIT_TEST_EVENT),
        id(null_widget_id), hit_z(0)
    {}
};

struct mouse_hit_test_event : ui_event
{
    routable_widget_id id;
    double hit_z;
    mouse_cursor cursor;

    mouse_hit_test_event()
      : ui_event(REGION_CATEGORY, MOUSE_HIT_TEST_EVENT),
        id(null_widget_id), hit_z(0), cursor(DEFAULT_CURSOR)
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

typedef std::map<string,property_map> flattened_style_tree;

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

// cached_rendering_content is used to record rendering actions to a surface
// and play them back later.
// cached_rendering_content is analagous to cached_image in that it provides
// ownership of the recorded content.
struct cached_rendering_content : noncopyable
{
    virtual ~cached_rendering_content() {}

    // Start recording content that's sent to the associated surface.
    virtual void start_recording() = 0;
    // Stop recording.
    virtual void stop_recording() = 0;

    // Playback the content.
    virtual void playback(surface& surface) const = 0;

    // cached_rendering_content is allowed to go invalid.
    // If that happens, this returns false, and the content needs to be
    // recached.
    virtual bool is_valid() const = 0;
};
typedef alia__shared_ptr<cached_rendering_content>
    cached_rendering_content_ptr;

// Determine if a cached_rendering_content_ptr contains valid content.
static inline bool is_valid(cached_rendering_content_ptr const& ptr)
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

    // Initialize a cached_rendering_content_ptr with empty content so that
    // it can be used to record.
    virtual void create_cached_content(
        cached_rendering_content_ptr& data) = 0;

    // Set the layer in which drawing should occur.
    virtual void set_layer_z(double z) = 0;

    // Request the surface to refresh again as soon as possible.
    // This is called when a widget is animating and wants the surface to
    // update continuously.
    virtual void request_refresh() = 0;

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

    // If this is valid, then there has been a request to make this widget
    // visible. It will be honored after the next layout calculation.
    routable_widget_id widget_to_make_visible;

    ui_system() : millisecond_tick_count(0) {}
};

}

#endif
