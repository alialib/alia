#ifndef ALIA_UI_API_HPP
#define ALIA_UI_API_HPP

#include <cstdio>
#include <vector>

#include <boost/function.hpp>

#include <alia/accessors.hpp>
#include <alia/actions.hpp>
#include <alia/color.hpp>
#include <alia/event_routing.hpp>
#include <alia/layout/api.hpp>

// This file declares all the types and functions necessary to use the UI
// library from the application end, including a standard library of widgets
// and containers.

// The implementations of the functions declared in here are spread across the
// source files in the ui/library subdirectory.

namespace alia {

struct dataless_ui_context;
struct ui_context;

typedef void const* widget_id;
widget_id static const auto_id = 0;

// Often, widgets with internal storage will want to give the application the
// option of providing their own storage for that data. (This is useful if the
// application wants to persist that storage or needs to manipulate it directly
// in reponse to other user actions.)
// In those cases, the widget can accept an optional_storage<T> argument.
// The application can then call storage(accessor) to provide an accessor for
// the widget's storage, or it can pass in 'none' to opt out of providing
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
    accessor_storage(accessor_storage&& other)
    {
        accessor_ = std::move(other.accessor_);
        this->storage = &accessor_;
    }
    accessor_storage& operator=(accessor_storage&& other)
    {
        accessor_ = std::move(other.accessor_);
        this->storage = &accessor_;
        return *this;
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
accessor_mux<input_accessor<bool>,indirect_accessor<T>,inout_accessor<T> >
resolve_storage(optional_storage<T> const& s, T* fallback)
{
    return select_accessor(in(s.storage != 0), ref(s.storage),
        inout(fallback));
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
    KEY_TAB                = 9,
    KEY_CLEAR                = 12,
    KEY_ENTER                = 13,
    KEY_PAUSE                = 19,
    KEY_ESCAPE                = 27,
    KEY_SPACE                = 32,
    KEY_EXCLAIM                = 33,
    KEY_QUOTEDBL        = 34,
    KEY_HASH                = 35,
    KEY_DOLLAR                = 36,
    KEY_AMPERSAND        = 38,
    KEY_QUOTE                = 39,
    KEY_LEFTPAREN        = 40,
    KEY_RIGHTPAREN        = 41,
    KEY_ASTERISK        = 42,
    KEY_PLUS                = 43,
    KEY_COMMA                = 44,
    KEY_MINUS                = 45,
    KEY_PERIOD                = 46,
    KEY_SLASH                = 47,
    KEY_COLON                = 58,
    KEY_SEMICOLON        = 59,
    KEY_LESS                = 60,
    KEY_EQUALS                = 61,
    KEY_GREATER                = 62,
    KEY_QUESTION        = 63,
    KEY_AT                = 64,
    // no uppercase letters
    KEY_LEFTBRACKET        = 91,
    KEY_BACKSLASH        = 92,
    KEY_RIGHTBRACKET        = 93,
    KEY_CARET                = 94,
    KEY_UNDERSCORE        = 95,
    KEY_BACKQUOTE        = 96,
    KEY_DELETE                = 127,

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
ALIA_DEFINE_FLAG(kmod, 0x00, KMOD_NONE)
ALIA_DEFINE_FLAG(kmod, 0x01, KMOD_SHIFT)
ALIA_DEFINE_FLAG(kmod, 0x02, KMOD_CTRL)
ALIA_DEFINE_FLAG(kmod, 0x04, KMOD_ALT)
ALIA_DEFINE_FLAG(kmod, 0x08, KMOD_WIN)
ALIA_DEFINE_FLAG(kmod, 0x10, KMOD_META)

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
    OPEN_HAND_CURSOR,
    POINTING_HAND_CURSOR,
    LEFT_RIGHT_ARROW_CURSOR,
    UP_DOWN_ARROW_CURSOR,
    FOUR_WAY_ARROW_CURSOR,
};

// general categories of UI events recognized by alia
// (This can be useful as a primary dispatching criteria in widget
// implementations.)
enum ui_event_category
{
    NO_CATEGORY,
    REFRESH_CATEGORY,
    REGION_CATEGORY,
    INPUT_CATEGORY,
    RENDER_CATEGORY,
    OVERLAY_CATEGORY,
};

// UI events recognized by alia
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
    MOUSE_HOVER_EVENT,

    // overlays
    OVERLAY_MOUSE_HIT_TEST_EVENT,
    OVERLAY_WHEEL_HIT_TEST_EVENT,
    OVERLAY_RENDER_EVENT,
    OVERLAY_MAKE_WIDGET_VISIBLE_EVENT,

    // uncategorized events

    WRAPPED_EVENT,
    SET_VALUE_EVENT,
    TIMER_EVENT,
    RESOLVE_LOCATION_EVENT,
    SHUTDOWN_EVENT,
    CUSTOM_EVENT,
};

// ui_controller is the interface for the application-supplied controller
// that specifies the actual content of the UI.
struct ui_controller : noncopyable
{
    virtual ~ui_controller() {}

    virtual void do_ui(ui_context& ctx) = 0;
};

// UI STYLING

// widget state flags
struct widget_state_flag_tag {};
typedef flag_set<widget_state_flag_tag> widget_state;
// primary state
ALIA_DEFINE_FLAG(widget_state, 0x01, WIDGET_NORMAL)
ALIA_DEFINE_FLAG(widget_state, 0x02, WIDGET_DISABLED)
ALIA_DEFINE_FLAG(widget_state, 0x03, WIDGET_HOT)
ALIA_DEFINE_FLAG(widget_state, 0x04, WIDGET_DEPRESSED)
ALIA_DEFINE_FLAG(widget_state, 0x05, WIDGET_SELECTED)
ALIA_DEFINE_FLAG(widget_state, 0x0f, WIDGET_PRIMARY_STATE_MASK)
// additional (independent) states
ALIA_DEFINE_FLAG(widget_state, 0x10, WIDGET_FOCUSED)

struct style_search_path;
struct dispatch_table;
struct primary_style_properties;

// style_state defines the style-related state that's maintained during
// a UI traversal. (It is a subcomponent of the ui_context structure.)
struct style_state
{
    // the current search path for style properties
    style_search_path const* path;
    // the current theme (which provides widget renderers)
    dispatch_table const* theme;
    // the 'primary' style properties
    // (what's required for rendering simple text)
    primary_style_properties const* properties;
    // a unique ID for the current value of the style state
    id_interface const* id;
};

// The layout system also requires some information about the current style.
struct layout_style_info;

// scoped_style is a scoped object that activates a new style for a UI context
// within its scope.
// (Since the style is specified explicitly as a state structure, this is not
// meant to be used directly in user code, but rather as a utility for more
// convenient forms like scoped_substyle.)
struct scoped_style : noncopyable
{
    scoped_style() : ctx_(0) {}
    scoped_style(dataless_ui_context& ctx, style_state const& style,
        layout_style_info const* info)
    { begin(ctx, style, info); }
    ~scoped_style() { end(); }

    void begin(dataless_ui_context& ctx, style_state const& style,
        layout_style_info const* info);
    void end();

 private:
    dataless_ui_context* ctx_;
    style_state old_state_;
    layout_style_info const* old_style_info_;
};

// scoped_substyle is similar to scoped_style in that it activates a new style
// for a UI context within its scope. However, the new style is specified as a
// simple string. The string is looked up in the current search path, and the
// associated style is loaded and activated.
// You can optionally specify a widget state, in which case the search will
// first look to see if it can match both the name and the state.
ALIA_DEFINE_FLAG_TYPE(scoped_substyle)
ALIA_DEFINE_FLAG(scoped_substyle, 0x1, SCOPED_SUBSTYLE_NO_PATH_SEPARATOR)
struct scoped_substyle : noncopyable
{
    scoped_substyle() {}
    scoped_substyle(ui_context& ctx, accessor<string> const& substyle_name,
        widget_state state = WIDGET_NORMAL,
        scoped_substyle_flag_set flags = NO_FLAGS)
    { begin(ctx, substyle_name, state, flags); }
    ~scoped_substyle() { end(); }

    void begin(ui_context& ctx, accessor<string> const& substyle_name,
        widget_state state = WIDGET_NORMAL,
        scoped_substyle_flag_set flags = NO_FLAGS);
    void end();

 private:
    scoped_style scoping_;
};

// ANIMATION

// The following are interpolation curves that can be used for animations.
typedef unit_cubic_bezier animation_curve;
animation_curve static const default_curve(0.25, 0.1, 0.25, 1);
animation_curve static const linear_curve(0, 0, 1, 1);
animation_curve static const ease_in_curve(0.42, 0, 1, 1);
animation_curve static const ease_out_curve(0, 0, 0.58, 1);
animation_curve static const ease_in_out_curve(0.42, 0, 0.58, 1);

// animated_transition specifies an animated transition from one state to
// another, defined by a duration and a curve to follow.
struct animated_transition
{
    animation_curve curve;
    ui_time_type duration;

    animated_transition() {}
    animated_transition(animation_curve const& curve, ui_time_type duration)
      : curve(curve), duration(duration)
    {}
};
animated_transition static const default_transition(default_curve, 400);

// VALIDATION

struct validation_error_reporting_context;
struct validation_error_detection_context;

struct validation_context
{
    validation_error_reporting_context* reporting;
    validation_error_detection_context* detection;
};

// CULLING

// A culling_block is an optimization.

struct culling_block
{
    culling_block() {}
    culling_block(ui_context& ctx, layout const& layout_spec = default_layout)
    { begin(ctx, layout_spec); }
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

#define alia_culling_block_(ctx, layout_spec) \
    { \
        ::alia::culling_block alia__culling_block(ctx, layout_spec); \
        { \
            ::alia::pass_dependent_if_block alia__if_block( \
                get_data_traversal(ctx), alia__culling_block.is_relevant()); \
            if (alia__culling_block.is_relevant()) \
            {

#define alia_culling_block(layout_spec) alia_culling_block_(ctx, layout_spec)

// UI CACHING

struct layout_node;
struct ui_caching_node;

struct cached_ui_block
{
    cached_ui_block() : ctx_(0) {}
    cached_ui_block(ui_context& ctx, id_interface const& id,
        layout const& layout_spec = default_layout)
    { begin(ctx, id, layout_spec); }
    ~cached_ui_block() { end(); }
    void begin(ui_context& ctx, id_interface const& id,
        layout const& layout_spec = default_layout);
    void end();
    bool is_relevant() const { return is_relevant_; }
 private:
    ui_context* ctx_;
    ui_caching_node* cacher_;
    culling_block culling_;
    bool is_relevant_;
    layout_node** layout_next_ptr_;
};

#define alia_cached_ui_block_(ctx, id, layout_spec) \
    { \
        ::alia::cached_ui_block alia__cached_ui_block(ctx, id); \
        { \
            ::alia::pass_dependent_if_block alia__if_block( \
                get_data_traversal(ctx), alia__cached_ui_block.is_relevant());\
            if (alia__cached_ui_block.is_relevant()) \
            {

#define alia_cached_ui_block(id, layout_spec) \
    alia_cached_ui_block_(ctx, id, layout_spec)

// UI CONTEXT

struct ui_system;
struct surface;
struct ui_event;
struct mouse_hover_context;
struct menu_node;
struct menu_container;

struct menu_context
{
    // pointer to where the next child should be attached
    menu_node** next_ptr;

    menu_container* active_container;
};

// dataless_ui_context defines everything about a UI context except for the
// data traversal. It's impossible to retrieve data from a dataless UI context
// (insofar as anything in C++ is impossible), and thus one can safely call
// functions that expect a dataless_ui_context even in areas where control
// flow is not properly tracked.
struct dataless_ui_context
{
    ui_system* system;
    geometry_context* geometry;
    layout_traversal* layout;
    alia::surface* surface;
    ui_event* event;
    event_routing_traversal routing;
    ui_caching_node* active_cacher;
    style_state style;
    bool pass_aborted;
    mouse_hover_context* hover;
    validation_context validation;
    menu_context menu;
};

struct ui_context : dataless_ui_context
{
    data_traversal* data;
};

static inline data_traversal&
get_data_traversal(ui_context& ctx)
{ return *ctx.data; }
static inline layout_traversal&
get_layout_traversal(dataless_ui_context& ctx)
{ return *ctx.layout; }
static inline geometry_context&
get_geometry_context(dataless_ui_context& ctx)
{ return *ctx.geometry; }

// UTILITIES - Various utilities that are considered part of the core API or
// must be visible for other reasons.

// The following macros are substitutes for normal C++ control flow statements.
// Unlike alia_if and company, they do NOT track control flow. Instead, they
// upcast your UI context variable to a dataless_ui_context within the block.
// This means that any attempt to retrieve data within the block will result
// in an error (as it should).

#define ALIA_REMOVE_DATA_TRACKING \
    ::alia::dataless_ui_context& alia__ctx = ctx; \
    ::alia::dataless_ui_context& ctx = alia__ctx;

#define alia_untracked_if(condition) \
    if (alia::is_true(condition)) \
    { \
        ALIA_REMOVE_DATA_TRACKING \
        {{

#define alia_untracked_else_if(condition) \
    }}} \
    else if (alia::is_true(condition)) \
    { \
        ALIA_REMOVE_DATA_TRACKING \
        {{

#define alia_untracked_else \
    }}} \
    else \
    { \
        ALIA_REMOVE_DATA_TRACKING \
        {{

#define alia_untracked_switch(expression) \
    switch (expression) \
    {{{

#define alia_untracked_case(c) \
        }}} \
        case c: \
        {{{ \
            ALIA_REMOVE_DATA_TRACKING \
            goto ALIA_CONCATENATE(alia__dummy_label_, __LINE__); \
            ALIA_CONCATENATE(alia__dummy_label_, __LINE__)

#define alia_untracked_default \
        }}} \
        default: \
        {{{ \
            ALIA_REMOVE_DATA_TRACKING \
            goto ALIA_CONCATENATE(alia__dummy_label_, __LINE__); \
            ALIA_CONCATENATE(alia__dummy_label_, __LINE__)

// alia_scoped_data_block does the opposite of the above.
// It transitions from untracked control flow back to tracked control flow.
// In order to do this, you must supply a data_block to use for tracking and
// data retrieval.
#define alia_tracked_block(block) \
    { \
        ::alia::ui_context& alia__ctx = static_cast<::alia::ui_context&>(ctx);\
        ::alia::ui_context& ctx = alia__ctx; \
        ::alia::scoped_data_block alia__block(ctx, (block)); \
        {{

// Calling end_pass(ctx) will immediately end the currently traversal over
// your UI. This may be necessary if an event has triggered a change in state
// that invalidates the current traversal. (For example, if clicking a button
// removed an item from a list, and you're in the middle of traversing that
// list.) It's implemented by throwing an exception which is caught outside
// the traversal.
void end_pass(dataless_ui_context& ctx);

struct untyped_ui_value
{
    virtual ~untyped_ui_value() {}
};

template<class T>
struct typed_ui_value : untyped_ui_value
{
    T value;
};

// erase_type(ctx, x) stores a copy of x within the data graph of ctx and
// returns a pointer to that copy.
// The idea here is that you can then return that pointer to a calling
// function without that function caring what the concrete type of x is.
// This is useful when implementing functions that return accessors since
// you can declare the function type as simply returning a an
// indirect_accessor, hiding the details of how that accessor is created.
// Of course, when using this, you must ensure that x doesn't contain any
// references that might be invalid when the stored copy is accessed.
template<class T>
T* erase_type(ui_context& ctx, T const& x)
{
    T* storage;
    get_cached_data(ctx, &storage);
    *storage = x;
    return storage;
}

// make_indirect(ctx, x) takes an accessor x, erases its type, and then
// returns an indirect_accessor that refers to it.
template<class Accessor>
indirect_accessor<typename Accessor::value_type>
make_indirect(ui_context& ctx, Accessor const& x)
{ return alia::ref(erase_type(ctx, x)); }
// Don't bother if it's already indirect.
template<class Value>
indirect_accessor<Value>
make_indirect(ui_context& ctx, indirect_accessor<Value> const& x)
{ return x; }

struct control_result
{
    bool changed;
    // allows use within if statements without other unintended conversions
    typedef bool control_result::* unspecified_bool_type;
    operator unspecified_bool_type() const
    { return changed ? &control_result::changed : 0; }
};

// Mark a location in the UI.
void mark_location(ui_context& ctx, id_interface const& id,
    layout_vector const& position = make_layout_vector(0, 0));

// Call this to request that the UI jump to a marked location.
ALIA_DEFINE_FLAG_TYPE(jump_to_location)
ALIA_DEFINE_FLAG(jump_to_location, 0x1, JUMP_TO_LOCATION_ABRUPTLY)
void jump_to_location(dataless_ui_context& ctx, id_interface const& id,
    jump_to_location_flag_set flags = NO_FLAGS);

// DISPLAYS - non-interactive widgets

void do_separator(ui_context& ctx, layout const& layout_spec = default_layout);

void do_color(ui_context& ctx, accessor<rgba8> const& color,
    layout const& layout_spec = default_layout);
void do_color(ui_context& ctx, accessor<rgb8> const& color,
    layout const& layout_spec = default_layout);

void do_progress_bar(ui_context& ctx, accessor<double> const& progress,
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

// TEXT CONVERSION

// All conversion of values to and from text goes through the functions
// from_string and to_string. In order to use a particular value type with
// the text-based widgets and utilities provided here, that type must
// implement these functions.

#define ALIA_DECLARE_STRING_CONVERSIONS(T) \
    void from_string(T* value, string const& s); \
    string to_string(T value);

// from_string(value, s) should parse the string s and store it in *value.
// It should throw a validation_error if the string doesn't parse.

// to_string(value) should simply return the string form of value.

// Implementations of from_string and to_string are provided for the following
// types.

ALIA_DECLARE_STRING_CONVERSIONS(int)
ALIA_DECLARE_STRING_CONVERSIONS(unsigned)
ALIA_DECLARE_STRING_CONVERSIONS(float)
ALIA_DECLARE_STRING_CONVERSIONS(double)
ALIA_DECLARE_STRING_CONVERSIONS(size_t)

// as_text(ctx, x) creates a text-based interface to the accessor x.
template<class T>
void update_text_conversion(keyed_data<string>* data, accessor<T> const& x)
{
    if (is_gettable(x))
    {
        refresh_keyed_data(*data, x.id());
        if (!is_valid(*data))
            set(*data, to_string(get(x)));
    }
    else
        invalidate(*data);
}
template<class T>
keyed_data_accessor<string>
as_text(ui_context& ctx, accessor<T> const& x)
{
    keyed_data<string>* data;
    get_cached_data(ctx, &data);
    update_text_conversion(data, x);
    return keyed_data_accessor<string>(data);
}

// as_settable_text(ctx, x) is similar to as_text but it works with full
// accessors and provides setting capabilities as well.
template<class Wrapped>
struct settable_text_accessor : accessor<string>
{
    settable_text_accessor(Wrapped wrapped, keyed_data<string>* data)
      : wrapped_(wrapped), data_(data)
    {}
    bool is_gettable() const { return is_valid(*data_); }
    string const& get() const { return alia::get(*data_); }
    alia__shared_ptr<string> get_ptr() const
    { return alia__shared_ptr<string>(new string(this->get())); }
    id_interface const& id() const { return wrapped_.id(); }
    bool is_settable() const { return wrapped_.is_settable(); }
    void set(string const& s) const
    {
        typename accessor_value_type<Wrapped>::type value;
        from_string(&value, s);
        wrapped_.set(value);
    }
 private:
    keyed_data<string>* data_;
    Wrapped wrapped_;
};
template<class Accessor>
settable_text_accessor<Accessor>
as_settable_text(ui_context& ctx, Accessor const& x)
{
    keyed_data<string>* data;
    get_cached_data(ctx, &data);
    update_text_conversion(data, x);
    return settable_text_accessor<Accessor>(x, data);
}

#ifdef _MSC_VER
int c99_snprintf(char* str, size_t size, const char* format, ...);
int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap);
#define ALIA_SNPRINTF alia::c99_snprintf
#else
#define ALIA_SNPRINTF std::snprintf
#endif

template<class Value>
Value
make_printf_friendly(Value x)
{ return x; }

static inline char const*
make_printf_friendly(string const& x)
{ return x.c_str(); }

template<class Arg0>
keyed_data_accessor<string>
printf(ui_context& ctx, char const* format, accessor<Arg0> const& arg0)
{
    keyed_data_accessor<string> cache;
    get_keyed_data(ctx, arg0.id(), &cache);
    if (!cache.is_gettable() && arg0.is_gettable())
    {
        int size = ALIA_SNPRINTF(0, 0, format,
            make_printf_friendly(get(arg0)));
        if (size >= 0)
        {
            string s;
            if (size > 0)
            {
                std::vector<char> chars(size + 1);
                ALIA_SNPRINTF(&chars[0], size + 1, format,
                    make_printf_friendly(get(arg0)));
                s = string(&chars[0]);
            }
            cache.set(s);
        }
    }
    return cache;
}

template<class Arg0, class Arg1>
keyed_data_accessor<string>
printf(ui_context& ctx, char const* format, accessor<Arg0> const& arg0,
    accessor<Arg1> const& arg1)
{
    keyed_data_accessor<string> cache;
    get_keyed_data(ctx, combine_ids(ref(&arg0.id()), ref(&arg1.id())), &cache);
    if (!cache.is_gettable() && arg0.is_gettable() && arg1.is_gettable())
    {
        int size = ALIA_SNPRINTF(0, 0, format,
            make_printf_friendly(get(arg0)), make_printf_friendly(get(arg1)));
        if (size >= 0)
        {
            string s;
            if (size > 0)
            {
                std::vector<char> chars(size + 1);
                ALIA_SNPRINTF(&chars[0], size + 1, format,
                    make_printf_friendly(get(arg0)),
                    make_printf_friendly(get(arg1)));
                s = string(&chars[0]);
            }
            cache.set(s);
        }
    }
    return cache;
}

template<class Arg0, class Arg1, class Arg2>
keyed_data_accessor<string>
printf(ui_context& ctx, char const* format, accessor<Arg0> const& arg0,
    accessor<Arg1> const& arg1, accessor<Arg2> const& arg2)
{
    keyed_data_accessor<string> cache;
    get_keyed_data(ctx, combine_ids(ref(&arg0.id()),
        combine_ids(ref(&arg1.id()), ref(&arg2.id()))), &cache);
    if (!cache.is_gettable() && arg0.is_gettable() && arg1.is_gettable() &&
        arg2.is_gettable())
    {
        int size = ALIA_SNPRINTF(0, 0, format,
            make_printf_friendly(get(arg0)), make_printf_friendly(get(arg1)),
            make_printf_friendly(get(arg2)));
        if (size >= 0)
        {
            string s;
            if (size > 0)
            {
                std::vector<char> chars(size + 1);
                ALIA_SNPRINTF(&chars[0], size + 1, format,
                    make_printf_friendly(get(arg0)),
                    make_printf_friendly(get(arg1)),
                    make_printf_friendly(get(arg2)));
                s = string(&chars[0]);
            }
            cache.set(s);
        }
    }
    return cache;
}

template<class Arg0, class Arg1, class Arg2, class Arg3>
keyed_data_accessor<string>
printf(ui_context& ctx, char const* format, accessor<Arg0> const& arg0,
    accessor<Arg1> const& arg1, accessor<Arg2> const& arg2, accessor<Arg3> const& arg3)
{
    keyed_data_accessor<string> cache;
    get_keyed_data(ctx, combine_ids(ref(&arg0.id()), combine_ids(ref(&arg1.id()),
        combine_ids(ref(&arg2.id()), ref(&arg3.id())))), &cache);
    if (!cache.is_gettable() && arg0.is_gettable() && arg1.is_gettable() &&
        arg2.is_gettable() && arg3.is_gettable())
    {
        int size = ALIA_SNPRINTF(0, 0, format,
            make_printf_friendly(get(arg0)), make_printf_friendly(get(arg1)),
            make_printf_friendly(get(arg2)), make_printf_friendly(get(arg3)));
        if (size >= 0)
        {
            string s;
            if (size > 0)
            {
                std::vector<char> chars(size + 1);
                ALIA_SNPRINTF(&chars[0], size + 1, format,
                    make_printf_friendly(get(arg0)),
                    make_printf_friendly(get(arg1)),
                    make_printf_friendly(get(arg2)),
                    make_printf_friendly(get(arg3)));
                s = string(&chars[0]);
            }
            cache.set(s);
        }
    }
    return cache;
}

// TEXT DISPLAY

// Do a string of text. If called inside a flow_layout, the text will wrap.
// Otherwise, it will request as much space as it needs to fit unwrapped.
void do_text(ui_context& ctx, accessor<string> const& text,
    layout const& layout_spec = default_layout);
// same, but will take a value and convert it to text
template<class T>
void do_text(ui_context& ctx, accessor<T> const& value,
    layout const& layout_spec = default_layout)
{
    do_text(ctx, as_text(ctx, value), layout_spec);
}

// Do a flow_layout with the given text inside it.
void do_flow_text(ui_context& ctx, accessor<string> const& text,
    layout const& layout_spec = default_layout);
// same, but will take a value and convert it to text
template<class T>
void do_flow_text(ui_context& ctx, accessor<T> const& value,
    layout const& layout_spec = default_layout)
{
    do_flow_text(ctx, as_text(ctx, value), layout_spec);
}

// This is here for backwards-compatibility.
void static
do_paragraph(ui_context& ctx, accessor<string> const& text,
    layout const& layout_spec = default_layout)
{
    do_flow_text(ctx, text, layout_spec);
}

// Do a text display will never wrap.
void do_label(ui_context& ctx, accessor<string> const& text,
    layout const& layout_spec = default_layout);

ALIA_DEFINE_FLAG_TYPE(ui_text_drawing)
ALIA_DEFINE_FLAG(ui_text_drawing, 0x00, ALIGN_TEXT_BASELINE)
ALIA_DEFINE_FLAG(ui_text_drawing, 0x01, ALIGN_TEXT_TOP)

// Draw text at a given position in the UI.
// (This doesn't do any layout.)
void draw_text(ui_context& ctx, accessor<string> const& text,
    vector<2,double> const& position,
    ui_text_drawing_flag_set flags = NO_FLAGS);

// Do the given text inside the given substyle.
void do_styled_text(ui_context& ctx, accessor<string> const& substyle_name,
    accessor<string> const& text, layout const& layout_spec = default_layout);

// do_heading is the same as do_styled_text but it also accepts a margin
// specification as part of its style so that space can be added around the
// text.
void do_heading(ui_context& ctx, accessor<string> const& substyle_name,
    accessor<string> const& text, layout const& layout_spec = default_layout);

// make_text(x, id), where x is a utf8_string, creates a read-only accessor
// for accessing x as a string. It uses the given ID to identify x.
template<class Id>
struct utf8_string_accessor : accessor<string>
{
    utf8_string_accessor(utf8_string const& x, Id const& id)
      : text_(x), id_(id)
    {}
    id_interface const& id() const { return id_; }
    bool is_gettable() const { return true; }
    string const& get() const { return lazy_getter_.get(*this); }
    bool is_settable() const { return false; }
    void set(string const& value) const {}
 private:
    utf8_string text_;
    Id id_;
    friend struct lazy_getter<string>;
    string generate() const
    { return string(text_.begin, text_.end - text_.begin); }
    lazy_getter<string> lazy_getter_;
};
template<class Id>
utf8_string_accessor<Id>
make_text(utf8_string const& x, Id const& id)
{ return utf8_string_accessor<Id>(x, id); }

// TEXT CONTROL

enum text_control_event_type
{
    TEXT_CONTROL_NO_EVENT,
    TEXT_CONTROL_ENTER_PRESSED,
    TEXT_CONTROL_FOCUS_LOST,
    TEXT_CONTROL_EDIT_CANCELED,
};

struct text_control_result : control_result
{
    text_control_event_type event;
};

ALIA_DEFINE_FLAG_TYPE(text_control)
ALIA_DEFINE_FLAG(text_control, 0x01, TEXT_CONTROL_DISABLED)
ALIA_DEFINE_FLAG(text_control, 0x02, TEXT_CONTROL_MASK_CONTENTS)
ALIA_DEFINE_FLAG(text_control, 0x04, TEXT_CONTROL_SINGLE_LINE)
ALIA_DEFINE_FLAG(text_control, 0x08, TEXT_CONTROL_MULTILINE)
ALIA_DEFINE_FLAG(text_control, 0x10, TEXT_CONTROL_IMMEDIATE)

text_control_result
do_unsafe_text_control(
    ui_context& ctx,
    accessor<string> const& value,
    layout const& layout_spec = default_layout,
    text_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id,
    optional<size_t> const& length_limit = none);

void static inline
do_text_control(
    ui_context& ctx,
    accessor<string> const& value,
    layout const& layout_spec = default_layout,
    text_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id,
    optional<size_t> const& length_limit = none)
{
    if (do_unsafe_text_control(ctx, value, layout_spec, flags, id,
            length_limit))
    {
        end_pass(ctx);
    }
}

template<class T>
text_control_result
do_unsafe_text_control(
    ui_context& ctx,
    accessor<T> const& accessor,
    layout const& layout_spec = default_layout,
    text_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id,
    optional<size_t> const& length_limit = none)
{
    return do_unsafe_text_control(
        ctx, as_settable_text(ctx, ref(&accessor)), layout_spec, flags, id,
        length_limit);
}

template<class T>
void
do_text_control(
    ui_context& ctx,
    accessor<T> const& accessor,
    layout const& layout_spec = default_layout,
    text_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id,
    optional<size_t> const& length_limit = none)
{
    if (do_unsafe_text_control(ctx, accessor, layout_spec, flags, id,
            length_limit))
    {
        end_pass(ctx);
    }
}

template<class T>
settable_text_accessor<indirect_accessor<T>>
formatted_number_as_settable_text(
    ui_context& ctx, indirect_accessor<T> const& x, char const* format)
{
    keyed_data<string>* data;
    get_cached_data(ctx, &data);
    auto d = printf(ctx, format, x);

    if (is_gettable(x))
    {
        refresh_keyed_data(*data, x.id());
        if (!is_valid(*data) && is_gettable(d))
        {
            set(*data, get(d));
        }
    }
    else
    {
        invalidate(*data);
    }

    return settable_text_accessor<indirect_accessor<T>>(x, data);
}

template<class T>
text_control_result
do_unsafe_formatted_numeric_text_control(
    ui_context& ctx,
    accessor<T> const& accessor,
    char const* format,
    layout const& layout_spec = default_layout,
    text_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id,
    optional<size_t> const& length_limit = none)
{
    return do_unsafe_text_control(
        ctx,
        formatted_number_as_settable_text(ctx, ref(&accessor), format),
        layout_spec,
        flags, id, length_limit);
}

template<class T>
void
do_formatted_numeric_text_control(
    ui_context& ctx,
    accessor<T> const& accessor,
    char const* format,
    layout const& layout_spec = default_layout,
    text_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id,
    optional<size_t> const& length_limit = none)
{
    if (do_unsafe_formatted_numeric_text_control(ctx, accessor, layout_spec, flags, id,
        length_limit))
    {
        end_pass(ctx);
    }
}

// BUTTONS

// link - meant to resemble browser links

bool
do_unsafe_link(
    ui_context& ctx,
    accessor<string> const& text,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

bool
do_unsafe_link(
    ui_context& ctx,
    accessor<string> const& text,
    accessor<string> const& tooltip,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

[[deprecated("Use the action-based interface.")]]
bool static inline
do_link(
    ui_context& ctx,
    accessor<string> const& text,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id)
{
    return do_unsafe_link(ctx, text, layout_spec, id);
}

void
do_link(
    ui_context& ctx,
    accessor<string> const& text,
    action const& on_click,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

void
do_link(
    ui_context& ctx,
    accessor<string> const& text,
    accessor<string> const& tooltip,
    action const& on_click,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

template<class Handler>
[[deprecated("Use the action-based interface.")]]
std::enable_if_t<
    !std::is_convertible<Handler,layout>::value &&
    !std::is_base_of<action,Handler>::value>
do_link(
    ui_context& ctx,
    accessor<string> const& text,
    Handler const& handler,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id)
{
    if (do_unsafe_link(ctx, text, layout_spec, id))
    {
        handler();
        end_pass(ctx);
    }
}

template<class Handler>
[[deprecated("Use the action-based interface.")]]
std::enable_if_t<
    !std::is_convertible<Handler,layout>::value &&
    !std::is_base_of<action,Handler>::value>
do_link(
    ui_context& ctx,
    accessor<string> const& text,
    accessor<string> const& tooltip,
    Handler const& handler,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id)
{
    if (do_unsafe_link(ctx, text, tooltip, layout_spec, id))
    {
        handler();
        end_pass(ctx);
    }
}

void do_url_link(
    ui_context& ctx,
    accessor<string> const& text,
    accessor<string> const& url,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

// text button

typedef bool button_result;

ALIA_DEFINE_FLAG_TYPE(button)
ALIA_DEFINE_FLAG(button, 0x1, BUTTON_DISABLED)

button_result
do_unsafe_button(
    ui_context& ctx,
    accessor<string> const& label,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

button_result
do_unsafe_button(
    ui_context& ctx,
    accessor<string> const& label,
    accessor<string> const& tooltip,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

[[deprecated("Use the action-based interface.")]]
button_result static inline
do_button(
    ui_context& ctx,
    accessor<string> const& label,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    return do_unsafe_button(ctx, label, layout_spec, flags, id);
}

template<class Handler>
[[deprecated("Use the action-based interface.")]]
std::enable_if_t<
    !std::is_convertible<Handler,layout>::value &&
    !std::is_base_of<action,Handler>::value>
do_button(
    ui_context& ctx,
    accessor<string> const& label,
    Handler const& handler,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_button(ctx, label, layout_spec, flags, id))
    {
        handler();
        end_pass(ctx);
    }
}

template<class Handler>
[[deprecated("Use the action-based interface.")]]
std::enable_if_t<
    !std::is_convertible<Handler,layout>::value &&
    !std::is_base_of<action,Handler>::value>
do_button(
    ui_context& ctx,
    accessor<string> const& label,
    accessor<string> const& tooltip,
    Handler const& handler,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_button(ctx, label, tooltip, layout_spec, flags, id))
    {
        handler();
        end_pass(ctx);
    }
}

void
do_button(
    ui_context& ctx,
    accessor<string> const& label,
    action const& on_press,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

void
do_button(
    ui_context& ctx,
    accessor<string> const& label,
    accessor<string> const& tooltip,
    action const& on_press,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

// the primary button for a UI

button_result
do_unsafe_primary_button(
    ui_context& ctx,
    accessor<string> const& label,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

[[deprecated("Use the action-based interface.")]]
button_result static inline
do_primary_button(
    ui_context& ctx,
    accessor<string> const& label,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    return do_unsafe_primary_button(ctx, label, layout_spec, flags, id);
}

template<class Handler>
[[deprecated("Use the action-based interface.")]]
std::enable_if_t<
    !std::is_convertible<Handler,layout>::value &&
    !std::is_base_of<action,Handler>::value>
do_primary_button(
    ui_context& ctx,
    accessor<string> const& label,
    Handler const& handler,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_primary_button(ctx, label, layout_spec, flags, id))
    {
        handler();
        end_pass(ctx);
    }
}

void
do_primary_button(
    ui_context& ctx,
    accessor<string> const& label,
    action const& on_press,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

// a button with a custom style

button_result
do_unsafe_styled_button(
    ui_context& ctx,
    accessor<string> const& style,
    accessor<string> const& label,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

button_result
do_unsafe_styled_button(
    ui_context& ctx,
    accessor<string> const& style,
    accessor<string> const& label,
    accessor<string> const& tooltip,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

[[deprecated("Use the action-based interface.")]]
button_result static inline
do_styled_button(
    ui_context& ctx,
    accessor<string> const& style,
    accessor<string> const& label,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    return do_unsafe_styled_button(ctx, style, label, layout_spec, flags, id);
}

template<class Handler>
[[deprecated("Use the action-based interface.")]]
std::enable_if_t<
    !std::is_convertible<Handler,layout>::value &&
    !std::is_base_of<action,Handler>::value>
do_styled_button(
    ui_context& ctx,
    accessor<string> const& style,
    accessor<string> const& label,
    Handler const& handler,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_styled_button(ctx, style, label, layout_spec, flags, id))
    {
        handler();
        end_pass(ctx);
    }
}

template<class Handler>
[[deprecated("Use the action-based interface.")]]
std::enable_if_t<
    !std::is_convertible<Handler,layout>::value &&
    !std::is_base_of<action,Handler>::value>
do_styled_button(
    ui_context& ctx,
    accessor<string> const& style,
    accessor<string> const& label,
    accessor<string> const& tooltip,
    Handler const& handler,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_styled_button(ctx, style, label, tooltip, layout_spec, flags, id))
    {
        handler();
        end_pass(ctx);
    }
}

void
do_styled_button(
    ui_context& ctx,
    accessor<string> const& style,
    accessor<string> const& label,
    action const& on_press,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

void
do_styled_button(
    ui_context& ctx,
    accessor<string> const& style,
    accessor<string> const& label,
    accessor<string> const& tooltip,
    action const& on_press,
    layout const& layout_spec = default_layout,
    button_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

// icon button

ALIA_DEFINE_FLAG_TYPE(simple_control)
ALIA_DEFINE_FLAG(simple_control, 0x1, SIMPLE_CONTROL_DISABLED)

typedef bool icon_button_result;

enum icon_type
{
    REMOVE_ICON,
    DRAG_ICON,
    MENU_ICON,
    EXPAND_ICON,
    SHRINK_ICON,
    PLUS_ICON,
    MINUS_ICON,
    CONTOUR_ICON,
    SOLID_ICON
};

icon_button_result
do_unsafe_icon_button(
    ui_context& ctx,
    icon_type icon,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

icon_button_result
do_unsafe_icon_button(
    ui_context& ctx,
    icon_type icon,
    accessor<string> const& tooltip,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

[[deprecated("Use the action-based interface.")]]
icon_button_result static inline
do_icon_button(
    ui_context& ctx,
    icon_type icon,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    return do_unsafe_icon_button(ctx, icon, layout_spec, flags, id);
}

template<class Handler>
[[deprecated("Use the action-based interface.")]]
std::enable_if_t<
    !std::is_convertible<Handler,layout>::value &&
    !std::is_base_of<action,Handler>::value>
do_icon_button(
    ui_context& ctx,
    icon_type icon,
    Handler const& handler,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_icon_button(ctx, icon, layout_spec, flags, id))
    {
        handler();
        end_pass(ctx);
    }
}

template<class Handler>
[[deprecated("Use the action-based interface.")]]
std::enable_if_t<
    !std::is_convertible<Handler,layout>::value &&
    !std::is_base_of<action,Handler>::value>
do_icon_button(
    ui_context& ctx,
    icon_type icon,
    accessor<string> const& tooltip,
    Handler const& handler,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_icon_button(ctx, icon, tooltip, layout_spec, flags, id))
    {
        handler();
        end_pass(ctx);
    }
}

void
do_icon_button(
    ui_context& ctx,
    icon_type icon,
    action const& on_press,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

void
do_icon_button(
    ui_context& ctx,
    icon_type icon,
    accessor<string> const& tooltip,
    action const& on_press,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

// CONTROLS

// check box

typedef control_result check_box_result;

check_box_result
do_unsafe_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

void static inline
do_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_check_box(ctx, value, layout_spec, flags, id))
        end_pass(ctx);
}

check_box_result
do_unsafe_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& text,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

void static inline
do_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& text,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_check_box(ctx, value, text, layout_spec, flags, id))
        end_pass(ctx);
}

check_box_result
do_unsafe_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& text,
    accessor<string> const& tooltip,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

void static inline
do_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& text,
    accessor<string> const& tooltip,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_check_box(ctx, value, text, tooltip, layout_spec, flags, id))
        end_pass(ctx);
}

// radio button

typedef control_result radio_button_result;

radio_button_result
do_unsafe_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

void static inline
do_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_radio_button(ctx, value, layout_spec, flags, id))
        end_pass(ctx);
}

radio_button_result
do_unsafe_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& text,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

void static inline
do_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& text,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_radio_button(ctx, value, text, layout_spec, flags, id))
        end_pass(ctx);
}

radio_button_result
do_unsafe_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& text,
    accessor<string> const& tooltip,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

void static inline
do_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& text,
    accessor<string> const& tooltip,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_radio_button(ctx, value, text, tooltip, layout_spec, flags, id))
        end_pass(ctx);
}

radio_button_result
do_unsafe_radio_button_with_description(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& label,
    accessor<string> const& description,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

void static inline
do_radio_button_with_description(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& label,
    accessor<string> const& description,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_radio_button_with_description(
            ctx, value, label, description, layout_spec, flags, id))
    {
        end_pass(ctx);
    }
}

// make_radio_accessor(selected_value, this_value), where selected_value and
// this_value are both of type accessor<T>, yields an accessor<bool> whose
// value tells whether or not selected_value is set to this_value.
// Setting the resulting accessor to any value sets selected_value's value to
// this_value. (Setting it to false is considered nonsensical.)
template<class Accessor, class Index>
struct radio_accessor : regular_accessor<bool>
{
    radio_accessor(
        Accessor const& selected_value,
        Index const& this_value)
      : selected_value_(selected_value), this_value_(this_value)
    {}
    bool is_gettable() const
    { return selected_value_.is_gettable() && this_value_.is_gettable(); }
    bool const& get() const
    { return lazy_getter_.get(*this); }
    bool is_settable() const
    { return selected_value_.is_settable() && this_value_.is_gettable(); }
    void set(bool const& value) const
    { selected_value_.set(this_value_.get()); }
 private:
    friend struct lazy_getter<bool>;
    bool generate() const
    { return selected_value_.get() == this_value_.get(); }
    Accessor selected_value_;
    Index this_value_;
    lazy_getter<bool> lazy_getter_;
};
template<class Accessor, class Index>
radio_accessor<
    typename copyable_accessor_helper<Accessor const&>::result_type,
    typename copyable_accessor_helper<Index const&>::result_type>
make_radio_accessor(
    Accessor const& selected_value,
    Index const& this_value)
{
    return
        radio_accessor<
            typename copyable_accessor_helper<Accessor const&>::result_type,
            typename copyable_accessor_helper<Index const&>::result_type>(
                make_accessor_copyable(selected_value),
                make_accessor_copyable(this_value));
}

// make_radio_accessor_for_optional(selected_value, this_value), where
// selected_value is of type accessor<optional<T>> and this_value is of type
// accessor<T>, yields an accessor<bool> whose value tells whether or not
// selected_value is set to this_value.
// Setting the resulting accessor to any value sets selected_value's value to
// this_value. (Setting it to false results in a none optional type set)
template<class Accessor, class Index>
struct radio_accessor_for_optional : regular_accessor<bool>
{
    radio_accessor_for_optional(
        Accessor const& selected_value,
        Index const& this_value)
      : selected_value_(selected_value), this_value_(this_value)
    {}
    bool is_gettable() const
    { return selected_value_.is_gettable() && this_value_.is_gettable(); }
    bool const& get() const
    { return lazy_getter_.get(*this); }
    bool is_settable() const
    { return selected_value_.is_settable() && this_value_.is_gettable(); }
    void set(bool const& value) const
    {
        if(value)
            selected_value_.set(some(this_value_.get()));
        else
            selected_value_.set(none);
    }
 private:
    friend struct lazy_getter<bool>;
    bool generate() const
    {
        auto const& selected = selected_value_.get();
        return selected && selected.get() == this_value_.get();
    }
    Accessor selected_value_;
    Index this_value_;
    lazy_getter<bool> lazy_getter_;
};
template<class Accessor, class Index>
radio_accessor_for_optional<
    typename copyable_accessor_helper<Accessor const&>::result_type,
    typename copyable_accessor_helper<Index const&>::result_type>
make_radio_accessor_for_optional(
    Accessor const& selected_value,
    Index const& this_value)
{
    return
        radio_accessor_for_optional<
            typename copyable_accessor_helper<Accessor const&>::result_type,
            typename copyable_accessor_helper<Index const&>::result_type>(
                make_accessor_copyable(selected_value),
                make_accessor_copyable(this_value));
}

template<class Index>
radio_button_result
do_unsafe_radio_button(
    ui_context& ctx,
    accessor<Index> const& selected_value,
    accessor<Index> const& this_value,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    return do_unsafe_radio_button(ctx,
        make_radio_accessor(ref(&selected_value), this_value),
        layout_spec, flags, id);
}

template<class Index>
void
do_radio_button(
    ui_context& ctx,
    accessor<Index> const& selected_value,
    accessor<Index> const& this_value,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_radio_button(ctx, selected_value, this_value,
            layout_spec, flags, id))
    {
        end_pass(ctx);
    }
}

template<class Index>
radio_button_result
do_unsafe_radio_button(
    ui_context& ctx,
    accessor<Index> const& selected_value,
    accessor<Index> const& this_value,
    accessor<string> const& text,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    return do_unsafe_radio_button(ctx,
        make_radio_accessor(ref(&selected_value), this_value),
        text, layout_spec, flags, id);
}

template<class Index>
void
do_radio_button(
    ui_context& ctx,
    accessor<Index> const& selected_value,
    accessor<Index> const& this_value,
    accessor<string> const& text,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_radio_button(ctx, selected_value, this_value,
            text, layout_spec, flags, id))
    {
        end_pass(ctx);
    }
}

template<class Index>
radio_button_result
do_unsafe_radio_button_with_description(
    ui_context& ctx,
    accessor<Index> const& selected_value,
    accessor<Index> const& this_value,
    accessor<string> const& label,
    accessor<string> const& description,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    return do_unsafe_radio_button_with_description(ctx,
        make_radio_accessor(ref(&selected_value), this_value),
        label, description, layout_spec, flags, id);
}

template<class Index>
void
do_radio_button_with_description(
    ui_context& ctx,
    accessor<Index> const& selected_value,
    accessor<Index> const& this_value,
    accessor<string> const& label,
    accessor<string> const& description,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id)
{
    if (do_unsafe_radio_button_with_description(
            ctx, selected_value, this_value, label, description,
            layout_spec, flags, id))
    {
        end_pass(ctx);
    }
}

// node expander
typedef control_result node_expander_result;
node_expander_result
do_unsafe_node_expander(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec = default_layout,
    simple_control_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

// slider

typedef control_result slider_result;

ALIA_DEFINE_FLAG_TYPE(slider)
ALIA_DEFINE_FLAG(slider, 0x0, SLIDER_HORIZONTAL)
ALIA_DEFINE_FLAG(slider, 0x1, SLIDER_VERTICAL)

slider_result
do_unsafe_slider(ui_context& ctx, accessor<double> const& value,
    double minimum, double maximum, double step = 0,
    layout const& layout_spec = default_layout,
    slider_flag_set flags = NO_FLAGS);

void static inline
do_slider(ui_context& ctx, accessor<double> const& value,
    double minimum, double maximum, double step = 0,
    layout const& layout_spec = default_layout,
    slider_flag_set flags = NO_FLAGS)
{
    if (do_unsafe_slider(ctx, value, minimum, maximum, step,
            layout_spec, flags))
    {
        end_pass(ctx);
    }
}

// PANELS

// Currently all panel types share the same flag set, but some flags obviously
// only apply to certain types.
ALIA_DEFINE_FLAG_TYPE(panel)
ALIA_DEFINE_FLAG(panel, 0x00001, PANEL_HORIZONTAL)
ALIA_DEFINE_FLAG(panel, 0x00002, PANEL_VERTICAL)
ALIA_DEFINE_FLAG(panel, 0x00004, PANEL_HIDE_FOCUS)
ALIA_DEFINE_FLAG(panel, 0x00010, PANEL_SELECTED)
ALIA_DEFINE_FLAG(panel, 0x00020, PANEL_NO_INTERNAL_PADDING)
ALIA_DEFINE_FLAG(panel, 0x00040, PANEL_NO_CLICK_DETECTION)
ALIA_DEFINE_FLAG(panel, 0x00080, PANEL_IGNORE_STYLE_PADDING)
ALIA_DEFINE_FLAG(panel, 0x00100, PANEL_NO_REGION)
ALIA_DEFINE_FLAG(panel, 0x00200, PANEL_UNSAFE_CLICK_DETECTION)
// scrolling only
ALIA_DEFINE_FLAG(panel, 0x01000, PANEL_NO_HORIZONTAL_SCROLLING)
ALIA_DEFINE_FLAG(panel, 0x02000, PANEL_NO_VERTICAL_SCROLLING)
ALIA_DEFINE_FLAG(panel, 0x04000, PANEL_RESERVE_HORIZONTAL_SCROLLBAR)
ALIA_DEFINE_FLAG(panel, 0x08000, PANEL_RESERVE_VERTICAL_SCROLLBAR)
// clickable only
ALIA_DEFINE_FLAG(panel, 0x10000, PANEL_DISABLED)

struct panel_data;

struct panel : noncopyable
{
 public:
    panel() {}
    panel(
        ui_context& ctx, accessor<string> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        widget_state state = WIDGET_NORMAL)
    { begin(ctx, style, layout_spec, flags, id, state); }
    ~panel() { end(); }
    void begin(
        ui_context& ctx, accessor<string> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        widget_state state = WIDGET_NORMAL);
    void end();
    // inner_region() is the region inside the panel's border
    layout_box inner_region() const { return inner_.padded_region(); }
    // outer_region() includes the border
    layout_box outer_region() const;
    // padded_region() includes the padding
    layout_box padded_region() const;
 private:
    ui_context* ctx_;
    panel_data* data_;
    bordered_layout outer_;
    scoped_substyle substyle_;
    linear_layout inner_;
    panel_flag_set flags_;
};

class clickable_panel : noncopyable
{
 public:
    clickable_panel() {}
    clickable_panel(
        ui_context& ctx, accessor<string> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS, widget_id id = auto_id)
    { begin(ctx, style, layout_spec, flags, id); }
    void begin(
        ui_context& ctx, accessor<string> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS, widget_id id = auto_id);
    void end() { panel_.end(); }
    layout_box inner_region() const { return panel_.inner_region(); }
    layout_box outer_region() const { return panel_.outer_region(); }
    layout_box padded_region() const { return panel_.padded_region(); }
    bool clicked() const { return clicked_; }
 private:
    panel panel_;
    bool clicked_;
};

struct scrolling_data;

struct scrollable_region : noncopyable
{
    scrollable_region() : ctx_(0) {}
    scrollable_region(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        unsigned scrollable_axes = 1 | 2,
        widget_id id = auto_id,
        optional_storage<layout_vector> const& scroll_position_storage = none,
        unsigned reserved_axes = 0)
    {
        begin(ctx, layout_spec, scrollable_axes, id, scroll_position_storage,
            reserved_axes);
    }
    ~scrollable_region() { end(); }

    void begin(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        unsigned scrollable_axes = 1 | 2,
        widget_id id = auto_id,
        optional_storage<layout_vector> const& scroll_position_storage = none,
        unsigned reserved_axes = 0);
    void end();

 private:
    ui_context* ctx_;
    scrolling_data* data_;
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
        ui_context& ctx, accessor<string> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        optional_storage<layout_vector> const& scroll_position_storage = none)
    { begin(ctx, style, layout_spec, flags, scroll_position_storage); }
    ~scrollable_panel() { end(); }
    void begin(
        ui_context& ctx, accessor<string> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        optional_storage<layout_vector> const& scroll_position_storage = none);
    void end();
 private:
    bordered_layout outer_;
    scoped_substyle substyle_;
    scrollable_region region_;
    bordered_layout padding_border_;
    linear_layout inner_;
};

struct custom_panel_data;
struct panel_style_info;

struct custom_panel : noncopyable
{
 public:
    custom_panel() : ctx_(0) {}
    custom_panel(
        ui_context& ctx, custom_panel_data& data,
        accessor<panel_style_info> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        widget_state state = WIDGET_NORMAL)
    { begin(ctx, data, style, layout_spec, flags, id, state); }
    ~custom_panel() { end(); }
    void begin(
        ui_context& ctx, custom_panel_data& data,
        accessor<panel_style_info> const& style,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        widget_state state = WIDGET_NORMAL);
    void end();
    // inner_region() is the region inside the panel's border
    layout_box inner_region() const { return inner_.padded_region(); }
    // outer_region() includes the border
    layout_box outer_region() const;
    // padded_region() includes the padding
    layout_box padded_region() const;
 private:
    ui_context* ctx_;
    panel_style_info* style_;
    bordered_layout outer_;
    linear_layout inner_;
    panel_flag_set flags_;
};

// CONTAINERS

class collapsible_content : noncopyable
{
 public:
    collapsible_content() {}
    ~collapsible_content() { end(); }

    collapsible_content(ui_context& ctx, bool expanded,
        animated_transition const& transition = default_transition,
        double const offset_factor = 1.,
        layout const& layout_spec = default_layout)
    { begin(ctx, expanded, transition, offset_factor, layout_spec); }

    collapsible_content(ui_context& ctx, float expansion,
        double const offset_factor = 1.,
        layout const& layout_spec = default_layout)
    { begin(ctx, expansion, offset_factor, layout_spec); }

    void begin(ui_context& ctx, bool expanded,
        animated_transition const& transition = default_transition,
        double const offset_factor = 1.,
        layout const& layout_spec = default_layout);

    void begin(ui_context& ctx, float expansion,
        double const offset_factor = 1.,
        layout const& layout_spec = default_layout);

    void end();

    bool do_content() const { return do_content_; }

 private:
    ui_context* ctx_;
    scoped_layout_container container_;
    scoped_clip_region clipper_;
    scoped_transformation transform_;
    column_layout layout_;
    bool do_content_;
};

class horizontal_collapsible_content : noncopyable
{
 public:
    horizontal_collapsible_content() {}
    ~horizontal_collapsible_content() { end(); }

    horizontal_collapsible_content(ui_context& ctx, bool expanded,
        animated_transition const& transition = default_transition,
        double const offset_factor = 1.,
        layout const& layout_spec = default_layout)
    { begin(ctx, expanded, transition, offset_factor, layout_spec); }

    horizontal_collapsible_content(ui_context& ctx, float expansion,
        double const offset_factor = 1.,
        layout const& layout_spec = default_layout)
    { begin(ctx, expansion, offset_factor, layout_spec); }

    void begin(ui_context& ctx, bool expanded,
        animated_transition const& transition = default_transition,
        double const offset_factor = 1.,
        layout const& layout_spec = default_layout);

    void begin(ui_context& ctx, float expansion,
        double const offset_factor = 1.,
        layout const& layout_spec = default_layout);

    void end();

    bool do_content() const { return do_content_; }

 private:
    ui_context* ctx_;
    scoped_layout_container container_;
    scoped_clip_region clipper_;
    scoped_transformation transform_;
    row_layout layout_;
    bool do_content_;
};

ALIA_DEFINE_FLAG_TYPE(tree_node)
ALIA_DEFINE_FLAG(tree_node, 0x1, TREE_NODE_INITIALLY_EXPANDED)
ALIA_DEFINE_FLAG(tree_node, 0x2, TREE_NODE_DISABLED)

struct tree_node : noncopyable
{
    tree_node() {}
    ~tree_node() { end(); }

    tree_node(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        tree_node_flag_set flags = NO_FLAGS,
        optional_storage<bool> const& expanded = optional_storage<bool>(none),
        widget_id expander_id = auto_id)
    { begin(ctx, layout_spec, flags, expanded, expander_id); }

    void begin(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        tree_node_flag_set flags = NO_FLAGS,
        optional_storage<bool> const& expanded = optional_storage<bool>(none),
        widget_id expander_id = auto_id);

    bool do_children();

    node_expander_result const& expander_result() const
    { return expander_result_; }

    void end();

 private:
    ui_context* ctx_;

    grid_layout grid_;
    row_layout label_region_;
    collapsible_content content_;
    grid_row row_;
    column_layout column_;

    bool is_expanded_;
    node_expander_result expander_result_;
};

struct accordion : noncopyable
{
    accordion(ui_context& ctx, layout const& layout_spec = default_layout)
    { begin(ctx, layout_spec); }
    ~accordion() { end(); }
    void begin(ui_context& ctx, layout const& layout_spec = default_layout);
    void end();
 private:
    friend struct accordion_section;
    ui_context* ctx_;
    int* selection_;
    int index_;
    column_layout layout_;
};

struct accordion_section : noncopyable
{
    accordion_section() {}
    accordion_section(ui_context& ctx, accessor<bool> const& selected)
    { begin(ctx, selected); }
    accordion_section(accordion& parent)
    { begin(parent); }
    ~accordion_section() { end(); }
    void begin(ui_context& ctx, accessor<bool> const& selected);
    void begin(accordion& parent);
    void end();
    bool do_content();
    bool clicked() const { return clicked_; }
 private:
    ui_context* ctx_;
    clickable_panel panel_;
    bool is_selected_;
    collapsible_content content_;
    bool clicked_;
};

struct horizontal_accordion : noncopyable
{
    horizontal_accordion(
        ui_context& ctx, layout const& layout_spec = default_layout)
    { begin(ctx, layout_spec); }
    ~horizontal_accordion() { end(); }
    void begin(ui_context& ctx, layout const& layout_spec = default_layout);
    void end();
 private:
    friend struct horizontal_accordion_section;
    ui_context* ctx_;
    int* selection_;
    int index_;
    row_layout layout_;
};

struct horizontal_accordion_section : noncopyable
{
    horizontal_accordion_section() {}
    horizontal_accordion_section(
        ui_context& ctx, accessor<bool> const& selected)
    { begin(ctx, selected); }
    horizontal_accordion_section(horizontal_accordion& parent)
    { begin(parent); }
    ~horizontal_accordion_section() { end(); }
    void begin(ui_context& ctx, accessor<bool> const& selected);
    void begin(horizontal_accordion& parent);
    void end();
    bool do_content();
    bool clicked() const { return clicked_; }
 private:
    ui_context* ctx_;
    clickable_panel panel_;
    bool is_selected_;
    horizontal_collapsible_content content_;
    bool clicked_;
};

struct clamped_content : noncopyable
{
    clamped_content() {}
    clamped_content(ui_context& ctx,
        accessor<string> const& background_style,
        accessor<string> const& content_style,
        absolute_size const& max_size,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS)
    {
        begin(ctx, background_style, content_style, max_size, layout_spec,
            flags);
    }
    ~clamped_content() { end(); }

    void begin(ui_context& ctx,
        accessor<string> const& background_style,
        accessor<string> const& content_style,
        absolute_size const& max_size,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS);
    void end();

 private:
    ui_context* ctx_;
    scrollable_panel background_;
    clamped_layout clamp_;
    panel content_;
};

struct clamped_header : noncopyable
{
    clamped_header() {}
    clamped_header(ui_context& ctx,
        accessor<string> const& background_style,
        accessor<string> const& header_style,
        absolute_size const& max_size,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS)
    {
        begin(ctx, background_style, header_style, max_size, layout_spec,
            flags);
    }
    ~clamped_header() { end(); }

    void begin(ui_context& ctx,
        accessor<string> const& background_style,
        accessor<string> const& header_style,
        absolute_size const& max_size,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS);
    void end();

 private:
    ui_context* ctx_;
    scrollable_panel background_;
    clamped_layout clamp_;
    panel header_;
};

ALIA_DEFINE_FLAG_TYPE(tab_strip)
ALIA_DEFINE_FLAG(tab_strip, 0x0, TAB_STRIP_HORIZONTAL)
ALIA_DEFINE_FLAG(tab_strip, 0x1, TAB_STRIP_VERTICAL)

struct tab_strip : noncopyable
{
    tab_strip(ui_context& ctx, layout const& layout_spec = default_layout,
        tab_strip_flag_set flags = NO_FLAGS)
    { begin(ctx, layout_spec, flags); }
    ~tab_strip() { end(); }
    void begin(ui_context& ctx, layout const& layout_spec = default_layout,
        tab_strip_flag_set flags = NO_FLAGS);
    void end();
 private:
    friend struct tab;
    ui_context* ctx_;
    scoped_substyle style_;
    layered_layout layering_;
    linear_layout tab_container_;
};

struct tab : noncopyable
{
    tab() {}
    tab(ui_context& ctx, accessor<bool> const& selected)
    { begin(ctx, selected); }
    ~tab() { end(); }
    void begin(ui_context& ctx, accessor<bool> const& selected);
    void end();
 private:
    ui_context* ctx_;
    clickable_panel panel_;
    bool is_selected_;
};

void do_tab(ui_context& ctx, accessor<bool> const& selected,
    accessor<string> const& label);

struct transitioning_layout_container;
struct transitioning_layout_content_data;

// A transitioning container allows you to specify multiple alternative
// content blocks and have the UI smoothly transition between them.
class transitioning_container : noncopyable
{
 public:
    transitioning_container() {}
    ~transitioning_container() { end(); }

    transitioning_container(
        ui_context& ctx,
        animated_transition const& transition = default_transition,
        layout const& layout_spec = default_layout)
    { begin(ctx, transition, layout_spec); }

    void begin(
        ui_context& ctx,
        animated_transition const& transition = default_transition,
        layout const& layout_spec = default_layout);

    void end();

 private:
    friend class transitioning_container_content;
    ui_context* ctx_;
    animated_transition transition_;
    transitioning_layout_container* layout_;
    scoped_layout_container container_;
    scoped_transformation transform_;
    widget_id id_;
    scoped_clip_region clipper_;
    transitioning_layout_content_data** next_ptr_;
};

struct offscreen_subsurface;
struct scoped_surface_opacity_data;

// Within the scope of a scoped_surface_opacity, all renderer content is
// reduced in opacity by applying the specified factor.
// If possible, this is done by generating an offscreen rendering buffer.
struct scoped_surface_opacity : noncopyable
{
    scoped_surface_opacity() : ctx_(0) {}
    scoped_surface_opacity(ui_context& ctx, float opacity)
    { begin(ctx, opacity); }
    ~scoped_surface_opacity() { end(); }
    void begin(ui_context& ctx, float opacity);
    void end();
 private:
    dataless_ui_context* ctx_;
    scoped_surface_opacity_data* data_;
    // used if offscreen rendering if supported
    offscreen_subsurface* old_subsurface_;
    float opacity_;
    // used in fallback mode
    float old_opacity_;
};

// transitioning_container_content specifies a single content block within a
// transitioning_container.
class transitioning_container_content : noncopyable
{
 public:
    transitioning_container_content() {}
    ~transitioning_container_content() { end(); }

    transitioning_container_content(ui_context& ctx,
        transitioning_container& container, bool active)
    { begin(ctx, container, active); }

    void begin(ui_context& ctx, transitioning_container& container,
        bool active);

    void end();

    bool do_content() const { return do_content_; }

    column_layout& content_holder() { return content_holder_; }

 private:
    ui_context* ctx_;
    transitioning_container* container_;
    column_layout content_holder_;
    scoped_surface_opacity transparency_;
    bool do_content_;
};

// OVERLAYS

struct overlay_event_transformer
{
    overlay_event_transformer() : ctx_(0) {}
    overlay_event_transformer(dataless_ui_context& ctx, widget_id id)
    { begin(ctx, id); }
    ~overlay_event_transformer() { end(); }
    void begin(dataless_ui_context& ctx, widget_id id);
    void end();
 private:
    dataless_ui_context* ctx_;
    ui_event_category real_event_category_;
    ui_event_type real_event_type_;
};

struct popup_positioning
{
    layout_vector lower_bound = make_layout_vector(0, 0);
    layout_vector upper_bound = make_layout_vector(0, 0);
    layout_vector absolute_lower = make_layout_vector(0, 0);
    layout_vector absolute_upper = make_layout_vector(0, 0);
    layout_vector minimum_size = make_layout_vector(-1, -1); // ignored if negative
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
    overlay_event_transformer overlay_;
};

// DROP DOWNS

struct ddl_data;

ALIA_DEFINE_FLAG_TYPE(ddl)
ALIA_DEFINE_FLAG(ddl, 0x1, DDL_COMMAND_LIST)
ALIA_DEFINE_FLAG(ddl, 0x2, DDL_DISABLED)

struct untyped_drop_down_list : noncopyable
{
 public:
    untyped_drop_down_list() : ctx_(0) {}
    ~untyped_drop_down_list() { end(); }

    untyped_ui_value const*
    begin(ui_context& ctx, layout const& layout_spec, ddl_flag_set flags);

    void end();

    bool do_list();

    ui_context& context() { return *ctx_; }

 private:
    friend struct untyped_ddl_item;

    ui_context* ctx_;
    layout layout_spec_;
    ddl_flag_set flags_;
    ddl_data* data_;
    widget_id id_;
    panel container_;
    flow_layout contents_;

    popup popup_;
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
        ddl_flag_set flags = NO_FLAGS)
    { begin(ctx, selection, layout_spec, flags); }
    ~drop_down_list() { end(); }

    void begin(ui_context& ctx, accessor<Index> const& selection,
        layout const& layout_spec = default_layout,
        ddl_flag_set flags = NO_FLAGS)
    {
        changed_ = false;

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
            // This should only fail if an event with the wrong value type is
            // somehow sent to this widget.
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
    template<class _Index>
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
 private:
    untyped_drop_down_list* list_;
    panel panel_;
    flow_layout layout_;
};

template<class Index>
struct ddl_item : noncopyable
{
 public:
    ddl_item() {}
    ddl_item(drop_down_list<Index>& list, accessor<Index> const& index)
    { begin(list, index); }
    ddl_item(drop_down_list<Index>& list, Index const& index)
    { begin(list, index); }
    ~ddl_item() { end(); }

    void begin(drop_down_list<Index>& list, accessor<Index> const& index)
    {
        alia_if_(list.list_.context(), is_gettable(index))
        {
            selected_ = list.selection_ ? get(list.selection_) == index.get() : false;
            if (item_.begin(list.list_, selected_))
            {
                typed_ui_value<Index>* v = new typed_ui_value<Index>;
                v->value = index.get();
                item_.select(v);
            }
        }
        alia_end
    }
    void begin(drop_down_list<Index>& list, Index const& index)
    {
        begin(list, in(index));
    }
    void end() { item_.end(); }

    bool is_selected() const { return selected_; }

 private:
    untyped_ddl_item item_;
    bool selected_;
};

template<class Index>
void
do_drop_down_list(
    ui_context& ctx,
    accessor<Index> const& selection,
    layout const& layout_spec,
    boost::function<void()> const& do_selection,
    boost::function<void(drop_down_list<Index>& ddl)> const& do_list)
{
    drop_down_list<Index> ddl(ctx, selection, layout_spec);
    do_selection();
    alia_if (ddl.do_list())
    {
        do_list(ddl);
    }
    alia_end
    if (ddl.changed())
    {
        end_pass(ctx);
    }
}

// The following takes care of implementing a drop down menu of commands via the
// drop_down_list interface above.

struct drop_down_menu_context;

// Do an individual option within a menu.
// action-based interface
void
do_menu_option(
    drop_down_menu_context& menu_ctx,
    boost::function<void()> const& do_label,
    action const& on_click);
// lambda-based interface
void
do_menu_option(
    drop_down_menu_context& menu_ctx,
    boost::function<void()> const& do_label,
    boost::function<void()> const& on_click);
// action-based interface with text label
void
do_menu_option(
    drop_down_menu_context& menu_ctx,
    accessor<string> const& label,
    action const& on_click);
// lambda-based interface with text label
void
do_menu_option(
    drop_down_menu_context& menu_ctx,
    accessor<string> const& label,
    boost::function<void()> const& on_click);

// Do a drop down menu.
// do_options is called to enumerate the available menu options (by calling
// do_menu_option).
void
do_drop_down_menu(
    ui_context& ctx,
    layout const& layout_spec,
    boost::function<void(drop_down_menu_context& menu_ctx)> const& do_options);

// resizable_content is a container with a draggable separator for controlling
// the size of its contents.
ALIA_DEFINE_FLAG_TYPE(resizable_content)
ALIA_DEFINE_FLAG(resizable_content, 0, RESIZABLE_CONTENT_VERTICAL_SEPARATOR)
ALIA_DEFINE_FLAG(resizable_content, 1, RESIZABLE_CONTENT_HORIZONTAL_SEPARATOR)
ALIA_DEFINE_FLAG(resizable_content, 0, RESIZABLE_CONTENT_APPEND_SEPARATOR)
ALIA_DEFINE_FLAG(resizable_content, 2, RESIZABLE_CONTENT_PREPEND_SEPARATOR)
struct resizable_content : noncopyable
{
    resizable_content(ui_context& ctx) : ctx_(&ctx), active_(false) {}
    resizable_content(ui_context& ctx, accessor<int> const& size,
        resizable_content_flag_set flags = NO_FLAGS)
    { begin(ctx, size, flags); }
    ~resizable_content() { end(); }
    void begin(ui_context& ctx, accessor<int> const& size,
        resizable_content_flag_set flags = NO_FLAGS);
    void end();
 private:
    ui_context* ctx_;
    bool active_;
    widget_id id_;
    resizable_content_flag_set flags_;
    int size_;
    linear_layout layout_;
};

// TABLES

struct table_style_info;

struct table : noncopyable
{
    table() {}
    table(ui_context& ctx, accessor<string> const& style,
        layout const& layout_spec = default_layout)
    { begin(ctx, style, layout_spec); }
    ~table() { end(); }
    void begin(ui_context& ctx, accessor<string> const& style,
        layout const& layout_spec = default_layout);
    void end();
 private:
    friend struct table_row;
    //friend struct table_row_background;
    friend struct table_cell;
    ui_context* ctx_;
    grid_layout grid_;
    table_style_info const* style_;
    vector<2,int> cell_index_;
};

struct table_row : noncopyable
{
    table_row() {}
    table_row(table& table, layout const& layout_spec = default_layout)
    { begin(table, layout_spec); }
    ~table_row() { end(); }
    void begin(table& table, layout const& layout_spec = default_layout);
    void end();
 private:
    friend struct table_cell;
    table* table_;
    grid_row grid_row_;
};

struct table_cell : noncopyable
{
    table_cell() {}
    table_cell(table_row& row, layout const& layout_spec = default_layout)
    { begin(row, layout_spec); }
    ~table_cell() { end(); }
    void begin(table_row& row, layout const& layout_spec = default_layout);
    void end();
 private:
    table_row* row_;
    custom_panel panel_;
    scoped_style style_;
};

// FORMS

struct form : noncopyable
{
    form() : ctx_(0) {}
    form(ui_context& ctx, layout const& layout_spec = default_layout)
    { begin(ctx, layout_spec); }
    ~form() { end(); }
    void begin(ui_context& ctx, layout const& layout_spec = default_layout);
    void end();
    ui_context& context() { return *ctx_; }
    grid_layout& grid() { return grid_; }
 private:
    ui_context* ctx_;
    grid_layout grid_;
};

void do_form_section_heading(form& form, accessor<string> const& text);

struct form_field : noncopyable
{
    form_field() : form_(0) {}
    form_field(form& form, accessor<string> const& label)
    { begin(form, label); }
    ~form_field() { end(); }
    void begin(form& form, accessor<string> const& label);
    void end();
 private:
    form* form_;
    grid_row row_;
    column_layout contents_;
};

// Provides an empty form label
struct empty_form_field : noncopyable
{
    empty_form_field() : form_(0) {}
    empty_form_field(form& form) { begin(form); }
    ~empty_form_field() { end(); }
    void begin(form& form);
    void end();
 private:
    form* form_;
    grid_row row_;
    column_layout contents_;
};

struct form_buttons : noncopyable
{
    form_buttons() : form_(0) {}
    form_buttons(form& form) { begin(form); }
    ~form_buttons() { end(); }
    void begin(form& form);
    void end();
 private:
    form* form_;
    grid_row row_;
    row_layout contents_;
};

// VALIDATION

// enforce_min(accessor, min) wraps the given accessor with validation logic
// ensuring that no values less than min are written to it.
template<class Wrapped, class Min>
struct min_validation_wrapper
  : regular_accessor<typename accessor_value_type<Wrapped>::type>
{
    min_validation_wrapper(Wrapped wrapped, Min min)
      : wrapped_(wrapped), min_(min)
    {}
    bool is_gettable() const { return wrapped_.is_gettable(); }
    typename accessor_value_type<Wrapped>::type const& get() const
    { return wrapped_.get(); }
    bool is_settable() const
    { return wrapped_.is_settable() && min_.is_gettable(); }
    void set(typename accessor_value_type<Wrapped>::type const& value) const
    {
        if (value < min_.get())
        {
            throw validation_error(
                "This value must be at least " + to_string(min_.get()) + ".");
        }
        wrapped_.set(value);
    }
 private:
    Wrapped wrapped_;
    Min min_;
};
template<class Wrapped, class Min>
min_validation_wrapper<
    typename copyable_accessor_helper<Wrapped const&>::result_type,
    typename copyable_accessor_helper<Min const&>::result_type>
enforce_min(Wrapped const& accessor, Min const& min)
{
    return
        min_validation_wrapper<
            typename copyable_accessor_helper<Wrapped const&>::result_type,
            typename copyable_accessor_helper<Min const&>::result_type>(
                make_accessor_copyable(accessor),
                make_accessor_copyable(min));
}

// enforce_max(accessor, max) is analogous to enforce_max.
template<class Wrapped, class Max>
struct max_validation_wrapper
  : regular_accessor<typename accessor_value_type<Wrapped>::type>
{
    max_validation_wrapper(Wrapped wrapped, Max max)
      : wrapped_(wrapped), max_(max)
    {}
    bool is_gettable() const { return wrapped_.is_gettable(); }
    typename accessor_value_type<Wrapped>::type const& get() const
    { return wrapped_.get(); }
    bool is_settable() const
    { return wrapped_.is_settable() && max_.is_gettable(); }
    void set(typename accessor_value_type<Wrapped>::type const& value) const
    {
        if (value > max_.get())
        {
            throw validation_error(
                "This value cannot be greater than " +
                to_string(max_.get()) + ".");
        }
        wrapped_.set(value);
    }
 private:
    Wrapped wrapped_;
    Max max_;
};
template<class Wrapped, class Max>
max_validation_wrapper<
    typename copyable_accessor_helper<Wrapped const&>::result_type,
    typename copyable_accessor_helper<Max const&>::result_type>
enforce_max(Wrapped const& accessor, Max const& max)
{
    return
        max_validation_wrapper<
            typename copyable_accessor_helper<Wrapped const&>::result_type,
            typename copyable_accessor_helper<Max const&>::result_type>(
                make_accessor_copyable(accessor),
                make_accessor_copyable(max));
}

// MENUS

struct scoped_menu_container : noncopyable
{
    scoped_menu_container() : ctx_(0) {}
    scoped_menu_container(ui_context& ctx, menu_container* container) : ctx_(0)
    { begin(ctx, container); }
    ~scoped_menu_container() { end(); }
    void begin(ui_context& ctx, menu_container* container);
    void end();
 private:
    ui_context* ctx_;
};

// menu is a scoped object that groups its children into a menu (or submenu).
struct submenu : noncopyable
{
    submenu() {}
    submenu(ui_context& ctx, accessor<string> const& label,
        accessor<bool> const& enabled = in(true))
    { begin(ctx, label, enabled); }
    ~submenu() { end(); }
    void begin(ui_context& ctx, accessor<string> const& label,
        accessor<bool> const& enabled = in(true));
    void end();
 private:
    scoped_menu_container scoping_;
};

struct menu_bar : noncopyable
{
    menu_bar() {}
    menu_bar(ui_context& ctx) { begin(ctx); }
    ~menu_bar() { end(); }
    void begin(ui_context& ctx);
    void end();
 private:
    scoped_menu_container scoping_;
};

bool do_menu_option(ui_context& ctx, accessor<string> const& label,
    accessor<bool> const& enabled = in(true));

bool do_checkable_menu_option(ui_context& ctx, accessor<string> const& label,
    accessor<bool> const& checked, accessor<bool> const& enabled = in(true));

void do_menu_separator(ui_context& ctx);

}

#endif
