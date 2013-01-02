#ifndef ALIA_UI_API_HPP
#define ALIA_UI_API_HPP

#include <alia/accessors.hpp>
#include <alia/color.hpp>
#include <alia/layout/api.hpp>
#include <alia/event_routing.hpp>

// This file declares all the types and functions necessary to use the UI
// library from the application end, including a standard library of widgets
// and containers.

// The implementations of the functions declared in here are spread across the
// source files in the ui/library subdirectory.

namespace alia {

struct ui_context;

typedef void const* widget_id;
static widget_id const auto_id = 0;

// Often, widgets with internal storage will want to give the application the
// option of providing their own storage for that data. (This is useful if the
// application wants to persist that storage or needs to manipulate it directly
// in reponse to other user actions.)
// In those cases, the widget can accept an optional_storage<T> argument.
// The application can then call storage(accessor) to provide an accessor for
// the widget's storage, or it can pass in none to opt out of providing
// storage.
template<class T>
struct optional_storage : noncopyable
{
    optional_storage() {}
    optional_storage(none_type _) : storage(0) {}
    // 0 if no storage provided
    accessor<T> const* storage;
};
template<class Accessor>
struct accessor_storage : optional_storage<typename Accessor::value_type>
{
    accessor_storage(Accessor const& accessor)
      : accessor_(accessor)
    {
        this->storage = &accessor_;
    }
 private:
    Accessor accessor_;
};
template<class Accessor>
accessor_storage<Accessor> storage(Accessor const& accessor)
{ return accessor_storage<Accessor>(accessor); }

// resolve_storage(optional_storage, fallback) returns an accessor to the
// optional storage iff it's valid and to the fallback storage otherwise.
template<class T>
accessor_mux<indirect_accessor<T>,inout_accessor<T> >
resolve_storage(optional_storage<T> const& s, T* fallback)
{
    return select_accessor(s.storage != 0, ref(*s.storage), inout(fallback));
}

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
    OVERLAY_CATEGORY,
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

    // overlays
    OVERLAY_MOUSE_HIT_TEST_EVENT,
    OVERLAY_WHEEL_HIT_TEST_EVENT,
    OVERLAY_RENDER_EVENT,
    OVERLAY_MAKE_WIDGET_VISIBLE_EVENT,

    // uncategorized events
    WRAPPED_EVENT,
    SET_VALUE_EVENT,
    TIMER_EVENT,
    INITIAL_VISIBILITY_EVENT,
    CUSTOM_EVENT,
};

// STYLING

struct ui_controller : noncopyable
{
    virtual ~ui_controller() {}

    virtual void do_ui(ui_context& ctx) = 0;
};

// widget state flags
struct widget_state_flag_tag {};
typedef flag_set<widget_state_flag_tag> widget_state;
// primary state
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x01, WIDGET_NORMAL)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x02, WIDGET_DISABLED)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x03, WIDGET_HOT)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x04, WIDGET_DEPRESSED)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x05, WIDGET_SELECTED)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x0f, WIDGET_PRIMARY_STATE_MASK)
// additional (independent) states
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x10, WIDGET_FOCUSED)

struct style_search_path;
struct dispatch_table;
struct primary_style_properties;

struct style_state
{
    style_search_path const* path;
    dispatch_table const* theme;
    primary_style_properties const* properties;
    id_interface const* id;
};

struct layout_style_info;

struct scoped_substyle : noncopyable
{
    scoped_substyle() : ctx_(0) {}
    scoped_substyle(ui_context& ctx, getter<string> const& substyle_name,
        widget_state state = WIDGET_NORMAL)
    { begin(ctx, substyle_name, state); }
    ~scoped_substyle() { end(); }

    void begin(ui_context& ctx, getter<string> const& substyle_name,
        widget_state state = WIDGET_NORMAL);
    void end();

 private:
    ui_context* ctx_;
    style_state old_state_;
    layout_style_info const* old_style_info_;
};

// CULLING

struct culling_block
{
    culling_block() {}
    culling_block(ui_context& ctx) { begin(ctx); }
    ~culling_block() { end(); }
    void begin(ui_context& ctx, layout const& layout_spec = default_layout);
    void end();
    bool is_relevant() const { return is_relevant_; }
 private:
    ui_context* ctx_;
    scoped_routing_region srr_;
    column_layout layout_;
    bool is_relevant_;
};

#define alia_if_relevant_(ctx) \
    { \
        ::alia::culling_block alia__culling_block(ctx); \
        { \
            ::alia::pass_dependent_if_block alia__if_block( \
                get_data_traversal(ctx), alia__culling_block.is_relevant()); \
            if (alia__culling_block.is_relevant()) \
            {

#define alia_if_relevant alia_if_relevant_(ctx)

// UI CACHING

struct layout_node;
struct ui_caching_node;

struct cached_ui_block
{
    cached_ui_block() : ctx_(0) {}
    cached_ui_block(ui_context& ctx, id_interface const& id)
    { begin(ctx, id); }
    ~cached_ui_block() { end(); }
    void begin(ui_context& ctx, id_interface const& id);
    void end();
    bool is_relevant() const { return is_relevant_; }
 private:
    ui_context* ctx_;
    ui_caching_node* cacher_;
    culling_block culling_;
    bool is_relevant_;
    layout_node** layout_next_ptr_;
};

#define alia_cached_ui_block_(ctx, id) \
    { \
        ::alia::cached_ui_block alia__cached_ui_block(ctx, id); \
        { \
            ::alia::pass_dependent_if_block alia__if_block( \
                get_data_traversal(ctx), alia__cached_ui_block.is_relevant()); \
            if (alia__cached_ui_block.is_relevant()) \
            {

#define alia_cached_ui_block(id) alia_cached_ui_block_(ctx, id)

// UI CONTEXT

struct ui_system;
struct surface;
struct ui_event;

struct ui_context
{
    ui_system* system;
    data_traversal data;
    geometry_context geometry;
    layout_traversal layout;
    alia::surface* surface;
    ui_event* event;
    event_routing_traversal routing;
    ui_caching_node* active_cacher;
    style_state style;
    bool pass_aborted;
};

static inline data_traversal& get_data_traversal(ui_context& ctx)
{ return ctx.data; }
static inline layout_traversal& get_layout_traversal(ui_context& ctx)
{ return ctx.layout; }
static inline geometry_context& get_geometry_context(ui_context& ctx)
{ return ctx.geometry; }

void end_pass(ui_context& ctx);

struct untyped_ui_value
{
    virtual ~untyped_ui_value() {}
};

template<class T>
struct typed_ui_value : untyped_ui_value
{
    T value;
};

struct control_result
{
    bool changed;
    // allows use within if statements without other unintended conversions
    typedef bool control_result::* unspecified_bool_type;
    operator unspecified_bool_type() const
    { return changed ? &control_result::changed : 0; }
};

// COMMON FLAGS

struct ui_flag_tag {};
typedef flag_set<ui_flag_tag> ui_flag_set;
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000004, NO_FOCUS_INDICATOR)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000008, DISABLED)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000010, ROUNDED)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000020, HORIZONTAL)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000040, VERTICAL)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000080, COMMAND_LIST)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000100, FLIPPED)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000200, PREPEND)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000400, APPEND)
// expandables
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x001000, INITIALLY_EXPANDED)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x002000, INITIALLY_COLLAPSED)
// text controls
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x010000, PASSWORD)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x020000, SINGLE_LINE)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x040000, MULTILINE)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x080000, NO_PANEL)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x100000, ALWAYS_EDITING)
// panels
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x200000, NO_INTERNAL_PADDING)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x400000, NO_CLICK_DETECTION)

// DISPLAYS - non-interactive widgets

void do_separator(ui_context& ctx, layout const& layout_spec = default_layout);

void do_color(ui_context& ctx, getter<rgba8> const& color,
    layout const& layout_spec = default_layout);

void do_progress_bar(ui_context& ctx, getter<double> const& progress,
    layout const& layout_spec = default_layout);

void do_bullet(ui_context& ctx, layout const& layout_spec = default_layout);

struct bulleted_list : noncopyable
{
    bulleted_list() {}
    bulleted_list(ui_context& ctx, layout const& layout_spec = default_layout)
    { begin(ctx, layout_spec); }
    ~bulleted_list() { end(); }
    void begin(ui_context& ctx, layout const& layout_spec = default_layout);
    void end();
 private:
    friend struct bulleted_item;
    ui_context* ctx_;
    grid_layout grid_;
};

struct bulleted_item : noncopyable
{
    bulleted_item() {}
    bulleted_item(bulleted_list& list,
        layout const& layout_spec = default_layout)
    { begin(list, layout_spec); }
    ~bulleted_item() { end(); }
    void begin(bulleted_list& list,
        layout const& layout_spec = default_layout);
    void end();
 private:
    grid_row row_;
};

// TEXT DISPLAY

void do_text(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec = default_layout);

#define ALIA_DECLARE_STRING_CONVERSIONS(T) \
    bool from_string(T* value, string const& str, string* message); \
    string to_string(T value);

ALIA_DECLARE_STRING_CONVERSIONS(int)
ALIA_DECLARE_STRING_CONVERSIONS(unsigned)
ALIA_DECLARE_STRING_CONVERSIONS(float)
ALIA_DECLARE_STRING_CONVERSIONS(double)

struct cached_string_conversion
{
    cached_string_conversion() : valid(false) {}
    bool valid;
    owned_id id;
    string text;
};

struct cached_string_conversion_accessor : accessor<string>
{
    cached_string_conversion_accessor(cached_string_conversion* cache)
      : cache_(cache)
    {}
    bool is_gettable() const { return cache_->valid; }
    string const& get() const { return cache_->text; };
    id_interface const& id() const { return cache_->id.get(); }
    bool is_settable() const { return false; }
    void set(string const& value) const {}
 private:
    cached_string_conversion* cache_;
};

template<class T>
cached_string_conversion_accessor
as_text(ui_context& ctx, getter<T> const& value)
{
    cached_string_conversion* cache;
    get_cached_data(ctx, &cache);
    if (!cache->valid || !cache->id.matches(value.id()))
    {
        if (value.is_gettable())
        {
            cache->text = to_string(get(value));
            cache->valid = true;
        }
        else
            cache->valid = false;
        cache->id.store(value.id());
    }
    return cached_string_conversion_accessor(cache);
}

template<class T>
void do_text(ui_context& ctx, getter<T> const& value,
    layout const& layout_spec = default_layout)
{
    do_text(ctx, as_text(ctx, value), layout_spec);
}

cached_string_conversion_accessor
format_number(ui_context& ctx, char const* format,
    getter<double> const& number);

void do_paragraph(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec = default_layout);

void do_label(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec = default_layout);

bool do_link(
    ui_context& ctx,
    getter<string> const& text,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

void draw_text(ui_context& ctx, getter<string> const& text,
    vector<2,double> const& position);

void do_layout_dependent_text(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec);

// BUTTONS

// text button

typedef bool button_result;

button_result
do_button(
    ui_context& ctx,
    getter<string> const& text,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

// icon button

typedef bool icon_button_result;

enum icon_type
{
    REMOVE_ICON,
    EXPAND_ICON,
    SHRINK_ICON,
};

icon_button_result
do_icon_button(
    ui_context& ctx,
    icon_type icon,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

// CONTROLS

// check box

struct check_box_result : control_result {};

check_box_result
do_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

check_box_result
do_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    getter<string> const& text,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

// radio button

struct radio_button_result : control_result {};

radio_button_result
do_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

radio_button_result
do_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    getter<string> const& text,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

template<class Index>
struct indexed_accessor : regular_accessor<bool>
{
    indexed_accessor(
        accessor<Index> const& selected_value,
        getter<Index> const& this_value)
      : selected_value_(selected_value), this_value_(this_value)
    {}
    bool is_gettable() const
    { return selected_value_.is_gettable() && this_value_.is_gettable(); }
    bool const& get() const { return lazy_getter_.get(*this); }
    bool is_settable() const
    { return selected_value_.is_settable() && this_value_.is_gettable(); }
    void set(bool const& value) const
    { selected_value_.set(this_value_.get()); }
 private:
    friend struct lazy_getter<bool>;
    bool generate() const
    { return selected_value_.get() == this_value_.get(); }
    accessor<Index> const& selected_value_;
    getter<Index> const& this_value_;
    lazy_getter<bool> lazy_getter_;
};

template<class Index>
indexed_accessor<Index>
make_indexed_accessor(
    accessor<Index> const& selected_value,
    getter<Index> const& this_value)
{
    return indexed_accessor<Index>(selected_value, this_value);
}

template<class Index>
radio_button_result
do_radio_button(
    ui_context& ctx,
    accessor<Index> const& selected_value,
    getter<Index> const& this_value,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id)
{
    return do_radio_button(ctx,
        make_indexed_accessor(selected_value, this_value),
        layout_spec, id);
}

template<class Index>
radio_button_result
do_radio_button(
    ui_context& ctx,
    accessor<Index> const& selected_value,
    getter<Index> const& this_value,
    getter<string> const& text,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id)
{
    return do_radio_button(ctx,
        make_indexed_accessor(selected_value, this_value),
        text, layout_spec, id);
}

// node expander
struct node_expander_result : control_result {};
node_expander_result do_node_expander(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

// slider
// accepted flags:
// HORIZONTAL, VERTICAL (mutually exclusive, default is HORIZONTAL)
struct slider_result : control_result {};
slider_result
do_slider(ui_context& ctx, accessor<double> const& value,
    double minimum, double maximum, double step = 0,
    layout const& layout_spec = default_layout, ui_flag_set flags = NO_FLAGS);

// CONTAINERS

struct bordered_box : noncopyable
{
 public:
    bordered_box() {}
    // accepted flags: HORIZONTAL, VERTICAL - controls layout of children
    bordered_box(
        ui_context& ctx, layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS)
    { begin(ctx, layout_spec, flags); }
    ~bordered_box() { end(); }
    void begin(
        ui_context& ctx, layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS);
    void end();
    layout_box region() const { return box_.region(); }
 private:
    linear_layout box_;
};

struct panel : noncopyable
{
 public:
    panel() : ctx_(0) {}
    panel(
        ui_context& ctx, getter<string> const& style,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        widget_state state = WIDGET_NORMAL)
    { begin(ctx, style, layout_spec, flags, id, state); }
    ~panel() { end(); }
    void begin(
        ui_context& ctx, getter<string> const& style,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        widget_state state = WIDGET_NORMAL);
    void end();
    layout_box outer_region() const;
    layout_box inner_region() const { return inner_.region(); }
 private:
    ui_context* ctx_;
    column_layout outer_;
    scoped_substyle substyle_;
    linear_layout inner_;
    ui_flag_set flags_;
};

class clickable_panel : noncopyable
{
 public:
    clickable_panel() {}
    clickable_panel(
        ui_context& ctx, getter<string> const& style,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS, widget_id id = auto_id)
    { begin(ctx, style, layout_spec, flags, id); }
    void begin(
        ui_context& ctx, getter<string> const& style,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS, widget_id id = auto_id);
    void end() { panel_.end(); }
    layout_box outer_region() const { return panel_.outer_region(); }
    layout_box inner_region() const { return panel_.inner_region(); }
    bool clicked() const { return clicked_; }
 private:
    panel panel_;
    bool clicked_;
};

struct scrollable_layout_container;

struct scrollable_region : noncopyable
{
    scrollable_region() : ctx_(0) {}
    scrollable_region(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        unsigned scrollable_axes = 1 | 2,
        widget_id id = auto_id)
    { begin(ctx, layout_spec, scrollable_axes, id); }
    ~scrollable_region() { end(); }

    void begin(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        unsigned scrollable_axes = 1 | 2,
        widget_id id = auto_id);
    void end();

 private:
    ui_context* ctx_;
    scrollable_layout_container* container_;
    widget_id id_;
    scoped_clip_region scr_;
    scoped_transformation transform_;
    scoped_layout_container slc_;
    scoped_routing_region srr_;
};

struct scrollable_panel : noncopyable
{
 public:
    scrollable_panel() {}
    scrollable_panel(
        ui_context& ctx, getter<string> const& style,
        unsigned scrollable_axes = 1 | 2,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS)
    { begin(ctx, style, scrollable_axes, layout_spec, flags); }
    ~scrollable_panel() { end(); }
    void begin(
        ui_context& ctx, getter<string> const& style,
        unsigned scrollable_axes = 1 | 2,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS);
    void end();
 private:
    column_layout outer_;
    scoped_substyle substyle_;
    scrollable_region region_;
    linear_layout inner_;
};

struct tree_node : noncopyable
{
    tree_node() {}
    ~tree_node() { end(); }

    tree_node(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS,
        optional_storage<bool> const& expanded = optional_storage<bool>(none),
        widget_id expander_id = auto_id)
    { begin(ctx, layout_spec, flags, expanded, expander_id); }

    void begin(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS,
        optional_storage<bool> const& expanded = optional_storage<bool>(none),
        widget_id expander_id = auto_id);

    void end_header();

    bool do_children();

    node_expander_result const& expander_result() const
    { return expander_result_; }

    void end();

 private:
    ui_context* ctx_;

    grid_layout grid_;
    grid_row row_;
    column_layout column_;
    row_layout label_region_;

    bool do_children_;
    node_expander_result expander_result_;
};

// OVERLAYS

struct overlay
{
    overlay() : ctx_(0) {}
    overlay(ui_context& ctx, widget_id id) { begin(ctx, id); }        
    ~overlay() { end(); }
    void begin(ui_context& ctx, widget_id id);
    void end();
 private:
    ui_context* ctx_;
    ui_event_category real_event_category_;
    ui_event_type real_event_type_;
};

struct popup_positioning
{
    layout_vector lower_bound;
    layout_vector upper_bound;
    layout_vector absolute_lower;
    layout_vector absolute_upper;
};

struct popup
{
    popup() : ctx_(0) {}
    popup(ui_context& ctx, widget_id id,
        popup_positioning const& positioning)
    { begin(ctx, id, positioning); }
    ~popup() { end(); }
    void begin(ui_context& ctx, widget_id id,
        popup_positioning const& positioning);
    void end();
 private:
    ui_context* ctx_;
    widget_id id_, background_id_;
    floating_layout layout_;
    scoped_transformation transform_;
    overlay overlay_;
};

// DROP DOWNS

struct ddl_data;

struct untyped_drop_down_list : noncopyable
{
 public:
    untyped_drop_down_list() : ctx_(0) {}
    ~untyped_drop_down_list() { end(); }

    untyped_ui_value const*
    begin(ui_context& ctx, layout const& layout_spec, ui_flag_set flags);
    void end();

    bool do_list();

 private:
    friend struct untyped_ddl_item;

    ui_context* ctx_;
    ddl_data* data_;
    widget_id id_;
    panel container_;
    row_layout contents_;

    popup popup_;
    bordered_box list_border_;
    scrollable_panel list_panel_;
    int list_index_;
};

template<class Index>
struct drop_down_list : noncopyable
{
 public:
    drop_down_list() : changed_(false) {}
    drop_down_list(ui_context& ctx, accessor<Index> const& selection,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS)
    { begin(ctx, selection, layout_spec, flags); }
    ~drop_down_list() { end(); }

    void begin(ui_context& ctx, accessor<Index> const& selection,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS)
    {
        if (selection.is_gettable())
            selection_ = get(selection);
        else
            selection_ = none;

        untyped_ui_value const* new_value =
            list_.begin(ctx, layout_spec, flags);
        if (new_value)
        {
            typed_ui_value<Index> const* v =
                dynamic_cast<typed_ui_value<Index> const*>(new_value);
            // This should only fail if somehow an event with the wrong value
            // type is somehow sent to this widget.
            assert(v);
            if (v)
            {
                set(selection, v->value);
                changed_ = true;
            }
        }
    }
    void end() { list_.end(); }

    bool do_list() { return list_.do_list(); }

    bool changed() const { return changed_; }

 private:
    template<class Index>
    friend struct ddl_item;

    untyped_drop_down_list list_;
    optional<Index> selection_;
    bool changed_;
};

struct untyped_ddl_item : noncopyable
{
 public:
    untyped_ddl_item() : list_(0) {}
    ~untyped_ddl_item() { end(); }
    bool begin(untyped_drop_down_list& list, bool is_selected);
    void end();
    void select(untyped_ui_value* value);
    bool is_selected() const { return selected_; }
 private:
    untyped_drop_down_list* list_;
    bool selected_;
    panel panel_;
};

template<class Index>
struct ddl_item : noncopyable
{
 public:
    ddl_item() {}
    ddl_item(drop_down_list<Index>& list, Index const& index)
    { begin(list, index); }
    ~ddl_item() { end(); }

    void begin(drop_down_list<Index>& list, Index const& index)
    {
        if (item_.begin(list.list_,
            list.selection_ ? get(list.selection_) == index : false))
        {
            typed_ui_value<Index>* v = new typed_ui_value<Index>;
            v->value = index;
            item_.select(v);
        }
    }
    void end() { item_.end(); }

    bool is_selected() const { return item_.is_selected(); }

 private:
    untyped_ddl_item item_;
    bool selected_;
};

// TEXT CONTROL

enum text_control_event_type
{
    TEXT_CONTROL_NO_EVENT,
    TEXT_CONTROL_ENTER_PRESSED,
    TEXT_CONTROL_FOCUS_LOST,
    TEXT_CONTROL_INVALID_VALUE,
    TEXT_CONTROL_EDIT_CANCELED,
};

struct text_control_result : control_result
{
    text_control_event_type event;
};

text_control_result
do_text_control(
    ui_context& ctx,
    accessor<string> const& value,
    layout const& layout_spec = default_layout,
    ui_flag_set flags = NO_FLAGS,
    widget_id id = auto_id,
    optional<size_t> const& length_limit = none);

struct text_control_string_conversion
{
    text_control_string_conversion() : valid(false) {}
    bool valid;
    owned_id id;
    string text;
    // associated error message (if text doesn't parse)
    string message;
};

template<class T>
text_control_result
do_text_control(
    ui_context& ctx,
    accessor<T> const& accessor,
    layout const& layout_spec = default_layout,
    ui_flag_set flags = NO_FLAGS,
    widget_id id = auto_id,
    optional<size_t> const& length_limit = none)
{
    layout spec = add_default_alignment(layout_spec, LEFT, BASELINE_Y);
    column_layout c(ctx, spec);

    text_control_string_conversion* data;
    get_data(ctx, &data);
    if (ctx.event->type == REFRESH_EVENT)
    {
        bool valid = accessor.is_gettable();
        if (data->valid != valid || valid && !data->id.matches(accessor.id()))
        {
            // The external value has changed.
            data->valid = valid;
            data->text = valid ? to_string(get(accessor)) : "";
            data->message = "";
            data->id.store(accessor.id());
        }
    }

    text_control_result r = do_text_control(
        ctx, inout(&data->text), spec, flags, id, length_limit);
    alia_if(!data->message.empty())
    {
        do_paragraph(ctx, in(data->message));
    }
    alia_end

    text_control_result result;
    switch (r.event)
    {
     case TEXT_CONTROL_FOCUS_LOST:
     case TEXT_CONTROL_ENTER_PRESSED:
      {
        T new_value;
        if (from_string(&new_value, data->text, &data->message))
        {
            data->message = "";
            result.event = r.event;
            result.changed = true;
            accessor.set(new_value);
        }
        else
        {
            result.event = TEXT_CONTROL_INVALID_VALUE;
            result.changed = false;
        }
        break;
      }
     case TEXT_CONTROL_EDIT_CANCELED:
        result.event = TEXT_CONTROL_EDIT_CANCELED;
        result.changed = false;
        data->text = accessor.is_gettable() ? to_string(get(accessor)) : "";
        data->message = "";
        break;
     case TEXT_CONTROL_NO_EVENT:
     default:
        result.event = TEXT_CONTROL_NO_EVENT;
        result.changed = false;
        break;
    }
    return result;
}

bool do_draggable_separator(ui_context& ctx, accessor<int> const& width,
    layout const& layout_spec = default_layout, ui_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

// resizable_content is a container with a draggable separator for controlling
// the size of its contents.
// accepted flags:
// HORIZONTAL, VERTICAL - the orientation of the separator (default: VERTICAL)
// PREPEND, APPEND - is the separator appended or preprended (default: APPEND)
struct resizable_content : noncopyable
{
    resizable_content() {}
    resizable_content(ui_context& ctx, accessor<int> const& size,
        ui_flag_set flags = NO_FLAGS)
    { begin(ctx, size, flags); }
    ~resizable_content() { end(); }
    void begin(ui_context& ctx, accessor<int> const& size,
        ui_flag_set flags = NO_FLAGS);
    void end();
 private:
    ui_context* ctx_;
    widget_id id_;
    ui_flag_set flags_;
    int size_;
    linear_layout layout_;
};

}

#endif
