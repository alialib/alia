#include <alia/ui_system.hpp>
#include <alia/ui_utilities.hpp>
#include <boost/lexical_cast.hpp>

#include <windows.h>

namespace alia {

static void on_ui_style_change(ui_system& system)
{
    inc_version(system.style->id);
}

void set_text_magnification(ui_system& system, float magnification)
{
    on_ui_style_change(system);
    system.style->text_magnification = magnification;
}

rgba8 get_color_property(style_tree const* tree,
    char const* property_name, rgba8 default_value)
{
    string const* value = get_style_property(tree, property_name);
    return value ? parse_color_value(value->c_str()) : default_value;
}
rgba8 get_color_property(style_search_path const* path,
    char const* property_name, rgba8 default_value)
{
    string const* value = get_style_property(path, property_name);
    return value ? parse_color_value(value->c_str()) : default_value;
}

int get_integer_property(style_tree const* tree,
    char const* property_name, int default_value)
{
    string const* value = get_style_property(tree, property_name);
    return value ? boost::lexical_cast<int>(value->c_str()) : default_value;
}
int get_integer_property(style_search_path const* path,
    char const* property_name, int default_value)
{
    string const* value = get_style_property(path, property_name);
    return value ? boost::lexical_cast<int>(value->c_str()) : default_value;
}

float get_float_property(style_tree const* tree,
    char const* property_name, float default_value)
{
    string const* value = get_style_property(tree, property_name);
    return value ? boost::lexical_cast<float>(value->c_str()) : default_value;
}
float get_float_property(style_search_path const* path,
    char const* property_name, float default_value)
{
    string const* value = get_style_property(path, property_name);
    return value ? boost::lexical_cast<float>(value->c_str()) : default_value;
}

string get_string_property(style_tree const* tree,
    char const* property_name, string const& default)
{
    string const* value = get_style_property(tree, property_name);
    return value ? *value : default;
}
string get_string_property(style_search_path const* path,
    char const* property_name, string const& default)
{
    string const* value = get_style_property(path, property_name);
    return value ? *value : default;
}

bool get_boolean_property(style_tree const* tree,
    char const* property_name, bool default_value)
{
    string const* value = get_style_property(tree, property_name);
    return value ? *value == "true" : default_value;
}
bool get_boolean_property(style_search_path const* path,
    char const* property_name, bool default_value)
{
    string const* value = get_style_property(path, property_name);
    return value ? *value == "true" : default_value;
}

font get_font_properties(ui_system const& ui, style_search_path const* path)
{
    return font(
        get_string_property(path, "font_name", "arial"),
        get_float_property(path, "font_size", 13) *
            ui.style->text_magnification,
        (get_boolean_property(path, "font_bold", false) ?
            BOLD : NO_FLAGS) |
        (get_boolean_property(path, "font_italic", false) ?
            ITALIC : NO_FLAGS) |
        (get_boolean_property(path, "font_underline", false) ?
            UNDERLINE : NO_FLAGS) |
        (get_boolean_property(path, "font_strikethrough", false) ?
            STRIKETHROUGH : NO_FLAGS));
}

static void read_primary_style_properties(
    ui_system const& ui,
    primary_style_properties* props,
    style_search_path const* path)
{
    props->text_color =
        get_color_property(path, "text_color", rgba8(0x00, 0x00, 0x00, 0xff));
    props->bg_color =
        get_color_property(path, "background_color",
            rgba8(0xff, 0xff, 0xff, 0xff));
    props->selected_text_color =
        get_color_property(path, "selected_text_color",
            rgba8(0xff, 0xff, 0xff, 0xff));
    props->selected_bg_color =
        get_color_property(path, "selected_background_color",
            rgba8(0x32, 0x97, 0xfd, 0xff));
    props->border_color =
        get_color_property(path, "border_color",
            rgba8(0x80, 0x80, 0x80, 0xff));
    props->font = get_font_properties(ui, path);
}

static void read_layout_style_info(layout_style_info* style_info,
    font const& font, style_search_path const* path)
{
    style_info->is_padded = get_boolean_property(path, "padded", false);
    layout_scalar padding_size =
        as_layout_size(get_float_property(path, "padding_size", 0.2f) *
            font.size);
    style_info->padding_size = make_vector(padding_size, padding_size);

    style_info->font_size = font.size;
    // TODO
    style_info->character_size = make_vector(
        style_info->font_size * 0.6f, style_info->font_size);
    style_info->x_height = style_info->font_size * 0.5f;
}

struct initial_styling
{
    initial_styling(ui_context& ctx)
    {
        get_data(ctx, &path_);
        path_->rest = 0;
        path_->tree = &ctx.system->style->styles;

        read_primary_style_properties(*ctx.system, &props_, path_);

        id_ = get_id(ctx.system->style->id);

        ctx.style.path = path_;
        ctx.style.properties = &props_;
        ctx.style.id = &id_;
        ctx.style.theme = &ctx.system->style->theme;

        read_layout_style_info(&style_info_, props_.font, path_);
        ctx.layout.style_info = &style_info_;
    }
 private:
    primary_style_properties props_;
    style_search_path* path_;
    value_id_by_reference<local_id> id_;
    layout_style_info style_info_;
};

// focus events

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

//struct focus_recovery_event : ui_event
//{
//    widget_id id;
//
//    focus_recovery_event()
//      : ui_event(INPUT_CATEGORY, FOCUS_RECOVERY_EVENT), id(0)
//    {}
//};

static routable_widget_id
get_focus_successor(ui_system& ui, widget_id input_id)
{
    focus_successor_event e(input_id);
    // Initialize just_saw_input to true.
    // In cases where the input_id is null, this will cause the first widget to
    // become the successor.
    // (And in other cases, the correct widget will simply overwrite the first
    // one.)
    e.just_saw_input = true;
    issue_event(ui, e);
    return e.successor;
}

static routable_widget_id
get_focus_successor_no_wrap(ui_system& ui, widget_id input_id)
{
    focus_successor_event e(input_id);
    issue_event(ui, e);
    return e.successor;
}

static routable_widget_id
get_focus_predecessor(ui_system& ui, widget_id input_id)
{
    focus_predecessor_event e(input_id);
    issue_event(ui, e);
    return e.predecessor;
}

//static void recover_focus(ui_system& ui)
//{
//    focus_recovery_event e;
//    issue_event(ui, e);
//    set_focus(ui, get_focus_successor_no_wrap(ui, e.id));
//}

struct end_pass_exception {};

static void issue_event(
    ui_system& system, ui_event& event,
    bool targeted, routing_region_ptr const& target = routing_region_ptr())
{
    ui_context ctx;
    ctx.pass_aborted = false;
    ctx.system = &system;
    scoped_data_traversal data(system.data, ctx.data);
    bool is_refresh = event.type == REFRESH_EVENT;
    vector<2,double> size =
        is_refresh ? make_vector<double>(0, 0) :
            vector<2,double>(system.surface->size());
    initialize(ctx.geometry, box<2,double>(make_vector<double>(0, 0), size));
    ctx.surface = system.surface.get();
    if (event.category == RENDER_CATEGORY)
        set_subscriber(ctx.geometry, *ctx.surface);
    scoped_layout_refresh refresh;
    scoped_layout_traversal layout;
    if (is_refresh)
    {
        refresh.begin(system.layout, ctx.layout, ctx.data,
            system.surface->ppi());
    }
    else
    {
        layout.begin(system.layout, ctx.layout, ctx.data, ctx.geometry,
            system.surface->ppi());
    }
    ctx.event = &event;
    scoped_event_routing_traversal sert(ctx.routing, ctx.data, true, target);
    initial_styling styling(ctx);
    ctx.active_cacher = 0;
    try
    {
        system.controller->do_ui(ctx);
    }
    catch (end_pass_exception&)
    {
    }
}

void end_pass(ui_context& ctx)
{
    ctx.pass_aborted = true;
    throw end_pass_exception();
}

layout_vector measure_initial_ui(
    alia__shared_ptr<ui_controller> const& controller,
    alia__shared_ptr<ui_style> const& style,
    alia__shared_ptr<surface> const& surface)
{
    ui_system tmp;
    tmp.controller = controller;
    tmp.style = style;
    tmp.surface = surface;

    refresh_event e;
    issue_event(tmp, e, false);

    return get_minimum_size(tmp.layout);
}

///

//void scoped_text_processor::begin(
//    ui_context& ctx, ui_text_processor* processor)
//{
//    ctx_ = &ctx;
//    old_processor_ = ctx.active_text_processor;
//    ctx.active_text_processor = processor;
//}
//void scoped_text_processor::end()
//{
//    if (ctx_)
//    {
//        ctx_->active_text_processor = old_processor_;
//        ctx_ = 0;
//    }
//}

void render_ui(ui_system& system)
{
    render_event e(RENDER_EVENT);
    e.active = true;
    issue_event(system, e, false);

    if (is_valid(system.overlay))
    {
        render_event e(OVERLAY_RENDER_EVENT);
        e.active = false;
        issue_targeted_event(system, e, system.overlay);
    }
}

void refresh_and_layout(ui_system& system, vector<2,unsigned> const& size,
    ui_time_type millisecond_tick_count)
{
    system.millisecond_tick_count = millisecond_tick_count;

    refresh_event e;
    issue_event(system, e, false);

    resolve_layout(system.layout, layout_vector(size));

    //if (is_valid(system.input.focused_id) && !e.saw_focus)
    //    recover_focus(system);
}

void issue_event(ui_system& system, ui_event& event)
{
    issue_event(system, event, false);
}

void issue_targeted_event(ui_system& system, ui_event& event,
    routable_widget_id const& target)
{
    issue_event(system, event, true, target.region);
}

routable_widget_id do_hit_test(ui_system& ui, vector<2,double> const& position)
{
    hit_test_event e(position);
    issue_event(ui, e);
    return e.id;
}

static routable_widget_id get_mouse_target(ui_system& ui)
{
    return is_valid(ui.input.active_id) ? ui.input.active_id : ui.input.hot_id;
}

optional<mouse_cursor> update_mouse_cursor(ui_system& ui)
{
    routable_widget_id previous_mouse_target = get_mouse_target(ui);

    optional<mouse_cursor> cursor;

    if (ui.input.mouse_inside_window)
    {
        hit_test_event hit_test(vector<2,double>(ui.input.mouse_position));
        issue_event(ui, hit_test);
        ui.input.hot_id = hit_test.id;
        cursor = hit_test.cursor;
    }
    else
        ui.input.hot_id = null_widget_id;

    if (ui.input.active_id.id && ui.input.active_id.id != ui.input.hot_id.id)
    {
        mouse_cursor_query query(ui.input.active_id.id);
        issue_targeted_event(ui, query, ui.input.active_id);
        cursor = query.cursor;
    }

    routable_widget_id current_mouse_target = get_mouse_target(ui);
    if (current_mouse_target.id != previous_mouse_target.id)
    {
        {
            mouse_notification_event e(MOUSE_LOSS_EVENT);
            issue_targeted_event(ui, e, previous_mouse_target);
        }
        {
            mouse_notification_event e(MOUSE_GAIN_EVENT);
            issue_targeted_event(ui, e, current_mouse_target);
        }
    }

    return cursor;
}

void process_mouse_move(ui_system& ui, ui_time_type time,
    vector<2,int> const& position)
{
    if (!ui.input.mouse_inside_window || ui.input.mouse_position != position)
    {
        mouse_motion_event e(time, ui.input.mouse_position,
            ui.input.mouse_inside_window);
        ui.input.mouse_position = position;
        ui.input.mouse_inside_window = true;
        issue_targeted_event(ui, e, get_mouse_target(ui));
    }
}
void process_mouse_leave(ui_system& ui, ui_time_type time)
{
    ui.input.mouse_inside_window = false;
}
void process_mouse_press(ui_system& ui, ui_time_type time,
    vector<2,int> const& position, mouse_button button)
{
    ui.input.mouse_button_state |= 1 << int(button);
    routable_widget_id target = get_mouse_target(ui);
    {
        mouse_button_event e(MOUSE_PRESS_EVENT, time, button);
        issue_targeted_event(ui, e, target);
    }
    if (!is_valid(target))
        clear_focus(ui);
    ui.input.keyboard_interaction = false;
}
void process_mouse_release(ui_system& ui, ui_time_type time,
    vector<2,int> const& position, mouse_button button)
{
    ui.input.mouse_button_state &= ~(1 << int(button));
    mouse_button_event e(MOUSE_RELEASE_EVENT, time, button);
    issue_targeted_event(ui, e, get_mouse_target(ui));
    if (ui.input.mouse_button_state == 0)
        ui.input.active_id = null_widget_id;
}
void process_double_click(ui_system& ui, ui_time_type time,
    vector<2,int> const& position, mouse_button button)
{
    ui.input.mouse_button_state |= 1 << int(button);
    mouse_button_event e(DOUBLE_CLICK_EVENT, time, button);
    issue_targeted_event(ui, e, get_mouse_target(ui));
    ui.input.keyboard_interaction = false;
}
void process_mouse_wheel(ui_system& ui, ui_time_type time, float movement)
{
    mouse_wheel_event e(time, movement);
    issue_event(ui, e);
}

bool process_text_input(ui_system& ui, ui_time_type time,
    utf8_string const& text)
{
    text_input_event e(time, text);
    if (is_valid(ui.input.focused_id))
        issue_targeted_event(ui, e, ui.input.focused_id);
    if (!e.acknowledged)
    {
        e.type = BACKGROUND_TEXT_INPUT_EVENT;
        issue_event(ui, e);
    }
    return e.acknowledged;
}

bool process_key_press(ui_system& ui, ui_time_type time,
    key_event_info const& info)
{
    key_event e(KEY_PRESS_EVENT, time, info);
    if (is_valid(ui.input.focused_id))
        issue_targeted_event(ui, e, ui.input.focused_id);
    if (!e.acknowledged)
    {
        e.type = BACKGROUND_KEY_PRESS_EVENT;
        issue_event(ui, e);
    }
    ui.input.keyboard_interaction = true;
    return e.acknowledged;
}
bool process_key_release(ui_system& ui, ui_time_type time,
    key_event_info const& info)
{
    key_event e(KEY_RELEASE_EVENT, time, info);
    if (is_valid(ui.input.focused_id))
        issue_targeted_event(ui, e, ui.input.focused_id);
    if (!e.acknowledged)
    {
        e.type = BACKGROUND_KEY_RELEASE_EVENT;
        issue_event(ui, e);
    }
    return e.acknowledged;
}

void acknowledge_input_event(ui_context& ctx)
{
    get_event<input_event>(ctx).acknowledged = true;
}

void process_focus_loss(ui_system& ui, ui_time_type time)
{
    if (ui.input.window_has_focus)
    {
        if (is_valid(ui.input.focused_id))
        {
            focus_notification_event e(FOCUS_LOSS_EVENT,
                ui.input.focused_id.id);
            issue_targeted_event(ui, e, ui.input.focused_id);
        }
        ui.input.window_has_focus = false;
    }
}
void process_focus_gain(ui_system& ui, ui_time_type time)
{
    if (!ui.input.window_has_focus)
    {
        if (is_valid(ui.input.focused_id))
        {
            focus_notification_event e(FOCUS_GAIN_EVENT,
                ui.input.focused_id.id);
            issue_targeted_event(ui, e, ui.input.focused_id);
        }
        ui.input.window_has_focus = true;
    }
}

///

// WIDGET IDS

routable_widget_id make_routable_widget_id(ui_context& ctx, widget_id id)
{
    return routable_widget_id(id, get_active_region(ctx.routing));
}

//void refresh_widget_id(ui_context& ctx, widget_id id)
//{
//    id->last_refresh = ctx.layout.refresh_counter;
//}

widget_id get_widget_id(ui_context& ctx)
{
    widget_id id;
    get_data(ctx, &id);
    //if (is_refresh_pass(ctx))
    //    refresh_widget_id(ctx, id);
    return id;
}

// REGIONS

void hit_test_box_region(ui_context& ctx, widget_id id, box<2,int> const& box,
    mouse_cursor cursor)
{
    if (mouse_is_inside_box(ctx, alia::box<2,double>(box)))
    {
        hit_test_event& e = get_event<hit_test_event>(ctx);
        e.id = make_routable_widget_id(ctx, id);
        e.cursor = cursor;
    }
}

void do_region_visibility(ui_context& ctx, widget_id id,
    box<2,int> const& region)
{
    make_widget_visible_event& e = get_event<make_widget_visible_event>(ctx);
    if (e.id == id)
    {
        // TODO: This doesn't handle rotations properly.
        e.region = box<2,double>(
            transform(ctx.geometry.transformation_matrix,
                vector<2,double>(region.corner -
                    ctx.layout.style_info->padding_size)),
            vector<2,double>(region.size +
                ctx.layout.style_info->padding_size * 2));
        e.acknowledged = true;
    }
}

void do_box_region(ui_context& ctx, widget_id id, box<2,int> const& region,
    mouse_cursor cursor)
{
    switch (ctx.event->type)
    {
     case HIT_TEST_EVENT:
        hit_test_box_region(ctx, id, region, cursor);
        break;
     case MAKE_WIDGET_VISIBLE_EVENT:
        do_region_visibility(ctx, id, region);
        break;
    }
}

void override_mouse_cursor(ui_context& ctx, widget_id id, mouse_cursor cursor)
{
    if (ctx.event->type == HIT_TEST_EVENT)
    {
        hit_test_event& e = get_event<hit_test_event>(ctx);
        if (e.id.id == id)
            e.cursor = cursor;
    }
}

// MOUSE INPUT

vector<2,double> get_mouse_position(ui_context& ctx)
{
    return transform(inverse(ctx.geometry.transformation_matrix),
        vector<2,double>(ctx.system->input.mouse_position));
}
vector<2,int> get_integer_mouse_position(ui_context& ctx)
{
    vector<2,double> dp = get_mouse_position(ctx);
    return make_vector<int>(int(dp[0] + 0.5), int(dp[1] + 0.5));
}

bool mouse_is_inside_box(ui_context& ctx, box<2,double> const& box)
{
    return
        ctx.system->input.mouse_inside_window &&
        is_inside(box, get_mouse_position(ctx)) &&
        is_inside(ctx.geometry.clip_region,
            vector<2,double>(ctx.system->input.mouse_position));
}

bool is_mouse_button_pressed(ui_context& ctx, mouse_button button)
{
    return (ctx.system->input.mouse_button_state & (1 << int(button))) != 0;
}

bool is_region_active(ui_context& ctx, widget_id id)
{
    return ctx.system->input.active_id.id == id;
}
bool is_region_hot(ui_context& ctx, widget_id id)
{
    return ctx.system->input.hot_id.id == id;
}

bool detect_mouse_press(ui_context& ctx, mouse_button button)
{
    return (detect_event(ctx, MOUSE_PRESS_EVENT) ||
        detect_event(ctx, DOUBLE_CLICK_EVENT)) &&
        get_event<mouse_button_event>(ctx).button == button;
}

void process_mouse_notifications(ui_context& ctx, widget_id id)
{
    if (ctx.event->type == MOUSE_GAIN_EVENT ||
        ctx.event->type == MOUSE_LOSS_EVENT)
    {
        record_content_change(ctx);
    }
}

bool detect_mouse_press(ui_context& ctx, widget_id id, mouse_button button)
{
    process_mouse_notifications(ctx, id);
    if (detect_mouse_press(ctx, button) && is_region_hot(ctx, id))
    {
        ctx.system->input.active_id = make_routable_widget_id(ctx, id);
        return true;
    }
    else
        return false;
}

bool detect_mouse_release(ui_context& ctx, mouse_button button)
{
    return detect_event(ctx, MOUSE_RELEASE_EVENT) &&
        get_event<mouse_button_event>(ctx).button == button;
}
bool detect_mouse_release(ui_context& ctx, widget_id id,
    mouse_button button)
{
    process_mouse_notifications(ctx, id);
    return detect_mouse_release(ctx, button) && is_region_active(ctx, id);
}

bool detect_mouse_motion(ui_context& ctx, widget_id id)
{
    return detect_event(ctx, MOUSE_MOTION_EVENT) && is_region_hot(ctx, id);
}

bool detect_double_click(ui_context& ctx, widget_id id, mouse_button button)
{
    process_mouse_notifications(ctx, id);
    return detect_event(ctx, DOUBLE_CLICK_EVENT) &&
        get_event<mouse_button_event>(ctx).button == button &&
        is_region_hot(ctx, id);
}
bool detect_click(ui_context& ctx, widget_id id, mouse_button button)
{
    detect_mouse_press(ctx, id, button);
    return detect_mouse_release(ctx, id, button) && is_region_active(ctx, id);
}

bool detect_potential_click(ui_context& ctx, widget_id id)
{
    return is_region_hot(ctx, id) && is_region_active(ctx, 0);
}

bool detect_click_in_progress(ui_context& ctx, widget_id id,
    mouse_button button)
{
    return is_region_hot(ctx, id) && is_region_active(ctx, id) &&
        is_mouse_button_pressed(ctx, button);
}

bool detect_drag(ui_context& ctx, widget_id id, mouse_button button)
{
    detect_mouse_press(ctx, id, button);
    return detect_event(ctx, MOUSE_MOTION_EVENT) &&
        is_mouse_button_pressed(ctx, button) && is_region_active(ctx, id);
}

vector<2,double> get_drag_delta(ui_context& ctx)
{
    mouse_motion_event& e = get_event<mouse_motion_event>(ctx);
    matrix<3,3,double> m = inverse(ctx.geometry.transformation_matrix);
    return transform(m, vector<2,double>(ctx.system->input.mouse_position)) -
        transform(m, vector<2,double>(e.last_mouse_position));
}

bool detect_drag_in_progress(ui_context& ctx, widget_id id,
    mouse_button button)
{
    return is_mouse_button_pressed(ctx, button) && is_region_active(ctx, id);
}

bool detect_drag_release(ui_context& ctx, widget_id id, mouse_button button)
{
    return detect_mouse_release(ctx, button) && is_region_active(ctx, id);
}

bool detect_wheel_movement(ui_context& ctx, float* movement)
{
    if (ctx.event->type == MOUSE_WHEEL_EVENT)
    {
        mouse_wheel_event& e = get_event<mouse_wheel_event>(ctx);
        if (!e.acknowledged)
        {
            *movement = e.movement;
            return true;
        }
    }
    return false;
}

// KEYBOARD

void clear_focus(ui_system& ui)
{
    ui.input.focused_id = null_widget_id;
}

bool id_has_focus(ui_context& ctx, widget_id id)
{
    return ctx.system->input.focused_id.id == id;
}

bool detect_focus_gain(ui_context& ctx, widget_id id)
{
    if (detect_event(ctx, FOCUS_GAIN_EVENT))
    {
        focus_notification_event& e = get_event<focus_notification_event>(ctx);
        if (e.target == id)
            return true;
    }
    return false;
}

bool detect_focus_loss(ui_context& ctx, widget_id id)
{
    if (detect_event(ctx, FOCUS_LOSS_EVENT))
    {
        focus_notification_event& e = get_event<focus_notification_event>(ctx);
        if (e.target == id)
            return true;
    }
    return false;
}

//void refresh_focus(ui_context& ctx, widget_id id)
//{
//    refresh_event& e = get_event<refresh_event>(ctx);
//    if (id_has_focus(ctx, id))
//        e.saw_focus = true;
//    id->focus_bits |= e.saw_focus ? 0 : 1;
//}

void add_to_focus_order(ui_context& ctx, widget_id id)
{
    switch (ctx.event->type)
    {
     case FOCUS_PREDECESSOR_EVENT:
      {
        focus_predecessor_event& e = get_event<focus_predecessor_event>(ctx);
        if (e.input_id == id && is_valid(e.predecessor))
            e.saw_input = true;
        if (!e.saw_input)
            e.predecessor = make_routable_widget_id(ctx, id);
        break;
      }
     case FOCUS_SUCCESSOR_EVENT:
      {
        focus_successor_event& e = get_event<focus_successor_event>(ctx);
        if (e.just_saw_input)
        {
            e.successor = make_routable_widget_id(ctx, id);
            e.just_saw_input = false;
        }
        if (e.input_id == id)
            e.just_saw_input = true;
        break;
      }
     //case FOCUS_RECOVERY_EVENT:
     // {
     //   focus_recovery_event& e = get_event<focus_recovery_event>(ctx);
     //   if ((id->focus_bits & 2) != 0)
     //       e.id = id;
     //   break;
     // }
    }
}

void advance_focus(ui_system& ui)
{
    set_focus(ui, get_focus_successor(ui, ui.input.focused_id.id));
}

void regress_focus(ui_system& ui)
{
    set_focus(ui, get_focus_predecessor(ui, ui.input.focused_id.id));
}

void make_widget_visible(ui_system& ui, routable_widget_id id)
{
    make_widget_visible_event e(id.id);
    issue_targeted_event(ui, e, id);
}

void make_widget_visible(ui_context& ctx, widget_id id)
{
    make_widget_visible(*ctx.system, make_routable_widget_id(ctx, id));
}

void set_focus(ui_system& ui, routable_widget_id id)
{
    bool different = ui.input.focused_id.id != id.id;
    if (different && is_valid(ui.input.focused_id))
    {
        focus_notification_event e(FOCUS_LOSS_EVENT, ui.input.focused_id.id);
        issue_targeted_event(ui, e, ui.input.focused_id);
    }
    ui.input.focused_id = id;
    if (different && is_valid(id))
    {
        make_widget_visible(ui, id);
        focus_notification_event e(FOCUS_GAIN_EVENT, id.id);
        issue_targeted_event(ui, e, id);
    }
}

// Calling this ensure that a widget will steal the focus if it's click on.
static void do_click_focus(ui_context& ctx, widget_id id)
{
    if (detect_event(ctx, MOUSE_PRESS_EVENT) && is_region_hot(ctx, id))
        set_focus(ctx, id);
}

static bool detect_key_event(ui_context& ctx, key_event_info* info,
    ui_event_type event_type)
{
    if (detect_event(ctx, event_type))
    {
        key_event& e = get_event<key_event>(ctx);
        if (!e.acknowledged)
        {
            *info = e.info;
            return true;
        }
    }
    return false;
}

bool detect_key_press(ui_context& ctx, key_event_info* info, widget_id id)
{
    do_click_focus(ctx, id);
    return id_has_focus(ctx, id) &&
        detect_key_event(ctx, info, KEY_PRESS_EVENT);
}
bool detect_key_press(ui_context& ctx, key_event_info* info)
{
    return detect_key_event(ctx, info, BACKGROUND_KEY_PRESS_EVENT);
}

bool detect_key_release(ui_context& ctx, key_event_info* info, widget_id id)
{
    do_click_focus(ctx, id);
    return id_has_focus(ctx, id) &&
        detect_key_event(ctx, info, KEY_RELEASE_EVENT);
}
bool detect_key_release(ui_context& ctx, key_event_info* info)
{
    return detect_key_event(ctx, info, BACKGROUND_KEY_RELEASE_EVENT);
}

static bool detect_text_input(ui_context& ctx, utf8_string* text,
    ui_event_type event_type)
{
    if (detect_event(ctx, event_type))
    {
        text_input_event& e = get_event<text_input_event>(ctx);
        if (!e.acknowledged)
        {
            *text = e.text;
            return true;
        }
    }
    return false;
}

bool detect_text_input(ui_context& ctx, utf8_string* text, widget_id id)
{
    do_click_focus(ctx, id);
    return id_has_focus(ctx, id) &&
        detect_text_input(ctx, text, TEXT_INPUT_EVENT);
}
bool detect_text_input(ui_context& ctx, utf8_string* text)
{
    return detect_text_input(ctx, text, BACKGROUND_TEXT_INPUT_EVENT);
}

bool detect_key_press(ui_context& ctx, widget_id id,
    key_code code, key_modifiers modifiers)
{
    key_event_info info;
    if (detect_key_press(ctx, &info, id) && info.code == code &&
        info.mods == modifiers)
    {
        acknowledge_input_event(ctx);
        return true;
    }
    return false;
}

bool detect_key_press(ui_context& ctx, key_code code, key_modifiers modifiers)
{
    key_event_info info;
    if (detect_key_press(ctx, &info) && info.code == code &&
        info.mods == modifiers)
    {
        acknowledge_input_event(ctx);
        return true;
    }
    return false;
}

bool detect_key_release(ui_context& ctx, widget_id id,
    key_code code, key_modifiers modifiers)
{
    key_event_info info;
    if (detect_key_release(ctx, &info, id) && info.code == code &&
        info.mods == modifiers)
    {
        acknowledge_input_event(ctx);
        return true;
    }
    return false;
}

bool detect_key_release(ui_context& ctx, key_code code,
    key_modifiers modifiers)
{
    key_event_info info;
    if (detect_key_release(ctx, &info) && info.code == code &&
        info.mods == modifiers)
    {
        acknowledge_input_event(ctx);
        return true;
    }
    return false;
}

void set_focus(ui_context& ctx, widget_id id)
{
    set_focus(*ctx.system, make_routable_widget_id(ctx, id));
}

bool detect_keyboard_click(ui_context& ctx, keyboard_click_state& state,
    widget_id id, key_code code, key_modifiers modifiers)
{
    key_event_info info;
    if (detect_key_press(ctx, &info, id))
    {
        if (info.code == code && info.mods == modifiers)
        {
            if (state.state == 0)
                state.state = 1;
            acknowledge_input_event(ctx);
        }
        else if (state.state == 1)
            state.state = 2;
    }
    else if (detect_key_release(ctx, id, code, modifiers))
    {
        bool proper = state.state == 1;
        state.state = 0;
        return proper;
    }
    return false;
}
bool detect_keyboard_click(ui_context& ctx, keyboard_click_state& state,
    key_code code, key_modifiers modifiers)
{
    key_event_info info;
    if (detect_key_press(ctx, &info))
    {
        if (info.code == code && info.mods == modifiers)
        {
            if (state.state == 0)
                state.state = 1;
            acknowledge_input_event(ctx);
        }
        else if (state.state == 1)
            state.state = 2;
    }
    else if (detect_key_release(ctx, code, modifiers))
    {
        bool proper = state.state == 1;
        state.state = 0;
        return proper;
    }
    return false;
}

// CULLING

void culling_block::begin(ui_context& ctx)
{
    srr_.begin(ctx.routing);
}
void culling_block::end()
{
    srr_.end();
}

// STYLING

string const* get_style_property(
    style_tree const* tree, char const* property_name)
{
    if (tree)
    {
        property_map::const_iterator i = tree->properties.find(property_name);
        if (i != tree->properties.end())
            return &i->second;
    }
    return 0;
}

string const* get_style_property(
    style_search_path const* path, char const* property_name)
{
    while (path)
    {
        style_tree const& tree = *path->tree;
        property_map::const_iterator i = tree.properties.find(property_name);
        if (i != tree.properties.end())
            return &i->second;
        path = path->rest;
    }
    return 0;
}

rgba8 parse_color_value(char const* value)
{
    if (*value != '#')
        throw parse_error("invalid hex color constant");
    ++value;

    uint8_t digits[8];
    bool has_alpha;
    for (int i = 0; ; ++i)
    {
        char c = value[i];
        if (c == '\0')
        {
            switch (i)
            {
             case 6:
                has_alpha = false;
                break;
             case 8:
                has_alpha = true;
                break;
             default:
                throw parse_error("invalid hex color constant");
            }
            break;
        }
        else if (i >= 8)
            throw parse_error("invalid hex color constant");
        else if (c >= '0' && c <= '9')
            digits[i] = uint8_t(c - '0');
        else if (c >= 'a' && c <= 'f')
            digits[i] = 10 + uint8_t(c - 'a');
        else if (c >= 'A' && c <= 'F')
            digits[i] = 10 + uint8_t(c - 'A');
        else
            throw parse_error("invalid hex color constant");
    }

    alia::rgba8 color;
    color.r = (digits[0] << 4) + digits[1];
    color.g = (digits[2] << 4) + digits[3];
    color.b = (digits[4] << 4) + digits[5];
    color.a = has_alpha ? ((digits[6] << 4) + digits[7]) : 0xff;
    return color;
}

static string widget_state_string(widget_state state)
{
    string s;
    switch (state.code & WIDGET_PRIMARY_STATE_MASK_CODE)
    {
     case WIDGET_DISABLED_CODE:
        s = ".disabled";
        break;
     case WIDGET_HOT_CODE:
        s = ".hot";
        break;
     case WIDGET_DEPRESSED_CODE:
        s = ".depressed";
        break;
     case WIDGET_SELECTED_CODE:
        s = ".selected";
        break;
     case WIDGET_NORMAL_CODE:
     default:
        // the normal state has no specific string associated with it
        ;
    }
    if (state & WIDGET_FOCUSED)
        s += ".focused";
    return s;
}

style_tree const* find_substyle(style_search_path const* path,
    string const& substyle_name)
{
    while (path)
    {
        style_tree const& tree = *path->tree;
        std::map<string,style_tree>::const_iterator i =
            tree.substyles.find(substyle_name);
        if (i != tree.substyles.end())
            return &i->second;
        path = path->rest;
    }
    return 0;
}

style_tree const* find_substyle(style_search_path const* path,
    string const& substyle_name, widget_state state)
{
    style_tree const* substyle;
    substyle = find_substyle(path, substyle_name + widget_state_string(state));
    if (substyle)
        return substyle;
    if (state & WIDGET_FOCUSED)
    {
        widget_state substate(state.code & ~WIDGET_FOCUSED_CODE);
        substyle = find_substyle(path,
            substyle_name + widget_state_string(substate));
        if (substyle)
            return substyle;
    }
    if ((state & WIDGET_PRIMARY_STATE_MASK) != WIDGET_NORMAL)
    {
        widget_state substate(
            state.code & ~WIDGET_PRIMARY_STATE_MASK_CODE);
        substyle = find_substyle(path,
            substyle_name + widget_state_string(substate));
        if (substyle)
            return substyle;
    }
    if ((state & WIDGET_FOCUSED) &&
        (state & WIDGET_PRIMARY_STATE_MASK) != WIDGET_NORMAL)
    {
        substyle = find_substyle(path, substyle_name);
        if (substyle)
            return substyle;
    }
    return 0;
}

struct substyle_data
{
    owned_id key;
    style_search_path path;
    primary_style_properties properties;
    style_state state;
    layout_style_info style_info;
    value_id_by_reference<local_id> id;
    local_identity identity;
};

void scoped_substyle::begin(ui_context& ctx,
    getter<string> const& substyle_name, widget_state state)
{
    substyle_data* data;
    if (get_cached_data(ctx, &data) ||
        !data->key.matches(combine_ids(ref(*ctx.style.id),
            combine_ids(ref(substyle_name.id()), make_id(state)))))
    {
        inc_version(data->identity);

        style_tree const* substyle =
            find_substyle(ctx.style.path, get(substyle_name), state);
        if (substyle)
        {
            data->path.tree = substyle;
            data->path.rest = ctx.style.path;
        }
        else
            data->path = *ctx.style.path;

        data->state.path = &data->path;

        read_primary_style_properties(
            *ctx.system, &data->properties, data->state.path);
        data->state.properties = &data->properties;

        data->state.theme = ctx.style.theme;

        data->state.id = &data->id;

        read_layout_style_info(&data->style_info, data->properties.font,
            data->state.path);

        data->key.store(combine_ids(ref(*ctx.style.id),
            combine_ids(ref(substyle_name.id()), make_id(state))));

        data->id = get_id(data->identity);
    }

    old_state_ = ctx.style;
    ctx.style = data->state;

    old_style_info_ = ctx.layout.style_info;
    ctx.layout.style_info = &data->style_info;

    ctx_ = &ctx;
}
void scoped_substyle::end()
{
    if (ctx_)
    {
        ctx_->style = old_state_;
        ctx_->layout.style_info = old_style_info_;
        ctx_ = 0;
    }
}

void set_style(style_tree& tree, string const& subpath,
    property_map const& properties)
{
    std::size_t first_slash = subpath.find('/');
    if (first_slash == string::npos)
    {
        if (subpath.empty())
            tree.properties = properties;
        else
            tree.substyles[subpath].properties = properties;
        return;
    }

    string child_name = subpath.substr(0, first_slash),
        rest_of_path = subpath.substr(first_slash + 1);

    // This is a little too permissive, since it would accept paths like
    // 'a///b', but there's nothing really wrong with that.
    if (child_name.empty())
    {
        set_style(tree, rest_of_path, properties);
        return;
    }

    set_style(tree.substyles[child_name], rest_of_path, properties);
}

widget_state get_widget_state(
    ui_context& ctx, widget_id id, bool enabled, bool pressed, bool selected)
{
    widget_state state;
    if (enabled)
    {
        if (selected)
            state = WIDGET_SELECTED;
        else if (detect_click_in_progress(ctx, id, LEFT_BUTTON) || pressed)
            state = WIDGET_DEPRESSED;
        else if (detect_potential_click(ctx, id))
            state = WIDGET_HOT;
        else
            state = WIDGET_NORMAL;
        if (id_has_focus(ctx, id) && ctx.system->input.window_has_focus &&
            ctx.system->input.keyboard_interaction)
        {
            state = state | WIDGET_FOCUSED;
        }
    }
    else
        state = WIDGET_DISABLED;
    return state;
}

widget_state get_button_state(ui_context& ctx, widget_id id,
    button_input_state const& state)
{
    return get_widget_state(ctx, id, true, is_pressed(state.key));
}

bool do_button_input(ui_context& ctx, widget_id id, button_input_state& state)
{
    add_to_focus_order(ctx, id);
    return detect_click(ctx, id, LEFT_BUTTON) ||
        detect_keyboard_click(ctx, state.key, id, KEY_SPACE);
}

// TIMERS

void start_timer(ui_context& ctx, widget_id id, unsigned duration)
{
    input_event* ie = dynamic_cast<input_event*>(ctx.event);
    ui_time_type now = ie ? ie->time : ctx.system->millisecond_tick_count;
    ctx.surface->request_timer_event(make_routable_widget_id(ctx, id),
        now + duration);
}

bool is_timer_done(ui_context& ctx, widget_id id)
{
    return ctx.event->type == TIMER_EVENT &&
        get_event<timer_event>(ctx).id == id;
}

void restart_timer(ui_context& ctx, widget_id id, unsigned duration)
{
    timer_event& e = get_event<timer_event>(ctx);
    ctx.surface->request_timer_event(make_routable_widget_id(ctx, id),
        e.trigger_time + duration);
}

// UI CACHING

struct ui_caching_node
{
    ui_caching_node* parent;
    counter_type last_content_change;

    // cached layout info
    counter_type last_layout_update;
    owned_id layout_id;
    layout_node* layout_subtree_head;
    layout_node** layout_subtree_tail;

    // cached rendering info
    counter_type last_render_update;
    owned_id render_id;
    unsigned render_pass_count;
    cached_rendering_content_ptr cached_content;
};

void record_content_change(ui_context& ctx)
{
    ui_caching_node* cacher = ctx.active_cacher;
    while (cacher && cacher->last_content_change != ctx.layout.refresh_counter)
    {
        cacher->last_content_change = ctx.layout.refresh_counter;
        cacher = cacher->parent;
    }
}

void cached_ui_block::begin(ui_context& ctx, id_interface const& id)
{
    ctx_ = &ctx;

    routing_.begin(ctx.routing);

    ui_caching_node* cacher;
    get_cached_data(ctx, &cacher);

    cacher->parent = ctx.active_cacher;
    ctx.active_cacher = cacher;

    switch (ctx.event->type)
    {
     case REFRESH_EVENT:
        is_relevant_ =
            cacher->last_layout_update != cacher->last_content_change ||
            !cacher->layout_id.matches(
                combine_ids(ref(*ctx.style.id), ref(id)));
        // If we're going to actually update the layout, record the current
        // value of the layout context's next_ptr, so we'll know where to look
        // for the address of the first node.
        if (is_relevant_)
        {
            layout_next_ptr_ = ctx.layout.next_ptr;
            // Store the ID here because it's only available within this
            // function.
            cacher->layout_id.store(combine_ids(ref(*ctx.style.id), ref(id)));
            // Ensure that the layout content is considered invalid until it's
            // updated.
            cacher->last_layout_update = 0;
        }
        // Otherwise, just splice in the cached subtree.
        else
        {
            *ctx.layout.next_ptr = cacher->layout_subtree_head;
            ctx.layout.next_ptr = cacher->layout_subtree_tail;
        }
        break;

     case RENDER_EVENT:
        if (cacher->last_render_update != cacher->last_content_change ||
            !cacher->render_id.matches(
                combine_ids(ref(*ctx.style.id), ref(id))) ||
            !is_valid(cacher->cached_content))
        {
            cacher->render_pass_count = 0;
            cacher->render_id.store(combine_ids(ref(*ctx.style.id), ref(id)));
            cacher->last_render_update = cacher->last_content_change;
        }
        switch (cacher->render_pass_count)
        {
         case 0:
            is_relevant_ = true;
            break;
         case 1:
            is_relevant_ = true;
            if (!is_valid(cacher->cached_content))
                ctx.surface->cache_content(cacher->cached_content);
            cacher->cached_content->start_recording();
            break;
         default:
            cacher->cached_content->playback();
            is_relevant_ = false;
            break;
        }
        break;

     default:
        is_relevant_ = routing_.is_relevant();
        break;
    }
}
void cached_ui_block::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;
        ui_caching_node* cacher = ctx.active_cacher;

        switch (ctx.event->type)
        {
         case REFRESH_EVENT:
            // If the layout was just updated, record the head and tail of the
            // layout subtree so we can splice it into the parent tree on
            // passes where we skip layout.
            if (is_relevant_)
            {
                cacher->layout_subtree_head = *layout_next_ptr_;
                cacher->layout_subtree_tail = ctx.layout.next_ptr;
                cacher->last_layout_update = cacher->last_content_change;
            }
            break;

         case RENDER_EVENT:
            switch (cacher->render_pass_count)
            {
             case 1:
                cacher->cached_content->stop_recording();
             case 0:
                ++cacher->render_pass_count;
            }

         default:
            is_relevant_ = routing_.is_relevant();
            break;
        }

        routing_.end();

        ctx.active_cacher = cacher->parent;
        ctx_ = 0;
    }
}

struct fps_data
{
    fps_data() : frame_count(0) {}
    int frame_count;
    optional<unsigned> last_update;
    optional<int> fps;
};

bool compute_fps(ui_context& ctx, int* fps)
{
    widget_id id = get_widget_id(ctx);
    fps_data* data;
    get_data(ctx, &data);

    if (ctx.event->type == REFRESH_EVENT)
        ++data->frame_count;

    if (is_timer_done(ctx, id))
    {
        if (!data->last_update)
        {
            data->last_update = get_event<timer_event>(ctx).time;
        }
        else if (get_event<timer_event>(ctx).time >=
            get(data->last_update) + 1000)
        {
            get(data->last_update) += 1000;
            data->fps = data->frame_count;
            data->frame_count = 0;
        }
        end_pass(ctx);
    }

    start_timer(ctx, id, 0);

    if (data->fps)
    {
        *fps = get(data->fps);
        return true;
    }
    else
        return false;
}

}
