#ifndef ALIA_UI_DEFINITIONS_HPP
#define ALIA_UI_DEFINITIONS_HPP

#include <alia/data_graph.hpp>
#include <alia/layout_system.hpp>
#include <alia/rendering.hpp>
#include <alia/event_routing.hpp>
#include <alia/ui_interface.hpp>
#include <alia/dispatch_table.hpp>
#include <map>

namespace alia {

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
    OVERLAY_RENDER_EVENT,

    // regions
    HIT_TEST_EVENT,
    MAKE_WIDGET_VISIBLE_EVENT,

    // keyboard
    CHAR_EVENT,
    BACKGROUND_CHAR_EVENT,
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
    render_event(ui_event_type type)
      : ui_event(RENDER_CATEGORY, type)
      , active(false)
    {}
    bool active;
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
typedef unsigned char_type;
struct char_event : input_event
{
    char_type character;

    char_event(ui_time_type time, char_type character)
      : input_event(CHAR_EVENT, time), character(character)
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
    float movement;
    mouse_wheel_event(ui_time_type time, float movement)
      : input_event(MOUSE_WHEEL_EVENT, time)
      , movement(movement)
    {}
};
struct mouse_notification_event : ui_event
{
    mouse_notification_event(ui_event_type type)
      : ui_event(INPUT_CATEGORY, type)
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

struct widget_data
{
    //counter_type last_refresh;
    //widget_data() : last_refresh(0) {}
};

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

static inline bool is_valid(routable_widget_id const& id)
{ return id.id != 0; }

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

    routable_widget_id overlay;

    ui_system() : millisecond_tick_count(0) {}
};

static inline surface& get_surface(ui_context& ctx)
{ return *ctx.surface; }

static inline bool is_rendering_active(ui_context& ctx)
{
    return static_cast<render_event&>(*ctx.event).active == true;
}

static inline bool is_rendering(ui_context& ctx)
{
    return ctx.event->category == RENDER_CATEGORY &&
        is_rendering_active(ctx);
}

struct primary_style_properties
{
    alia::font font;
    rgba8 text_color, bg_color, selected_text_color, selected_bg_color,
        border_color;
};

struct hit_test_event : ui_event
{
    vector<2,double> position;
    routable_widget_id id;
    mouse_cursor cursor;

    hit_test_event(vector<2,double> const& position)
      : ui_event(REGION_CATEGORY, HIT_TEST_EVENT), position(position),
        id(null_widget_id), cursor(DEFAULT_CURSOR)
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

// If there is an active widget and it's not the one under the mouse cursor,
// we have to query it to see what cursor it wants.
struct mouse_cursor_query : ui_event
{
    widget_id id;
    mouse_cursor cursor;

    mouse_cursor_query(widget_id id)
      : ui_event(INPUT_CATEGORY, MOUSE_CURSOR_QUERY_EVENT), id(id),
        cursor(DEFAULT_CURSOR)
    {}
};

}

#endif
