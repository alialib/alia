#include <alia/abi/ui/input/constants.h>
#include <alia/abi/ui/layout/system.h>
#include <alia/abi/ui/system/work.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/flow/dispatch.h>
#include <alia/ui/system/internal_api.h>
#include <alia/ui/system/work_internal.h>

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
                alia_event bg = alia_make_background_key_press_event(
                    {.code = k.code, .mods = k.mods, .acknowledged = false});
                dispatch_event(ui, bg);
            }
            break;

        case ALIA_EVENT_BACKGROUND_KEY_PRESS:
            dispatch_event(ui, ev);
            break;

        case ALIA_EVENT_KEY_RELEASE:
            dispatch_targeted_event(ui, ev, ui.input.element_with_focus);
            if (!as_key_release_event(ev).acknowledged)
            {
                alia_key_input const& k = as_key_release_event(ev);
                alia_event bg = alia_make_background_key_release_event(
                    {.code = k.code, .mods = k.mods, .acknowledged = false});
                dispatch_event(ui, bg);
            }
            break;

        case ALIA_EVENT_BACKGROUND_KEY_RELEASE:
            dispatch_event(ui, ev);
            break;

        default:
            break;
    }
}

void
drain_event_queue(ui_system& ui)
{
    while (!ui.event_queue.empty())
    {
        alia_event ev = ui.event_queue.front();
        ui.event_queue.pop_front();

        apply_refresh_hook_policy(ui, ui.refresh_policy.before_input);
        run_layout_resolve(ui);

        apply_pointer_state_from_event(ui, ev);
        update_hot_from_pointer(ui);
        deliver_queued_event(ui, ev);

        // Conservative: dispatch or chained handlers may require another
        // refresh before draw.
        ui.ui_dirty = true;
    }
}

} // namespace alia

extern "C" {

void
alia_ui_enqueue_event(alia_ui_system* ui, alia_event const* event)
{
    ui->event_queue.push_back(*event);
}

void
alia_ui_mark_dirty(alia_ui_system* ui)
{
    ui->ui_dirty = true;
}

void
alia_ui_set_refresh_policy(
    alia_ui_system* ui, alia_ui_refresh_policy const* policy)
{
    ui->refresh_policy = *policy;
}

} // extern "C"
