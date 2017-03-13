#ifndef ALIA_UI_INTERNALS_HPP
#define ALIA_UI_INTERNALS_HPP

#include <alia/ui/api.hpp>
#include <alia/layout/internals.hpp>
#include <alia/dispatch_table.hpp>
#include <map>
#include <list>

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
    mouse_notification_event(ui_event_type type, widget_id target)
      : ui_event(INPUT_CATEGORY, type)
      , target(target)
    {}
    widget_id target;
};
struct mouse_hit_test_event : ui_event
{
    routable_widget_id id;
    mouse_cursor cursor;
    layout_box hit_box;
    string tooltip_message;

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

struct widget_visibility_request
{
    routable_widget_id widget;
    // If this is set, the UI will jump abruptly instead of smoothly scrolling.
    bool abrupt;
    // If this is set, the widget will be moved to the top of the UI instead
    // of just being made visible.
    bool move_to_top;
};

struct make_widget_visible_event : ui_event
{
    make_widget_visible_event(widget_visibility_request const& request)
      : ui_event(REGION_CATEGORY, MAKE_WIDGET_VISIBLE_EVENT),
        request(request), acknowledged(false)
    {}
    widget_visibility_request request;
    // This gets filled in once we find the widget in question.
    box<2,double> region;
    bool acknowledged;
};

struct resolve_location_event : ui_event
{
    resolve_location_event(owned_id const& id)
      : ui_event(NO_CATEGORY, RESOLVE_LOCATION_EVENT), id(id),
        acknowledged(false)
    {}
    owned_id id;
    routable_widget_id routable_id;
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

// When a menu item is selected, this event is dispatched.
struct menu_item_selection_event : ui_event
{
    menu_item_selection_event(widget_id target)
      : ui_event(NO_CATEGORY, CUSTOM_EVENT)
      , target(target)
    {}
    widget_id target;
};

// property_map maps UI property names to values (both are strings).
typedef std::map<string,string> property_map;

struct style_tree
{
    std::map<string,alia__shared_ptr<style_tree> > substyles;
    std::list<style_tree*> fallbacks;
    property_map properties;
};

typedef alia__shared_ptr<style_tree> style_tree_ptr;

struct style_search_path
{
    // first tree to search
    // If this is 0, it serves as an inheritance separator.
    // (See ui/accessors/styling.hpp for more info.)
    style_tree const* tree;
    // rest of the path (0 if this is the end)
    style_search_path const* rest;
};

struct input_state
{
    // Is the mouse inside the window associated with this UI?
    bool mouse_inside_window;

    // the state of the mouse buttons (one bit per button)
    unsigned mouse_button_state;

    // the raw mouse position inside the window
    vector<2,int> mouse_position;

    // the ID of the widget that the mouse is over
    routable_widget_id hot_id;

    // the ID of the widget that has the mouse captured - Note that this isn't necessarily
    // the same as the hot_id.
    routable_widget_id active_id;

    // the ID of the widget that has the keyboard focus
    routable_widget_id focused_id;

    // Is the user currently dragging the mouse (with a button pressed)?
    bool dragging;

    // Does the window have focus?
    bool window_has_focus;

    // Is the user currently interacting with the UI via the keyboard? - This is used as a
    // hint to display focus indicators.
    bool keyboard_interaction;

    // If the mouse is hovering over a widget (identifiied by hot_id), this is the time at
    // which the hovering started.
    // Note that hovering is only possible if there is no active widget.
    ui_time_type hover_start_time;

    input_state()
      : mouse_inside_window(false), mouse_button_state(0),
        hot_id(null_widget_id), active_id(null_widget_id),
        focused_id(null_widget_id), window_has_focus(true),
        keyboard_interaction(false), dragging(false)
    {}
};

struct mouse_hover_context
{
    optional<string> text;
};

struct ui_style
{
    style_tree_ptr styles;

    dispatch_table theme;

    float magnification;

    // style_id identifies the current state of the above style elements.
    // If any of them change, style_id also changes.
    local_identity id;

    ui_style()
      : magnification(1)
    {}
};

// font is the specification of a font in alia.
// Note that the name is interpreted by Skia, and so it doesn't need to
// exactly match a font on the system.
ALIA_DEFINE_FLAG_TYPE(font)
ALIA_DEFINE_FLAG(font, 1, BOLD)
ALIA_DEFINE_FLAG(font, 2, ITALIC)
ALIA_DEFINE_FLAG(font, 4, STRIKETHROUGH)
ALIA_DEFINE_FLAG(font, 8, UNDERLINE)
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
void draw_full_image(
    surface& surface, cached_image_ptr const& image,
    vector<2,double> const& position,
    rgba8 const& color = rgba8(0xff, 0xff, 0xff, 0xff));

// Determine if a cached_image_ptr contains a valid image.
static inline bool is_valid(cached_image_ptr const& ptr)
{ return ptr && ptr->is_valid(); }

// An offscreen_subsurface is an abstraction that lives within a surface and
// allows for offscreen rendering.
struct offscreen_subsurface : noncopyable
{
    virtual ~offscreen_subsurface() {}

    // An offscreen_subsurface is allowed to go invalid.
    // If that happens, this returns false, and the subsurface needs to be
    // regenerated.
    virtual bool is_valid() const = 0;

    // Get the region covered of the subsurface.
    virtual box<2,unsigned> region() const = 0;

    // Blit the buffer to the surface.
    // Each pixel in the buffer is multiplied (component-wise) by the given
    // color before display.
    // (The default value for color will make this a noop.)
    virtual void blit(
        surface& surface,
        rgba8 const& color = rgba8(0xff, 0xff, 0xff, 0xff)) = 0;
};
typedef alia__shared_ptr<offscreen_subsurface> offscreen_subsurface_ptr;

// A surface represents the device onto which the UI is rendered.
//
// The API is designed to be fairly minimal so that it's easy to implement new
// surface types. Most actual rendering is done via Skia and then rendered to
// the surface as an image.
//
// A surface is a geometry_context_subscriber, which means that it receives
// geometry commands about transformations and clipping from the layout engine.
//
struct surface : geometry_context_subscriber
{
    virtual ~surface() {}

    // Cache the given image in the given cached_image_ptr.
    // If the cached_image_ptr is already initialized, it may be reused to
    // store the new image.
    virtual void cache_image(
        cached_image_ptr& data,
        image_interface const& image) = 0;

    // Generate an offscreen subsurface for rendering to the specified region
    // of the surface.
    // If the given pointer is already initialized, it may be reused to store
    // the new subsurface.
    // This is allowed to fail (or be unsupported), in which case the
    // subsurface pointer remains uninitialized.
    virtual void generate_offscreen_subsurface(
        offscreen_subsurface_ptr& subsurface,
        box<2,unsigned> const& region) = 0;

    // Set the active offscreen subsurface target.
    // A null pointer represents the actual screen surface.
    virtual void set_active_subsurface(
        offscreen_subsurface* subsurface) = 0;

    // Get the active offscreen subsurface target.
    // A null pointer represents the actual screen surface.
    virtual offscreen_subsurface* get_active_subsurface() = 0;

    // Draw a filled box with a solid color.
    virtual void draw_filled_box(rgba8 const& color,
        box<2,double> const& box) = 0;

    // This is a scale factor that affect the opacity of all textures rendered
    // to the surface.
    virtual void set_opacity(float opacity) = 0;
    virtual float opacity() const = 0;
};
static inline surface& get_surface(surface& surface) { return surface; }

// os_interface provides an interface to functionality of the underlying OS.
struct os_interface
{
    virtual ~os_interface() {}

    // Get text from the clipboard.
    virtual string get_clipboard_text() = 0;

    // Copy text to the clipboard.
    virtual void set_clipboard_text(string const& text) = 0;
};

struct ui_timer_request
{
    ui_time_type trigger_time;
    routable_widget_id id;
    counter_type frame_issued;
};
typedef std::vector<ui_timer_request> ui_timer_request_list;

enum menu_node_type
{
    ROOT_MENU_NODE,
    SUBMENU_NODE,
    MENU_SEPARATOR_NODE,
    MENU_ITEM_NODE,
};

// A menu is defined by a hierarchy of menu_nodes.
struct menu_node
{
    menu_node_type type;

    // next node in the linked list of nodes
    menu_node* next;
};

// menu_container is a menu_node with children.
struct menu_container : menu_node
{
    menu_node* children;

    menu_container* parent;

    // This records the UI context's refresh_counter when the contents of this
    // menu last changed.
    counter_type last_change;
};

// submenu_node is a menu_container representing a submenu.
struct submenu_node : menu_container
{
    keyed_data<string> label;

    bool enabled;
};

// menu_separator_node is a menu_node representing a separator.
struct menu_separator_node : menu_node
{
};

// menu_item_node is a menu_node representing an actual selectable menu item.
struct menu_item_node : menu_node
{
    keyed_data<string> label;

    bool enabled;

    // If this is a checkable menu item, then this is its checked/unchecked
    // state. (If this is none, then it's not a checkable item.)
    optional<bool> checked;
};

// This describes the state of the tooltip feature. - The tooltip feature shares state
// across the entire UI system.
struct tooltip_state
{
    // Is the tooltip system enabled? - This gets toggled on and off based on whether or
    // not the mouse is hovering over a single widget.
    bool enabled = false;

    // the message to show - If this is empty, no tooltip is active.
    string message;

    // the region (within the window) that this tooltip applies to
    layout_box generating_region;

    // the data block used for the tooltip UI
    data_block data;
};

// ui_system defines all the persistent state associated with an alia UI.
struct ui_system
{
    // stores all state and cached data associated with the UI
    data_graph data;

    layout_system layout;

    alia__shared_ptr<ui_controller> controller;

    alia__shared_ptr<alia::surface> surface;
    vector<2,unsigned> surface_size;
    vector<2,float> ppi;

    alia__shared_ptr<os_interface> os;

    input_state input;

    ui_style style;

    ui_time_type millisecond_tick_count;

    routable_widget_id overlay_id;

    std::vector<widget_visibility_request> pending_visibility_requests;

    ui_timer_request_list timer_requests;
    // This prevents timer requests from being serviced in the same frame that
    // they're requested and thus throwing the event handler into a loop.
    counter_type timer_event_counter;

    optional<ui_time_type> next_update;

    menu_container menu_bar;

    int last_refresh_duration;

    tooltip_state tooltip;
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

struct end_pass_exception {};

}

#endif
