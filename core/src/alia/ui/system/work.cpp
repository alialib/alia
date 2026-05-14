#include <alia/abi/ui/input/constants.h>
#include <alia/abi/ui/layout/system.h>
#include <alia/abi/ui/system/work.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/flow/dispatch.h>
#include <alia/ui/system/internal_api.h>
#include <alia/ui/system/work_internal.h>

#include <cstdint>

using namespace alia;
using namespace alia::operators;

namespace alia {

namespace {

alia_element_id
get_mouse_target(alia_ui_system* ui)
{
    return alia_element_id_is_valid(ui->input.element_with_capture)
             ? ui->input.element_with_capture
             : ui->input.hot_element;
}

void
apply_pointer_state_from_event(alia_ui_system& ui, alia_event const& ev)
{
    switch (ev.type)
    {
        case ALIA_EVENT_MOUSE_MOTION: {
            auto const& m = as_mouse_motion_event(ev);
            ui.input.mouse_position = {m.x, m.y};
            ui.input.mouse_inside_window = true;
            break;
        }
        case ALIA_EVENT_MOUSE_PRESS: {
            auto const& b = as_mouse_press_event(ev);
            ui.input.mouse_position = {b.x, b.y};
            ui.input.mouse_inside_window = true;
            break;
        }
        case ALIA_EVENT_MOUSE_RELEASE: {
            auto const& b = as_mouse_release_event(ev);
            ui.input.mouse_position = {b.x, b.y};
            ui.input.mouse_inside_window = true;
            break;
        }
        case ALIA_EVENT_DOUBLE_CLICK: {
            auto const& b = as_double_click_event(ev);
            ui.input.mouse_position = {b.x, b.y};
            ui.input.mouse_inside_window = true;
            break;
        }
        default:
            break;
    }
}

} // namespace

bool
timer_is_due(ui_system const& ui)
{
    if (ui.timer_requests.empty())
        return false;
    auto const& top = ui.timer_requests.top();
    return static_cast<int64_t>(ui.tick_count - top.fire_time) >= 0;
}

bool
evaluate_refresh_hook_policy(ui_system& ui, alia_ui_refresh_hook_policy mode)
{
    switch (mode)
    {
        case ALIA_UI_REFRESH_NEVER:
            return false;
        case ALIA_UI_REFRESH_IF_DIRTY:
            return ui.ui_dirty;
        case ALIA_UI_REFRESH_ALWAYS:
        default:
            return true;
    }
}

void
apply_refresh_hook_policy(ui_system& ui, alia_ui_refresh_hook_policy mode)
{
    if (evaluate_refresh_hook_policy(ui, mode))
        refresh_system(ui);
}

void
run_layout_resolve(ui_system& ui)
{
    alia_layout_system_resolve(
        &ui.layout, alia_vec2i_to_vec2f(ui.surface_size));
}

void
update_hot_from_pointer(ui_system& ui)
{
    if (ui.input.mouse_inside_window)
    {
        alia_event event = alia_make_mouse_hit_test_event(
            {.x = ui.input.mouse_position.x,
             .y = ui.input.mouse_position.y,
             .result
             = {.id = alia_element_id{}, .cursor = ALIA_CURSOR_DEFAULT}});
        dispatch_event(ui, event);
        if (alia_element_id_is_valid(as_mouse_hit_test_event(event).result.id))
        {
            set_hot_element(ui, as_mouse_hit_test_event(event).result.id);
        }
        else
        {
            set_hot_element(ui, alia_element_id{});
        }
    }
    else
    {
        set_hot_element(ui, alia_element_id{});
    }
}

void
deliver_queued_event(ui_system& ui, alia_event& ev)
{
    switch (ev.type)
    {
        case ALIA_EVENT_MOUSE_MOTION:
            dispatch_targeted_event(ui, ev, get_mouse_target(&ui));
            if (ui.input.mouse_button_state != 0)
                ui.input.dragging = true;
            break;

        case ALIA_EVENT_MOUSE_PRESS: {
            auto target = get_mouse_target(&ui);
            if (alia_element_id_is_valid(target))
                dispatch_targeted_event(ui, ev, target);
            auto const& b = as_mouse_press_event(ev);
            ui.input.last_mouse_press_time[unsigned(b.button)] = ui.tick_count;
            ui.input.mouse_button_state |= 1 << unsigned(b.button);
            ui.input.keyboard_interaction = false;
            break;
        }

        case ALIA_EVENT_MOUSE_RELEASE: {
            auto target = get_mouse_target(&ui);
            if (alia_element_id_is_valid(target))
                dispatch_targeted_event(ui, ev, target);
            auto const& b = as_mouse_release_event(ev);
            ui.input.mouse_button_state &= ~(1 << unsigned(b.button));
            if (ui.input.mouse_button_state == 0)
            {
                set_element_with_capture(ui, alia_element_id{});
                ui.input.dragging = false;
            }
            break;
        }

        case ALIA_EVENT_DOUBLE_CLICK: {
            auto target = get_mouse_target(&ui);
            if (alia_element_id_is_valid(target))
                dispatch_targeted_event(ui, ev, target);
            auto const& b = as_double_click_event(ev);
            ui.input.mouse_button_state |= 1 << unsigned(b.button);
            ui.input.keyboard_interaction = false;
            break;
        }

        case ALIA_EVENT_SCROLL_INPUT: {
            alia_event hit_test_event = alia_make_scroll_input_hit_test_event(
                {.x = ui.input.mouse_position.x,
                 .y = ui.input.mouse_position.y,
                 .result = ALIA_ELEMENT_ID_NONE});
            dispatch_event(ui, hit_test_event);
            if (alia_element_id_is_valid(
                    as_scroll_input_hit_test_event(hit_test_event).result))
            {
                dispatch_targeted_event(
                    ui,
                    ev,
                    as_scroll_input_hit_test_event(hit_test_event).result);
            }
            break;
        }

        case ALIA_EVENT_KEY_PRESS:
            dispatch_targeted_event(ui, ev, ui.input.element_with_focus);
            if (!as_key_press_event(ev).acknowledged)
            {
                alia_key_input const& k = as_key_press_event(ev);
                alia_event global = alia_make_global_key_press_event(
                    {.key = k.key, .acknowledged = false});
                dispatch_event(ui, global);
            }
            break;

        case ALIA_EVENT_GLOBAL_KEY_PRESS:
            dispatch_event(ui, ev);
            break;

        case ALIA_EVENT_KEY_RELEASE:
            dispatch_targeted_event(ui, ev, ui.input.element_with_focus);
            if (!as_key_release_event(ev).acknowledged)
            {
                alia_key_input const& k = as_key_release_event(ev);
                alia_event global = alia_make_global_key_release_event(
                    {.key = k.key, .acknowledged = false});
                dispatch_event(ui, global);
            }
            break;

        case ALIA_EVENT_GLOBAL_KEY_RELEASE:
            dispatch_event(ui, ev);
            break;

        default:
            break;
    }
}

bool
drain_one_queued_event(ui_system& ui)
{
    if (ui.event_queue.empty())
        return false;

    alia_event ev = ui.event_queue.front();
    ui.event_queue.pop_front();

    apply_refresh_hook_policy(ui, ui.refresh_policy.before_input);
    run_layout_resolve(ui);

    apply_pointer_state_from_event(ui, ev);
    update_hot_from_pointer(ui);
    deliver_queued_event(ui, ev);

    ui.ui_dirty = true;
    return true;
}

void
drain_event_queue(ui_system& ui)
{
    while (drain_one_queued_event(ui))
        ;
}

void
finalize_update(ui_system& ui)
{
    run_layout_resolve(ui);
    update_hot_from_pointer(ui);
    if (evaluate_refresh_hook_policy(ui, ui.refresh_policy.before_draw))
    {
        refresh_system(ui);
        run_layout_resolve(ui);
        update_hot_from_pointer(ui);
    }
}

} // namespace alia

extern "C" {

void
alia_ui_system_poll_clock(alia_ui_system* ui)
{
    if (!ui)
        return;
    ui->tick_count = alia::steady_clock_now_ns();
}

void
alia_ui_system_begin_update(alia_ui_system* ui)
{
    if (!ui)
        return;

    alia_ui_system_poll_clock(ui);

    ++ui->timer_event_cycle;
    ui->update_timer_cycle = ui->timer_event_cycle;

    if (ui->event_queue.empty())
        refresh_system(*ui);
}

alia_ui_work_step_kind
alia_ui_work_step(alia_ui_system* ui)
{
    if (!ui)
        return ALIA_UI_WORK_STEP_IDLE;

    if (alia::drain_one_queued_event(*ui))
        return ALIA_UI_WORK_STEP_INPUT;

    if (alia::timer_is_due(*ui))
    {
        alia::process_due_timers(*ui, ui->tick_count, ui->update_timer_cycle);
        return ALIA_UI_WORK_STEP_TIMER;
    }

    return ALIA_UI_WORK_STEP_IDLE;
}

void
alia_ui_system_end_update(alia_ui_system* ui)
{
    if (!ui)
        return;
    alia::finalize_update(*ui);
}

bool
alia_ui_needs_tick(alia_ui_system* ui)
{
    if (!ui)
        return false;
    if (!ui->event_queue.empty())
        return true;
    if (alia::timer_is_due(*ui))
        return true;
    if (ui->ui_dirty)
        return true;
    return false;
}

bool
alia_ui_next_wake_ns(alia_ui_system* ui, alia_nanosecond_count* out_wake_ns)
{
    if (!ui || !out_wake_ns)
        return false;

    if (!ui->event_queue.empty())
    {
        *out_wake_ns = ui->tick_count;
        return true;
    }

    if (!ui->timer_requests.empty())
    {
        *out_wake_ns = ui->timer_requests.top().fire_time;
        return true;
    }

    return false;
}

void
alia_ui_enqueue_event(alia_ui_system* ui, alia_event const* event)
{
    if (!ui || !event)
        return;
    ui->event_queue.push_back(*event);
}

void
alia_ui_mark_dirty(alia_ui_system* ui)
{
    if (!ui)
        return;
    ui->ui_dirty = true;
}

void
alia_ui_set_refresh_policy(
    alia_ui_system* ui, alia_ui_refresh_policy const* policy)
{
    if (!ui || !policy)
        return;
    ui->refresh_policy = *policy;
}

} // extern "C"
