#ifndef ALIA_UI_UTILTIIES_HPP
#define ALIA_UI_UTILTIIES_HPP

#include <alia/ui_definitions.hpp>

// This file provides various utilties for implementing widgets.

namespace alia {

void record_content_change(ui_context& ctx);

template<class Event>
bool detect_event(ui_context& ctx)
{
    return dynamic_cast<Event*>(ctx.event) != 0;
}

static inline bool detect_event(ui_context& ctx, ui_event_type type)
{
    return ctx.event->type == type;
}

template<class Event>
Event& get_event(ui_context& ctx)
{
    assert(detect_event<Event>(ctx));
    return static_cast<Event&>(*ctx.event);
}

widget_state get_widget_state(ui_context& ctx, widget_id id,
    bool enabled = true, bool pressed = false, bool selected = false);

// THEMED RENDERING

struct renderer_data
{
    virtual ~renderer_data() {}
};
typedef alia__shared_ptr<renderer_data> renderer_data_ptr;

template<class Interface>
struct themed_rendering_data
{
    owned_id theme_id;
    Interface const* renderer;
    renderer_data_ptr data;
};

template<class Interface, class DefaultImplementation>
static void refresh_themed_rendering_data(
    ui_context& ctx, themed_rendering_data<Interface>& data,
    DefaultImplementation const* default_implementation)
{
    // TODO: This resets the data whenever the style ID changes, which is too
    // often. It should only reset it if it's actually a different theme.
    if (!data.theme_id.matches(*ctx.style.id))
    {
        data.data.reset();
        if (!get_implementation(*ctx.style.theme, &data.renderer))
            data.renderer = default_implementation;
        data.theme_id.store(*ctx.style.id);
    }
}

template<class Data>
struct typed_renderer_data : renderer_data
{
    Data data;
};

template<class Data>
bool cast_data_ptr(Data**typed_data, renderer_data_ptr& data_ptr)
{
    if (!data_ptr)
    {
        typed_renderer_data<Data>* data = new typed_renderer_data<Data>;
        *typed_data = &data->data;
        data_ptr.reset(data);
        return true;
    }
    else
    {
        assert(dynamic_cast<typed_renderer_data<Data>*>(data_ptr.get()));
        *typed_data =
            &static_cast<typed_renderer_data<Data>*>(data_ptr.get())->data;
        return false;
    }
}

// STYLING

// Look up the value of a property in the style tree.
string const* get_style_property(
    style_tree const* tree, char const* property_name);
// Look up the value of a property in the style search path.
string const* get_style_property(
    style_search_path const* path,
    char const* property_name);

// TIMING

// This implements a one-shot timer that can be used to schedule time-dependent
// UI events.
//
// Any UI element that includes time-dependent behavior should use the
// functions provided here.  To schedule some timed behavior, call
// start_timer() and then on each pass through the control function, poll
// is_timer_done().  Once it returns true, the given time has elapsed.
// Note that is_timer_done() can only return true on a TIMER_EVENT (which is
// in the INPUT_CATEGORY).
//
void start_timer(ui_context& ctx, widget_id id, unsigned duration);
bool is_timer_done(ui_context& ctx, widget_id id);

// restart_timer() is similar to start_timer(), but it can only be invoked
// when handling a previous event (i.e, when is_timer_done() returns true).
// It adjusts the duration so that it's relative to when the event SHOULD HAVE
// occurred, rather than when it actually occurred.  This is useful for
// scheduling repeated events on a fixed frequency without drifting.
//
void restart_timer(ui_context& ctx, widget_id id, unsigned duration);

bool compute_fps(ui_context& ctx, int* fps);

// REGIONS

routable_widget_id make_routable_widget_id(ui_context& ctx, widget_id id);

//void refresh_widget_id(ui_context& ctx, widget_id id);

widget_id get_widget_id(ui_context& ctx);

static inline void get_widget_id_if_needed(ui_context& ctx, widget_id& id)
{
    if (!id)
        id = get_widget_id(ctx);
}

bool mouse_is_inside_box(ui_context& ctx, box<2,double> const& box);

void hit_test_box_region(ui_context& ctx, widget_id id,
    box<2,int> const& box, mouse_cursor cursor = DEFAULT_CURSOR);

void do_region_visibility(ui_context& ctx, widget_id id,
    box<2,int> const& box);

void do_box_region(ui_context& ctx, widget_id id, box<2,int> const& box,
    mouse_cursor cursor = DEFAULT_CURSOR);

void make_widget_visible(ui_system& ui, routable_widget_id id);

void make_widget_visible(ui_context& ctx, widget_id id);

void override_mouse_cursor(ui_context& ctx, widget_id id, mouse_cursor cursor);

// MOUSE INPUT

// Get the mouse position in the current frame of reference.
vector<2,double> get_mouse_position(ui_context& ctx);
// Same, but rounded to integer coordinates.
vector<2,int> get_integer_mouse_position(ui_context& ctx);

// Is the mouse cursor within the surface?
bool is_mouse_in_surface(ui_context& ctx);

// Check if the given mouse button is pressed.
bool is_mouse_button_pressed(ui_context& ctx, mouse_button button);

// Detect if the given ID has captured the mouse, meaning that a mouse
// button was pressed down when the mouse was over the widget with the
// given ID, and the mouse button is still down.
bool is_region_active(ui_context& ctx, widget_id id);

// Detect if the mouse is over the given region.
bool is_region_hot(ui_context& ctx, widget_id id);

// Detect if a mouse button has just been pressed.
bool detect_mouse_press(ui_context& ctx, mouse_button button);

// Detect if a mouse button has just been pressed over the given region.
bool detect_mouse_press(ui_context& ctx, widget_id id, mouse_button button);

// Detect if a mouse button has just been released.
bool detect_mouse_release(ui_context& ctx, mouse_button button);

// Detect if a mouse button has just been released over the given region.
bool detect_mouse_release(ui_context& ctx, widget_id id, mouse_button button);

// Detect if a mouse button has just been double-clicked over the given
// region.
bool detect_double_click(ui_context& ctx, widget_id id, mouse_button button);

// Detect if a mouse button has been pressed and released over the given
// region.
bool detect_click(ui_context& ctx, widget_id id, mouse_button button);

// Detect if the mouse is over the given area and the mouse could
// potentially be clicked on that area.  Unlike is_region_hot(), this
// returns false if the mouse is currently captured by something else.
bool detect_potential_click(ui_context& ctx, widget_id id);

// Detect if a mouse button is currently down over an region and was
// originally pressed down over that same region.
bool detect_click_in_progress(ui_context& ctx, widget_id id,
    mouse_button button);

// Detect if the mouse is being dragged with the given mouse button down.
bool detect_drag(ui_context& ctx, widget_id id, mouse_button button);

// If the current event is a drag, this will return the mouse movement
// represented by this event, in the current frame of reference.
vector<2,double> get_drag_delta(ui_context& ctx);

bool detect_drag_in_progress(ui_context& ctx, widget_id id,
    mouse_button button);

// Detect if the mouse has just been released after a drag.
bool detect_drag_release(ui_context& ctx, widget_id id, mouse_button button);

bool detect_mouse_motion(ui_context& ctx, widget_id id);

// Detect if a mouse button has been pressed and released over the given
// region without 'significant' movement during the click.
bool detect_stationary_click(ui_context& ctx, widget_id id,
    mouse_button button);

// Detect if the mouse is being dragged with the given mouse button down,
// but only after there has been 'sigificant' movement since the click.
bool detect_explicit_drag(ui_context& ctx, widget_id id, mouse_button button);

// Detect if the mouse has just been released after a drag.
bool detect_explicit_drag_release(ui_context& ctx, widget_id id,
    mouse_button button);

// Detect scroll wheel movement.
// The return value is positive for upward movement.
bool detect_wheel_movement(ui_context& ctx, float* movement);

// FOCUS

//void refresh_focus(ui_context& ctx, widget_id id);

void add_to_focus_order(ui_context& ctx, widget_id id);

// Determine if the given widget ID has the keyboard focus.
bool id_has_focus(ui_context& ctx, widget_id id);

// Set the widget with focus and ensure that it's visible.
void set_focus(ui_context& ctx, widget_id id);

// Set the widget with focus and ensure that it's visible.
void set_focus(ui_system& ctx, routable_widget_id id);

// Get the ID of the widget before the one with the focus.
widget_id get_id_before_focus(ui_context& ctx);

// Get the ID of the widget after the one with the focus.
widget_id get_id_after_focus(ui_context& ctx);

bool detect_focus_gain(ui_context& ctx, widget_id id);

bool detect_focus_loss(ui_context& ctx, widget_id id);

// KEYBOARD INPUT

// The following are used to detect general keyboard events related to
// any key...

// Detect if a key press just occurred and was directed at the given
// widget.  The return value indicates whether or not an event occured.
// If one did, the 'info' argument is filled with the info.
bool detect_key_press(ui_context& ctx, key_event_info* info, widget_id id);
// same, but without ID (as background)
bool detect_key_press(ui_context& ctx, key_event_info* info);

// Detect if a key release just occurred and was directed at the given
// widget.  Note that many key presses may be received before the
// corresponding (single) key release is received.
bool detect_key_release(ui_context& ctx, key_event_info* info, widget_id id);
// same, but without ID (as background)
bool detect_key_release(ui_context& ctx, key_event_info* info);

// Detect a normal ASCII character key press, as seen by the given widget.
int detect_char(ui_context& ctx, widget_id id);
// same, but without ID (as background)
int detect_char(ui_context& ctx);

// If you use any of the above detect_ functions, you need to call this
// if you actually process the event.
void acknowledge_input_event(ui_context& ctx);

// The following are used to detect specific keys... 
// The acknowledgement is done automatically.

// Detect if the given key (plus optional modifiers) was just pressed.
bool detect_key_press(ui_context& ctx, widget_id id,
    key_code code, key_modifiers modifiers = KMOD_NONE);
// same, but without ID (as background)
bool detect_key_press(ui_context& ctx, key_code code,
    key_modifiers modifiers = KMOD_NONE);

// Detect if the given key (plus optional modifiers) was just released.
bool detect_key_release(ui_context& ctx, widget_id id,
    key_code code, key_modifiers modifiers = KMOD_NONE);
// same, but without ID (as background)
bool detect_key_release(ui_context& ctx, key_code code,
    key_modifiers modifiers = KMOD_NONE);

// A keyboard click is a keyboard interface to a UI button that operates in a
// similar manner to the mouse interface. Instead of triggering immediately
// when the key is pressed, the button will be pressed down and will trigger
// when the key is released.
struct keyboard_click_state
{
    keyboard_click_state() : state(0) {}
    int state;
};
static inline bool is_pressed(keyboard_click_state const& state)
{ return state.state == 1; }
// Detect a keyboard click.
bool detect_keyboard_click(ui_context& ctx, keyboard_click_state& state,
    widget_id id, key_code code = KEY_SPACE,
    key_modifiers modifiers = KMOD_NONE);
// same, but without ID (as background)
bool detect_keyboard_click(ui_context& ctx, keyboard_click_state& state,
    key_code code = KEY_SPACE, key_modifiers modifiers = KMOD_NONE);

// STYLING

style_tree const* find_substyle(style_search_path const* path,
    string const& substyle_name);
style_tree const* find_substyle(style_search_path const* path,
    string const& substyle_name, widget_state state);

rgba8 get_color_property(
    style_tree const* tree, char const* property_name,
    rgba8 default_value = rgba8(0xff, 0xff, 0xff, 0xff));
rgba8 get_color_property(
    style_search_path const* path, char const* property_name,
    rgba8 default_value = rgba8(0xff, 0xff, 0xff, 0xff));
static inline rgba8 get_color_property(
    ui_context& ctx, char const* property_name,
    rgba8 default_value = rgba8(0xff, 0xff, 0xff, 0xff))
{ return get_color_property(ctx.style.path, property_name, default_value); }

int get_integer_property(style_tree const* tree,
    char const* property_name, int default_value);
int get_integer_property(style_search_path const* path,
    char const* property_name, int default_value);
static inline int get_integer_property(ui_context& ctx,
    char const* property_name, int default_value)
{ return get_integer_property(ctx.style.path, property_name, default_value); }

float get_float_property(style_tree const* tree,
    char const* property_name, float default_value);
float get_float_property(style_search_path const* path,
    char const* property_name, float default_value);
static inline float get_float_property(ui_context& ctx,
    char const* property_name, float default_value)
{ return get_float_property(ctx.style.path, property_name, default_value); }

string get_string_property(style_tree const* tree,
    char const* property_name, string const& default);
string get_string_property(style_search_path const* path,
    char const* property_name, string const& default);
static inline string get_string_property(ui_context& ctx,
    char const* property_name, string const& default_value)
{ return get_string_property(ctx.style.path, property_name, default_value); }

bool get_boolean_property(style_tree const* tree,
    char const* property_name, bool default_value);
bool get_boolean_property(style_search_path const* path,
    char const* property_name, bool default_value);
static inline bool get_boolean_property(ui_context& ctx,
    char const* property_name, bool default_value)
{ return get_boolean_property(ctx.style.path, property_name, default_value); }

font get_font_properties(style_search_path const* path);

// GENERAL INPUT

// The following data structure and accompanying functions implement input
// handling for simple, clickable widgets.

struct button_input_state
{
    keyboard_click_state key;
};

widget_state get_button_state(ui_context& ctx, widget_id id,
    button_input_state const& state);

// Do input processing for the button.
// This returns true iff the button was just pressed.
bool do_button_input(ui_context& ctx, widget_id id, button_input_state& state);

// VALUE EVENTS

struct set_value_event : ui_event
{
    set_value_event(widget_id target, untyped_ui_value* value)
      : ui_event(NO_CATEGORY, SET_VALUE_EVENT)
      , value(value), target(target) {}
    alia__shared_ptr<untyped_ui_value> value;
    widget_id target;
};

template<class T>
void handle_set_value_events(ui_context& ctx, widget_id id,
    accessor<T> const& accessor)
{
    if (detect_event(ctx, SET_VALUE_EVENT))
    {
        set_value_event& e = get_event<set_value_event>(ctx);
        if (e.target == id)
        {
            // A dynamic_cast isn't really necessary here, but it's possible
            // that some bug coudlc ause an event could get sent with the wrong
            // type or to the wrong ID, and since this gets executed so
            // infrequently, it's better to just be safe.
            typed_ui_value<T>* typed_value =
                dynamic_cast<typed_ui_value<T>*>(e.value.get());
            assert(typed_value);
            if (typed_value)
                set(accessor, typed_value->value);
        }
    }
}

void issue_targeted_event(ui_system& system, ui_event& event,
    routable_widget_id const& target);

template<class T>
void issue_set_value_event(ui_context& ctx, widget_id id, T const& new_value)
{
    typed_ui_value<T>* value = new typed_ui_value<T>;
    value->value = new_value;
    set_value_event e(id, value);
    issue_targeted_event(*ctx.system, e, make_routable_widget_id(ctx, id));
}

}

#endif
