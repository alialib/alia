#include <alia/ui/system.hpp>
#include <alia/layout/system.hpp>
#include <alia/ui/utilities.hpp>

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

struct end_pass_exception {};

struct context_invoker
{
    ui_system* system;
    ui_context* ctx;
    void operator()()
    {
        try
        {
            system->controller->do_ui(*ctx);
        }
        catch (end_pass_exception&)
        {
        }
    }
};

static void issue_event(
    ui_system& system, ui_event& event,
    bool targeted, routing_region_ptr const& target = routing_region_ptr())
{
    ui_context ctx;
    ctx.pass_aborted = false;
    ctx.system = &system;
    scoped_data_traversal data(system.data, ctx.data);
    bool is_refresh = event.type == REFRESH_EVENT;
    // Only use refresh events to decide when data is no longer needed.
    ctx.data.gc_enabled = ctx.data.cache_clearing_enabled = is_refresh;
    vector<2,double> size =
        is_refresh ? make_vector<double>(0, 0) :
            vector<2,double>(system.surface->size());
    initialize(ctx.geometry, box<2,double>(make_vector<double>(0, 0), size));
    ctx.surface = system.surface.get();
    if (event.type == RENDER_EVENT || event.type == OVERLAY_RENDER_EVENT)
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
    setup_initial_styling(ctx);
    ctx.active_cacher = 0;
    context_invoker fn;
    fn.system = &system;
    fn.ctx = &ctx;
    invoke_routed_traversal(fn, ctx.routing, ctx.data, targeted, target);
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

void render_ui(ui_system& system)
{
    {
        render_event e;
        issue_event(system, e);
    }
    if (is_valid(system.overlay_id))
    {
        render_event e;
        e.category = OVERLAY_CATEGORY;
        e.type = OVERLAY_RENDER_EVENT;
        issue_targeted_event(system, e, system.overlay_id);
    }
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

static routable_widget_id get_mouse_target(ui_system& ui)
{
    return is_valid(ui.input.active_id) ? ui.input.active_id : ui.input.hot_id;
}

void refresh_ui(ui_system& ui)
{
    refresh_event e;
    issue_event(ui, e, false);

    resolve_layout(ui.layout, layout_vector(ui.surface_size));
}

void update_ui(ui_system& ui, vector<2,unsigned> const& size,
    ui_time_type millisecond_tick_count, mouse_cursor* current_cursor)
{
    ui.millisecond_tick_count = millisecond_tick_count;

    // If the surface changes size, that could invalidate popup positioning,
    // so close any active popups.
    if (ui.surface_size != size)
    {
        ui.overlay_id = null_widget_id;
        ui.surface_size = size;
    }

    refresh_ui(ui);

    // Once layout has been resolved, we can honor requests to make a
    // particular widget visible.
    if (is_valid(ui.widget_to_make_visible))
    {
        make_widget_visible_event e(ui.widget_to_make_visible.id);
        if (is_valid(ui.overlay_id))
        {
            e.category = OVERLAY_CATEGORY;
            e.type = OVERLAY_MAKE_WIDGET_VISIBLE_EVENT;
        }
        issue_targeted_event(ui, e, ui.widget_to_make_visible);
        ui.widget_to_make_visible = null_widget_id;
    }

    routable_widget_id previous_mouse_target = get_mouse_target(ui);

    mouse_cursor cursor = DEFAULT_CURSOR;

    if (ui.input.mouse_inside_window)
    {
        bool overlay_hot = false;
        if (is_valid(ui.overlay_id))
        {
            mouse_hit_test_event hit_test;
            hit_test.category = OVERLAY_CATEGORY;
            hit_test.type = OVERLAY_MOUSE_HIT_TEST_EVENT;
            issue_targeted_event(ui, hit_test, ui.overlay_id);
            if (is_valid(hit_test.id))
            {
                ui.input.hot_id = hit_test.id;
                cursor = hit_test.cursor;
                overlay_hot = true;
            }
        }
        if (!overlay_hot)
        {
            mouse_hit_test_event hit_test;
            issue_event(ui, hit_test);
            ui.input.hot_id = hit_test.id;
            cursor = hit_test.cursor;
        }
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

        // This may have caused state changes, so we need to refresh again.
        refresh_ui(ui);
    }

    if (current_cursor)
        *current_cursor = cursor;
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
    // First determine who should receive the event.
    routable_widget_id target;
    if (is_valid(ui.overlay_id))
    {
        wheel_hit_test_event hit_test;
        hit_test.category = OVERLAY_CATEGORY;
        hit_test.type = OVERLAY_WHEEL_HIT_TEST_EVENT;
        issue_targeted_event(ui, hit_test, ui.overlay_id);
        if (is_valid(hit_test.id))
            target = hit_test.id;
    }
    if (!is_valid(target))
    {
        wheel_hit_test_event hit_test;
        issue_event(ui, hit_test);
        target = hit_test.id;
    }
    // Now dispatch it.
    if (is_valid(target))
    {
        mouse_wheel_event event(time, target.id, movement);
        issue_targeted_event(ui, event, target);
    }
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

void advance_focus(ui_system& ui)
{
    set_focus(ui, get_focus_successor(ui, ui.input.focused_id.id));
}

void regress_focus(ui_system& ui)
{
    set_focus(ui, get_focus_predecessor(ui, ui.input.focused_id.id));
}

}
