#include <alia/ui/system/api.hpp>

#include <alia/core/flow/events.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/system/object.hpp>
#include <alia/ui/utilities/hit_testing.hpp>
#include <alia/ui/utilities/keyboard.hpp>

namespace alia {

void
initialize(
    ui_system& ui,
    std::function<void(ui_context)> controller,
    std::shared_ptr<os_interface> os,
    std::shared_ptr<window_interface> window)
{
    initialize_core_system<vanilla_ui_context>(ui, nullptr);
    ui.controller = std::move(controller);
    ui.os = std::move(os);
    ui.window = std::move(window);
}

// void initialize_ui(
//     ui_system& ui,
//     alia__shared_ptr<ui_controller> const& controller,
//     alia__shared_ptr<alia::surface> const& surface,
//     vector<2,float> const& ppi,
//     alia__shared_ptr<os_interface> const& os,
//     alia__shared_ptr<style_tree> const& style)
// {
//     ui.controller = controller;
//     ui.surface = surface;
//     ui.surface_size = make_vector<unsigned>(0, 0);
//     ui.ppi = ppi;
//     ui.os = os;
//     ui.millisecond_tick_count = 0;
//     ui.timer_event_counter = 0;
//     ui.style.styles = style;
//     ui.menu_bar.parent = 0;
//     ui.menu_bar.children = 0;
//     ui.menu_bar.last_change = 0;
//     ui.last_refresh_duration = 0;
// }

void
update(ui_system& ui)
{
    // refresh_ui(ui);

    // Once layout has been resolved, we can honor requests to make a
    // particular widget visible.
    // if (!ui.pending_visibility_requests.empty())
    // {
    //     for (std::vector<widget_visibility_request>::const_iterator i
    //          = ui.pending_visibility_requests.begin();
    //          i != ui.pending_visibility_requests.end();
    //          ++i)
    //     {
    //         make_widget_visible_event e(*i);
    //         if (is_valid(ui.overlay_id))
    //         {
    //             e.category = OVERLAY_CATEGORY;
    //             e.type = OVERLAY_MAKE_WIDGET_VISIBLE_EVENT;
    //         }
    //         issue_targeted_event(ui, e, i->widget);
    //     }
    //     ui.pending_visibility_requests.clear();
    //     // The movement may have caused changes that require a refresh, so
    //     // issue another one.
    //     refresh_ui(ui);
    // }

    // routable_widget_id previous_mouse_target = get_mouse_target(ui);

    mouse_cursor resolved_cursor = mouse_cursor::DEFAULT;

    // Determine which widget is under the mouse cursor.
    if (ui.input.mouse_inside_window)
    {
        mouse_hit_test_event hit_test(ui.input.mouse_position);
        dispatch_event(ui, hit_test);
        if (hit_test.result)
        {
            set_hot_element(ui, hit_test.result->element);
            resolved_cursor = hit_test.result->cursor;
            // record_tooltip(ui, hit_test);
        }
        else
        {
            set_hot_element(ui, external_element_id());
        }
    }
    else
    {
        set_hot_element(ui, external_element_id());
    }

    // The block above gives us the mouse cursor that's been requested by
    // the widget under the mouse. However, if there's a different widget
    // that's active, it should take priority, so we need to see what cursor
    // it wants.
    // if (ui.input.id_with_capture.id && ui.input.id_with_capture.id !=
    // ui.input.hot_id.id)
    // {
    //     mouse_cursor_query query(ui.input.id_with_capture.id);
    //     issue_targeted_event(ui, query, ui.input.id_with_capture);
    //     resolved_cursor = query.cursor;
    // }

    // Communicate the desired mouse cursor back to window.
    if (resolved_cursor != ui.input.current_mouse_cursor)
    {
        ui.window->set_mouse_cursor(resolved_cursor);
        ui.input.current_mouse_cursor = resolved_cursor;
    }

    // Update the state of the tooltip based on the passage of time.
    // update_tooltip(ui);

    // If there's been a change in which widget the mouse is interacting with,
    // issue notification events.
    // routable_widget_id current_mouse_target = get_mouse_target(ui);
    // if (current_mouse_target.id != previous_mouse_target.id)
    // {
    //     {
    //         mouse_notification_event e(
    //             MOUSE_LOSS_EVENT, previous_mouse_target.id);
    //         issue_targeted_event(ui, e, previous_mouse_target);
    //     }
    //     {
    //         mouse_notification_event e(
    //             MOUSE_GAIN_EVENT, current_mouse_target.id);
    //         issue_targeted_event(ui, e, current_mouse_target);
    //     }

    //     // This may have caused state changes, so we need to refresh again.
    //     refresh_ui(ui);
    // }
}

void
update_window_size(ui_system& ui, vector<2, unsigned> const& new_size)
{
    // // If the surface changes size, that could invalidate popup positioning,
    // // so close any active popups.
    // if (ui.surface_size != new_size)
    //     ui.overlay_id = null_widget_id;

    ui.surface_size = new_size;
}

#if 0

struct initial_styling_data
{
    owned_id id;
    primary_style_properties props;
    layout_style_info info;
    style_search_path path;
};

static void
setup_initial_styling(ui_context& ctx)
{
    initial_styling_data* data;
    get_data(ctx, &data);

    if (!data->id.matches(get_id(ctx.system->style.id)))
    {
        data->path.rest = 0;
        data->path.tree = &*ctx.system->style.styles;

        read_primary_style_properties(*ctx.system, &data->props, &data->path);

        data->id.store(get_id(ctx.system->style.id));

        read_layout_style_info(
            ctx, &data->info, data->props.font, &data->path);
    }
    get_layout_traversal(ctx).style_info = &data->info;

    ctx.style.path = &data->path;
    ctx.style.properties = &data->props;
    ctx.style.id = &data->id.get();
    ctx.style.theme = &ctx.system->style.theme;
}

#endif

static external_element_id
get_focus_successor(ui_system& /*sys*/, internal_element_id /*target*/)
{
    return external_element_id();
    // TODO
    // focus_successor_event event;
    // event.target = target;
    // Initializing `just_saw_target` to true handles the cases where no widget
    // has focus or where the focus is on the last widget in the focus
    // sequence. The correct successor here is the very first widget, so by
    // initializing `just_saw_target` to true, we'll pick up that first widget
    // as the default answer to the query. (And in cases where another widget
    // is the correct successor, it'll overwrite that answer.)
    // event.just_saw_target = true;

    // event_delivery_fixture<focus_successor_event> delivery_fixture(sys);
    // walk_widget_tree(sys.root_widget, [&](widget* widget) {
    //     delivery_fixture.deliver(widget, event);
    // });

    // return externalize(event.successor);
}

static external_element_id
get_focus_predecessor(ui_system& /*sys*/, internal_element_id /*target*/)
{
    return external_element_id();
    // focus_predecessor_event event;
    // event.target = target;
    // event.saw_target = false;

    // event_delivery_fixture<focus_predecessor_event> delivery_fixture(sys);
    // walk_widget_tree(sys.root_widget, [&](widget* widget) {
    //     delivery_fixture.deliver(widget, event);
    // });

    // return externalize(event.predecessor);
}

#if 0

struct tooltip_overlay_state
{
    string message;
    layout_box generating_region;
    float opacity;
};

bool static
operator==(tooltip_overlay_state const& a, tooltip_overlay_state const& b)
{
    return
        a.message == b.message &&
        a.generating_region == b.generating_region &&
        a.opacity == b.opacity;
}

bool static
operator!=(tooltip_overlay_state const& a, tooltip_overlay_state const& b)
{
    return !(a == b);
}

tooltip_overlay_state static
interpolate(tooltip_overlay_state const& a, tooltip_overlay_state const& b, double factor)
{
    tooltip_overlay_state interpolated;
    // Transition the message/region instantly unless there's no message to transition to.
    if (!b.message.empty())
    {
        interpolated.message = b.message;
        interpolated.generating_region = b.generating_region;
    }
    else
    {
        interpolated.message = a.message;
        interpolated.generating_region = a.generating_region;
    }
    // Transition the opacity smoothly.
    interpolated.opacity = interpolate(a.opacity, b.opacity, factor);
    return interpolated;
}

static void
do_tooltip_overlay(ui_context& ctx)
{
    auto& tooltip = ctx.system->tooltip;

    scoped_data_block data_block(ctx, tooltip.data);

    // If there's an active tooltip message, but the tooltip system isn't enabled yet, we
    // need to make sure the UI continues updating so that we can display the tooltip when
    // necessary.
    if (!tooltip.enabled && !tooltip.message.empty())
        request_animation_refresh(ctx);

    // This is all written somewhat crudely (without using accessors) because the
    // utilities that would allow it to be written more elegantly are currently part of
    // CRADLE, not alia.

    auto current_state =
        tooltip_overlay_state{
            tooltip.message,
            tooltip.generating_region,
            tooltip.enabled && !tooltip.message.empty() ? 1.f : 0.f };

    auto smoothed_state =
        smooth_raw_value(ctx, current_state, animated_transition(default_curve, 200));

    alia_if (!smoothed_state.message.empty() && smoothed_state.opacity > 0)
    {
        floating_layout layout(ctx);

        scoped_surface_opacity scoped_opacity(ctx, smoothed_state.opacity);

        scoped_transformation transformation;
        if (!is_refresh_pass(ctx))
        {
            vector<2,int> position;
            // Decide how to align each axis of the tooltip.
            auto surface_size = layout_vector(ctx.system->surface_size);
            // First get the region that generated the tooltip (plus a little padding).
            // We want to try to align the tooltip with this.
            auto generating_region =
                add_border(
                    smoothed_state.generating_region,
                    as_layout_size(ctx.system->style.magnification * 2.5));
            auto lower_region_edge = get_low_corner(generating_region);
            auto upper_region_edge = get_high_corner(generating_region);
            // For the horizontal alignment, we prefer to align the left edge of the
            // tooltip with the left edge of the generating region.
            if (lower_region_edge[0] + layout.size()[0] <= surface_size[0] ||
                surface_size[0] - lower_region_edge[0] > upper_region_edge[0])
            {
                position[0] = lower_region_edge[0];
            }
            else
            {
                position[0] = upper_region_edge[0] - layout.size()[0];
            }
            // For the vertical alignment, we prefer to align the bottom edge of the
            // tooltip with the top edge of the generating region.
            if (lower_region_edge[1] > layout.size()[1] ||
                surface_size[1] - upper_region_edge[1] < lower_region_edge[1])
            {
                position[1] = lower_region_edge[1] - layout.size()[1];
            }
            else
            {
                position[1] = upper_region_edge[1];
            }
            // Set up our transformation to move the tooltip to that position.
            transformation.begin(*get_layout_traversal(ctx).geometry);
            transformation.set(translation_matrix(vector<2,double>(position)));
        }

        // If the string is short enough, just display it without wrapping.
        alia_if (smoothed_state.message.length() < 64)
        {
            panel p(ctx, text("tooltip"));
            do_text(ctx, in_ptr(&smoothed_state.message));
        }
        // Otherwise, create a panel of a fixed width and let it flow within that.
        alia_else
        {
            panel p(ctx, text("tooltip"), width(30, EM));
            do_flow_text(ctx, in_ptr(&smoothed_state.message));
        }
        alia_end
    }
    alia_end
}

static bool
issue_event(
    ui_system& system, ui_event& event,
    bool targeted, routing_region_ptr const& target = routing_region_ptr())
{
    ui_context ctx;
    ctx.system = &system;

    geometry_context geometry;
    ctx.geometry = &geometry;
    initialize(geometry,
        box<2,double>(make_vector<double>(0, 0),
            vector<2,double>(system.surface_size)));

    ctx.surface = system.surface.get();
    if (event.type == RENDER_EVENT || event.type == OVERLAY_RENDER_EVENT)
        set_subscriber(*ctx.geometry, *ctx.surface);

    layout_traversal layout;
    ctx.layout = &layout;

    scoped_layout_refresh slr;
    scoped_layout_traversal slt;
    if (is_refresh)
        slr.begin(system.layout, layout, system.ppi);
    else
        slt.begin(system.layout, layout, geometry, system.ppi);

    ctx.event = &event;

    ctx.active_cacher = 0;

    setup_initial_styling(ctx);

    ctx.validation.detection = 0;
    ctx.validation.reporting = 0;

    ctx.menu.active_container = 0;
    ctx.menu.next_ptr = 0;

    context_invoker fn;
    fn.system = &system;
    fn.ctx = &ctx;
    invoke_routed_traversal(fn, ctx.routing, data, targeted, target);

    // Do the tooltip overlay (if any).
    // Note that we only need refresh and render events for the tooltip overlay, and we
    // actually want it to render after the OVERLAY_RENDER_EVENT (if there is one).
    switch (event.type)
    {
     case RENDER_EVENT:
        // If there's an active overlay, do the rendering after that.
        if (is_valid(system.overlay_id))
            break;
        // intentional fallthrough
     case OVERLAY_RENDER_EVENT:
        event.category = RENDER_CATEGORY;
        event.type = RENDER_EVENT;
        // intentional fallthrough
     case REFRESH_EVENT:
        do_tooltip_overlay(ctx);
    }

    return fn.aborted;
}

layout_vector
measure_initial_ui(
    alia__shared_ptr<ui_controller> const& controller,
    ui_style const& style,
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

void refresh_ui(ui_system& ui)
{
    // Capture the start time.
    // Only supporting Windows at the moment.
#ifdef WIN32
    // First get the frequency of the counter.
    // This doesn't change, so we only have to do it once.
    static LARGE_INTEGER frequency;
    static bool queried_frequency = false;
    if (!queried_frequency)
    {
        QueryPerformanceFrequency(&frequency);
        queried_frequency = true;
    }

    LARGE_INTEGER start_time;
    QueryPerformanceCounter(&start_time);
#endif

    refresh_event e;
    // Continue refreshing as long as the refresh event is being aborted.
    // This is a workaround for code that wants to handle events on refresh
    // passes.
    while (issue_event(ui, e, false))
        ;

    // Record the duration of the refresh.
#ifdef WIN32
    LARGE_INTEGER end_time;
    QueryPerformanceCounter(&end_time);
    ui.last_refresh_duration =
        int((end_time.QuadPart - start_time.QuadPart) * 1000000 /
            frequency.QuadPart);
#else
    ui.last_refresh_duration = 0;
#endif

    resolve_layout(ui.layout, layout_vector(ui.surface_size));
}

int get_last_refresh_duration(ui_system& ui)
{
    return ui.last_refresh_duration;
}

void static
record_tooltip(ui_system& ui, mouse_hit_test_event const& hit_test)
{
    ui.tooltip.message = hit_test.tooltip_message;
    ui.tooltip.generating_region = hit_test.hit_box;
}

void static
update_tooltip(ui_system& ui)
{
    bool tooltips_enabled = ui.tooltip.enabled;

    // If the UI has been hovering over a widget with a tooltip for a while, enable
    // the tooltip overlay.
    if (!is_valid(ui.input.id_with_capture) && !ui.tooltip.message.empty() &&
        ui.millisecond_tick_count - ui.input.hover_start_time > 500)
    {
        tooltips_enabled = true;
    }
    // If the UI has been hovering over nothing for a while, disable the tooltip
    // overlay.
    if (ui.tooltip.message.empty() &&
        ui.millisecond_tick_count - ui.input.hover_start_time > 200)
    {
        tooltips_enabled = false;
    }

    // If the tooltip state has changed, refresh the UI.
    if (tooltips_enabled != ui.tooltip.enabled)
    {
        ui.tooltip.enabled = tooltips_enabled;
        refresh_ui(ui);
    }
}

bool process_timer_requests(ui_system& ui, ui_time_type now)
{
    ++ui.timer_event_counter;
    bool update_required = false;
    while (1)
    {
        // Ideally, the list would be stored sorted, but it has to be
        // sorted relative to the current tick count (to handle wrapping),
        // and the list is generally not very long anyway.
        ui_timer_request_list::iterator next_event =
            ui.timer_requests.end();
        for (ui_timer_request_list::iterator
            i = ui.timer_requests.begin();
            i != ui.timer_requests.end(); ++i)
        {
            if (i->frame_issued != ui.timer_event_counter &&
                int(now - i->trigger_time) >= 0 &&
                (next_event == ui.timer_requests.end() ||
                int(next_event->trigger_time - i->trigger_time) >= 0))
            {
                next_event = i;
            }
        }
        if (next_event == ui.timer_requests.end())
            break;

        update_required = true;

        ui_timer_request request = *next_event;
        ui.timer_requests.erase(next_event);

        {
            timer_event e(request.id.id, request.trigger_time, now);
            issue_targeted_event(ui, e, request.id);
        }

        refresh_ui(ui);
    }
    if (ui.next_update && int(now - get(ui.next_update)) >= 0)
        update_required = true;
    return update_required;
}

bool has_timer_requests(ui_system& ui)
{
    return ui.next_update || !ui.timer_requests.empty();
}

optional<ui_time_type>
get_time_until_next_update(ui_system& ui, ui_time_type now)
{
    optional<ui_time_type> time_remaining;

    ui_timer_request_list::iterator next_event =
        ui.timer_requests.end();
    for (ui_timer_request_list::iterator
        i = ui.timer_requests.begin();
        i != ui.timer_requests.end(); ++i)
    {
        if (next_event == ui.timer_requests.end() ||
            int(next_event->trigger_time - i->trigger_time) >= 0)
        {
            next_event = i;
        }
    }
    if (next_event != ui.timer_requests.end())
        time_remaining = next_event->trigger_time - now;

    if (ui.next_update)
    {
        if (!time_remaining || get(ui.next_update) < get(time_remaining))
            time_remaining = get(ui.next_update);
    }

    return time_remaining;
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
bool process_focused_key_press(ui_system& ui, ui_time_type time,
    key_event_info const& info)
{
    ui.input.keyboard_interaction = true;
    key_event e(KEY_PRESS_EVENT, time, info);
    if (is_valid(ui.input.focused_id))
        issue_targeted_event(ui, e, ui.input.focused_id);
    return e.acknowledged;
}
bool process_background_key_press(ui_system& ui, ui_time_type time,
    key_event_info const& info)
{
    key_event e(BACKGROUND_KEY_PRESS_EVENT, time, info);
    issue_event(ui, e);
    if (!e.acknowledged && info.code == KEY_TAB)
    {
        if (info.mods == KMOD_SHIFT)
        {
            regress_focus(ui);
            e.acknowledged = true;
        }
        else if (info.mods == 0)
        {
            advance_focus(ui);
            e.acknowledged = true;
        }
    }
    return e.acknowledged;
}
bool process_key_press(ui_system& ui, ui_time_type time,
    key_event_info const& info)
{
    return process_focused_key_press(ui, time, info) ||
        process_background_key_press(ui, time, info);
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

#endif

void
advance_focus(ui_system& /*sys*/)
{
    // TODO
    // set_focus(
    //     sys,
    //     get_focus_successor(
    //         sys,
    //         internal_element_id{
    //             sys.input.element_with_focus.widget.raw_ptr(),
    //             sys.input.element_with_focus.id}));
}

void
regress_focus(ui_system& /*sys*/)
{
    // TODO
    // set_focus(
    //     sys,
    //     get_focus_predecessor(
    //         sys,
    //         internal_element_id{
    //             sys.input.element_with_focus.widget.raw_ptr(),
    //             sys.input.element_with_focus.id}));
}

void
clear_focus(ui_system& ui)
{
    ui.input.element_with_focus = external_element_id();
}

void
set_element_with_capture(ui_system& ui, external_element_id element)
{
    ui.input.element_with_capture = std::move(element);
}

void
set_hot_element(ui_system& ui, external_element_id element)
{
    // TODO:
    // If no element has capture and the mouse is moving to a different
    // element, this marks the start of a hover.
    // if (!ui.input.element_with_capture && ui.input.hot_element != element)
    // {
    //     // ui.input.hover_start_time = ui.tick_count;
    // }

    ui.input.hot_element = std::move(element);
}

#if 0

void set_system_style(ui_system& system,
    alia__shared_ptr<style_tree> const& style)
{
    system.style.styles = style;
    on_ui_style_change(system);
}

#endif
} // namespace alia
