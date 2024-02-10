#include "alia/core/signals/lambdas.hpp"
#include "alia/core/timing/ticks.hpp"
#include <alia/ui/scrolling.hpp>

#include <algorithm>

#include <alia/core/signals/core.hpp>
#include <alia/core/timing/smoothing.hpp>
#include <alia/ui/common.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events/delivery.hpp>
#include <alia/ui/events/input.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/containers/utilities.hpp>
#include <alia/ui/layout/logic/linear.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/styling.hpp>
#include <alia/ui/system/input_constants.hpp>
#include <alia/ui/utilities/hit_testing.hpp>
#include <alia/ui/utilities/keyboard.hpp>
#include <alia/ui/utilities/mouse.hpp>
#include <alia/ui/utilities/scrolling.hpp>
#include <alia/ui/utilities/skia.hpp>
#include <alia/ui/widget.hpp>

#include <include/core/SkColor.h>
#include <ratio>

namespace alia {

// TODO: Move

element_state
get_element_state(
    ui_system& sys,
    internal_element_ref element,
    element_state overrides = NO_FLAGS)
{
    element_state state;
    if (!(overrides & ELEMENT_DISABLED))
    {
        if (overrides & ELEMENT_SELECTED)
        {
            state = ELEMENT_SELECTED;
        }
        else if (
            is_click_in_progress(sys, element, mouse_button::LEFT)
            || (overrides & ELEMENT_DEPRESSED))
        {
            state = ELEMENT_DEPRESSED;
        }
        else if (is_click_possible(sys, element))
        {
            state = ELEMENT_HOT;
        }
        else
        {
            state = ELEMENT_NORMAL;
        }
        if (element_has_focus(sys, element) && sys.input.window_has_focus
            && sys.input.keyboard_interaction)
        {
            state |= ELEMENT_FOCUSED;
        }
    }
    else
    {
        state = ELEMENT_DISABLED;
    }
    return state;
}

struct scrollbar_renderer
{
    virtual scrollbar_metrics
    get_metrics(dataless_ui_context ctx) const
        = 0;

    virtual void
    draw_background(
        render_event& render,
        scrollbar_metrics const& metrics,
        layout_box const& rect,
        unsigned axis,
        unsigned which,
        element_state state) const
        = 0;

    virtual void
    draw_thumb(
        render_event& render,
        scrollbar_metrics const& metrics,
        layout_box const& rect,
        unsigned axis,
        element_state state) const
        = 0;

    virtual void
    draw_button(
        render_event& render,
        scrollbar_metrics const& metrics,
        layout_vector const& position,
        unsigned axis,
        unsigned which,
        element_state state) const
        = 0;
};

// A scrollbar junction is the little square where two scrollbars meet.
struct scrollbar_junction_renderer
{
    virtual void
    draw(render_event& render, layout_box const& position) const
        = 0;
};

struct default_scrollbar_renderer : scrollbar_renderer
{
    virtual scrollbar_metrics
    get_metrics(dataless_ui_context) const override
    {
        // style_path_storage storage;
        // style_search_path const* path
        //     = add_substyle_to_path(&storage, ctx.style.path, 0,
        //     "scrollbar");
        // scrollbar_metrics metrics;
        // metrics.width = as_layout_size(resolve_absolute_length(
        //     get_layout_traversal(ctx),
        //     0,
        //     get_property(
        //         path,
        //         "width",
        //         UNINHERITED_PROPERTY,
        //         absolute_length(0.8f, EM))));
        scrollbar_metrics metrics;
        metrics.width = 30;
        // as_layout_size(resolve_absolute_length(
        //     get_layout_traversal(ctx), 0, absolute_length(0.8f, EM)));
        metrics.button_length = 30;
        metrics.minimum_thumb_length = 30;
        // // The minimum thumb length must be larger than the width in order
        // for
        // // rendering to work properly.
        // if (metrics.minimum_thumb_length < metrics.width + 1)
        //     metrics.minimum_thumb_length = metrics.width + 1;
        return metrics;
    }

    // background
    void
    draw_background(
        render_event& render,
        scrollbar_metrics const&,
        layout_box const& rect,
        unsigned,
        unsigned,
        element_state) const override
    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(12, 0, 0, 0));

        render.canvas->drawRect(as_skrect(rect), paint);
    }

    // thumb
    void
    draw_thumb(
        render_event& render,
        scrollbar_metrics const&,
        layout_box const& rect,
        unsigned,
        element_state) const override
    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(60, 0, 0, 0));

        render.canvas->drawRect(as_skrect(rect), paint);
    }

    // button
    void
    draw_button(
        render_event& render,
        scrollbar_metrics const&,
        layout_vector const& p,
        unsigned,
        unsigned,
        element_state) const override
    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(40, 0, 0, 0));

        render.canvas->drawRect(
            as_skrect(layout_box(p, make_layout_vector(30, 30))), paint);
    }
};

struct default_scrollbar_junction_renderer : scrollbar_junction_renderer
{
    void
    draw(render_event&, layout_box const&) const override
    {
    }
};

layout_scalar
get_scrollbar_width(scrollbar_data const& data)
{
    return data.metrics->width;
}

layout_scalar
get_minimum_scrollbar_length(scrollbar_data const& data)
{
    return data.metrics->minimum_thumb_length
           + 2 * data.metrics->button_length;
}

scrollbar_metrics
get_metrics(scrollbar_parameters const& sb)
{
    return *sb.data->metrics;
}

// The following are utilities for calculating the layout of the various parts
// of the scrollbar.

layout_box
get_background_area(scrollbar_parameters const& sb)
{
    layout_box area = sb.area;
    area.corner[sb.axis] += get_metrics(sb).button_length;
    area.size[sb.axis] -= get_metrics(sb).button_length * 2;
    return area;
}

layout_box
get_thumb_area(scrollbar_parameters const& sb)
{
    layout_box bg_area = get_background_area(sb);
    layout_box area = bg_area;
    area.size[sb.axis] = (std::max)(
        get_metrics(sb).minimum_thumb_length,
        sb.window_size * bg_area.size[sb.axis] / sb.content_size);
    area.corner[sb.axis]
        = bg_area.corner[sb.axis] + sb.data->physical_position;
    return area;
}

// button0 is the top/left button (depending on orientation)
layout_box
get_button0_area(scrollbar_parameters const& sb)
{
    layout_box area = sb.area;
    area.size[sb.axis] = get_metrics(sb).button_length;
    return area;
}

// button1 is the bottom/right button (depending on orientation)
layout_box
get_button1_area(scrollbar_parameters const& sb)
{
    layout_box area = sb.area;
    layout_scalar length = get_metrics(sb).button_length;
    area.corner[sb.axis] += sb.area.size[sb.axis] - length;
    area.size[sb.axis] = length;
    return area;
}

// bg0 is the background to the top/left of the thumb
// (depending on orientation)
layout_box
get_bg0_area(scrollbar_parameters const& sb)
{
    layout_box area = get_background_area(sb);
    area.size[sb.axis] = sb.data->physical_position;
    return area;
}

// bg1 is the background to the bottom/right of the thumb
// (depending on orientation)
layout_box
get_bg1_area(scrollbar_parameters const& sb)
{
    layout_box bg_area = get_background_area(sb);
    layout_box area = bg_area;
    area.corner[sb.axis] = get_high_corner(get_thumb_area(sb))[sb.axis];
    area.size[sb.axis]
        = get_high_corner(bg_area)[sb.axis] - area.corner[sb.axis];
    return area;
}

// The following are utilities for getting the IDs of the various parts of
// the scrollbar.

internal_element_ref
get_thumb_ref(scrollbar_parameters const& sb)
{
    return internal_element_ref{sb.widget, sb.element_id_start + 0};
}

internal_element_ref
get_button0_ref(scrollbar_parameters const& sb)
{
    return internal_element_ref{sb.widget, sb.element_id_start + 1};
}

internal_element_ref
get_button1_ref(scrollbar_parameters const& sb)
{
    return internal_element_ref{sb.widget, sb.element_id_start + 2};
}

internal_element_ref
get_bg0_ref(scrollbar_parameters const& sb)
{
    return internal_element_ref{sb.widget, sb.element_id_start + 3};
}

internal_element_ref
get_bg1_ref(scrollbar_parameters const& sb)
{
    return internal_element_ref{sb.widget, sb.element_id_start + 4};
}

// The following are utilities for working with scrollbar positions.
// A scrollbar's physical position is its actual position within its track, in
// pixels.
// A scrollbar's logical position is in terms of the scrolling units.
// (These may also be pixels, but they're relative to the overall content size,
// not just the track.)

layout_scalar
get_max_physical_position(scrollbar_parameters const& sb)
{
    return get_background_area(sb).size[sb.axis]
           - get_thumb_area(sb).size[sb.axis];
}

layout_scalar
get_max_logical_position(scrollbar_parameters const& sb)
{
    return sb.content_size - sb.window_size;
}

layout_scalar
logical_position_to_physical(
    scrollbar_parameters const& sb, layout_scalar position)
{
    layout_scalar max_physical = get_max_physical_position(sb);
    layout_scalar max_logical = get_max_logical_position(sb);
    return std::clamp(
        position * max_physical / max_logical, layout_scalar(0), max_physical);
}

layout_scalar
physical_position_to_logical(
    scrollbar_parameters const& sb, layout_scalar position)
{
    layout_scalar max_physical = get_max_physical_position(sb);
    layout_scalar max_logical = get_max_logical_position(sb);
    return max_physical <= 0 ? 0
                             : std::clamp(
                                 position * max_logical / max_physical,
                                 layout_scalar(0),
                                 max_logical);
}

void
reset_smoothing(scrollbar_data& data)
{
    data.smoothed_scroll_position = data.scroll_position;
    reset_smoothing(data.smoother, data.scroll_position);
}

void
set_logical_position(scrollbar_parameters const& sb, layout_scalar position)
{
    layout_scalar clamped
        = std::clamp(position, layout_scalar(0), get_max_logical_position(sb));
    auto& data = *sb.data;
    data.scroll_position = clamped;
    data.scroll_position_changed = true;
    data.physical_position = logical_position_to_physical(sb, clamped);
}

void
offset_logical_position(scrollbar_parameters const& sb, layout_scalar offset)
{
    set_logical_position(sb, sb.data->scroll_position + offset);
}

void
set_logical_position_abruptly(
    scrollbar_parameters const& sb, layout_scalar position)
{
    set_logical_position(sb, position);
    reset_smoothing(*sb.data);
}

void
set_physical_position(scrollbar_parameters const& sb, layout_scalar position)
{
    layout_scalar clamped = std::clamp(
        position, layout_scalar(0), get_max_physical_position(sb));
    auto& data = *sb.data;
    data.physical_position = clamped;
    data.scroll_position = physical_position_to_logical(sb, clamped);
    data.scroll_position_changed = true;
}

void
set_physical_position_abruptly(
    scrollbar_parameters const& sb, layout_scalar position)
{
    set_physical_position(sb, position);
    reset_smoothing(*sb.data);
}

inline layout_scalar
get_page_increment(scrollbar_parameters const& sb)
{
    return sb.window_size;
}

inline layout_scalar
get_line_increment(scrollbar_parameters const&)
{
    // TODO: something less arbitrary
    return 120;
}

void
process_button_input(
    ui_event_context ctx,
    scrollbar_parameters const& sb,
    internal_element_ref element,
    layout_scalar increment)
{
    static int const delay_after_first_increment = 400;
    static int const delay_after_other_increment = 40;

    static millisecond_count expect_timer_at = 0;

    if (detect_mouse_press(ctx, element, mouse_button::LEFT))
    {
        offset_logical_position(sb, increment);
        auto& sys = get_system(ctx);
        auto element_ref = externalize(element);
        expect_timer_at = get_system(ctx).external->get_tick_count()
                          + delay_after_first_increment;
        auto trigger_time = expect_timer_at;
        sys.external->schedule_callback(
            [&sys, element, element_ref, trigger_time] {
                prototype_timer_event event{
                    {{}, input_event_type::TIMER}, element, trigger_time};
                deliver_input_event(sys, element_ref.widget, event);
            },
            trigger_time);
        return;
    }

    if (element_has_capture(ctx, element))
    {
        if (is_mouse_button_pressed(ctx, mouse_button::LEFT))
        {
            prototype_timer_event* event;
            if (detect_event(ctx, &event) && event->target == element
                && event->trigger_time == expect_timer_at)
            {
                if (is_element_hot(ctx, element))
                {
                    offset_logical_position(sb, increment);
                }
                auto& sys = get_system(ctx);
                auto element_ref = externalize(element);
                expect_timer_at = get_system(ctx).external->get_tick_count()
                                  + delay_after_other_increment;
                auto trigger_time = expect_timer_at;
                sys.external->schedule_callback(
                    [&sys, element, element_ref, trigger_time] {
                        prototype_timer_event event{
                            {{}, input_event_type::TIMER},
                            element,
                            trigger_time};
                        deliver_input_event(sys, element_ref.widget, event);
                    },
                    trigger_time);
            }
        }
        else
        {
            expect_timer_at = 0;
        }
    }
}

static default_scrollbar_renderer default_renderer;

bool
scrollbar_is_valid(scrollbar_parameters const& sb)
{
    return sb.content_size > 0 && sb.window_size > 0
           && sb.window_size < sb.content_size
           && get_max_physical_position(sb) >= 0;
}

layout_scalar
clamp_scroll_position(scrollbar_parameters const& sb, layout_scalar position)
{
    return sb.content_size > sb.window_size ? std::clamp(
               position, layout_scalar(0), sb.content_size - sb.window_size)
                                            : 0;
}

void
refresh_scrollbar(dataless_ui_context ctx, scrollbar_parameters const& sb)
{
    scrollbar_renderer const* renderer;
    // TODO: get_themed_renderer(ctx, data.rendering, &renderer,
    // &default_renderer);
    renderer = &default_renderer;

    auto& data = *sb.data;

    // TODO: Is this ever useful?
    // auto clamped_position = clamp_scroll_position(sb, data.scroll_position);
    // if (clamped_position != data.scroll_position)
    //     set_logical_position_abruptly(sb, clamped_position);

    // Get the smoothed version of the scroll position.
    for (unsigned i = 0; i != 2; ++i)
    {
        data.smoothed_scroll_position = smooth_raw(
            ctx,
            data.smoother,
            data.scroll_position,
            animated_transition{linear_curve, 120});
    }

    if (!scrollbar_is_valid(sb))
        return;

    data.metrics.refresh(/* TODO: *ctx.style.id */ unit_id, [&] {
        return renderer->get_metrics(ctx);
    });

    // If the thumb isn't being dragged, then the physical position should
    // stay in sync with the logical position.
    if (!is_drag_in_progress(ctx, get_thumb_ref(sb), mouse_button::LEFT))
    {
        data.physical_position
            = logical_position_to_physical(sb, data.scroll_position);
    }
}

void
render_scrollbar(render_event& event, scrollbar_parameters const& sb)
{
    scrollbar_data& data = *sb.data;
    auto* renderer = &default_renderer;

    if (get_max_physical_position(sb) < 0)
    {
        // In this case, the scrollbar is too small to function, so just
        // draw the background and return.
        if (sb.area.size[0] > 0 && sb.area.size[1] > 0)
        {
            renderer->draw_background(
                event, *data.metrics, sb.area, sb.axis, 0, ELEMENT_NORMAL);
        }
        return;
    }

    renderer->draw_background(
        event,
        *data.metrics,
        get_bg0_area(sb),
        sb.axis,
        0,
        get_element_state(*event.sys, get_bg0_ref(sb)));
    renderer->draw_background(
        event,
        *data.metrics,
        get_bg1_area(sb),
        sb.axis,
        1,
        get_element_state(*event.sys, get_bg1_ref(sb)));
    renderer->draw_thumb(
        event,
        *data.metrics,
        get_thumb_area(sb),
        sb.axis,
        get_element_state(*event.sys, get_thumb_ref(sb)));
    renderer->draw_button(
        event,
        *data.metrics,
        get_button0_area(sb).corner,
        sb.axis,
        0,
        get_element_state(*event.sys, get_button0_ref(sb)));
    renderer->draw_button(
        event,
        *data.metrics,
        get_button1_area(sb).corner,
        sb.axis,
        1,
        get_element_state(*event.sys, get_button1_ref(sb)));
}

void
hit_test_scrollbar(
    scrollbar_parameters const& sb,
    hit_test_base& test,
    vector<2, double> const& point)
{
    hit_test_box(test, point, get_bg0_ref(sb), get_bg0_area(sb));
    hit_test_box(test, point, get_bg1_ref(sb), get_bg1_area(sb));
    hit_test_box(test, point, get_thumb_ref(sb), get_thumb_area(sb));
    hit_test_box(test, point, get_button0_ref(sb), get_button0_area(sb));
    hit_test_box(test, point, get_button1_ref(sb), get_button1_area(sb));
}

void
process_scrollbar_input(ui_event_context ctx, scrollbar_parameters const& sb)
{
    scrollbar_data& data = *sb.data;

    process_button_input(ctx, sb, get_bg0_ref(sb), -get_page_increment(sb));
    process_button_input(ctx, sb, get_bg1_ref(sb), get_page_increment(sb));

    if (detect_mouse_press(ctx, get_thumb_ref(sb), mouse_button::LEFT))
    {
        data.drag_start_delta = data.physical_position
                                - get_integer_mouse_position(
                                    get_system(ctx), *sb.widget)[sb.axis];
    }
    if (detect_drag(ctx, get_thumb_ref(sb), mouse_button::LEFT))
    {
        int mouse
            = get_integer_mouse_position(get_system(ctx), *sb.widget)[sb.axis];
        set_physical_position_abruptly(sb, mouse + data.drag_start_delta);
    }

    process_button_input(
        ctx, sb, get_button0_ref(sb), -get_line_increment(sb));
    process_button_input(ctx, sb, get_button1_ref(sb), get_line_increment(sb));
}

// TODO
constexpr unsigned scrollable_bit_offset = 0;
constexpr unsigned reserved_axes_bit_offset = 2;
constexpr unsigned scrollbars_on_bit_offset = 2;

// persistent data required for a scrollable view
struct scrollable_view_data
{
    // set by caller and copied here
    unsigned scrollable_axes, reserved_axes;

    // determined at usage site and needed by layout
    layout_scalar minimum_window_size, line_size;

    // determined by layout and stored here for general usage
    unsigned scrollbars_on;
    layout_vector content_size, window_size;

    // persistent data for scrollbars
    scrollbar_data sb_data[2];

    // // rendering data for junction
    // themed_rendering_data junction_rendering;

    // // layout container
    // scrollable_layout_container container;
};

void
set_scrollbar_on(scrollable_view_data& data, unsigned axis, bool on)
{
    unsigned const masks[] = {0x10, 0x01};
    data.scrollbars_on
        = (data.scrollbars_on & masks[axis]) | ((on ? 1 : 0) << axis);
}

bool
is_scrollbar_on(scrollable_view_data const& data, unsigned axis)
{
    return (data.scrollbars_on & (1 << axis)) != 0;
}

struct scrollable_view : widget_container
{
    void
    refresh(dataless_ui_context ctx)
    {
        data.scrollable_axes = 2;
        for (unsigned axis = 0; axis != 2; ++axis)
        {
            data.sb_data[axis].metrics.refresh(
                /* TODO: *ctx.style.id */ unit_id,
                [&] { return default_renderer.get_metrics(ctx); });
            if (is_scrollbar_on(data, axis))
                refresh_scrollbar(ctx, sb(axis));
        }
        // if (data.scroll_position_changed)
        // {
        //     if (signal_ready_to_write(position))
        //     {
        //         write_signal(position, data.scroll_position[1]);
        //         data.scroll_position_changed = false;
        //     }
        // }
        // else
        // {
        //     if (signal_has_value(position))
        //         data.scroll_position[1] = read_signal(position);
        // }
        data.minimum_window_size = 120;
        data.line_size = 60;
    }

    // implementation of layout interface
    layout_requirements
    get_horizontal_requirements() override
    {
        return cache_horizontal_layout_requirements(
            cacher, last_content_change, [&] {
                if ((data.scrollable_axes & 1) != 0)
                {
                    // If the window is horizontally scrollable, then we only
                    // need enough space for scrolling to happen.
                    return calculated_layout_requirements{
                        data.minimum_window_size, 0, 0};
                }
                else
                {
                    // Otherwise, we need to calculate the requirements.
                    calculated_layout_requirements x
                        = logic->get_horizontal_requirements(children);
                    layout_scalar required_width = x.size;
                    if ((data.scrollable_axes & 2) != 0)
                        required_width += get_scrollbar_width(data.sb_data[1]);
                    return calculated_layout_requirements{
                        required_width, 0, 0};
                }
            });
    }

    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width) override
    {
        return cache_vertical_layout_requirements(
            cacher, last_content_change, assigned_width, [&] {
                if ((data.scrollable_axes & 2) != 0)
                {
                    // If the window is vertically scrollable, then we only
                    // need enough space for scrolling to happen.
                    return calculated_layout_requirements{
                        data.minimum_window_size, 0, 0};
                }
                else
                {
                    // Otherwise, we need to calculate the requirements.
                    layout_scalar resolved_width = resolve_assigned_width(
                        this->cacher.resolved_spec,
                        assigned_width,
                        this->get_horizontal_requirements());
                    calculated_layout_requirements x
                        = logic->get_horizontal_requirements(children);
                    layout_scalar actual_width
                        = (std::max)(resolved_width, x.size);
                    calculated_layout_requirements y
                        = logic->get_vertical_requirements(
                            children, actual_width);
                    layout_scalar required_height = y.size;
                    if ((data.scrollable_axes & 1) != 0
                        && x.size > resolved_width)
                    {
                        required_height
                            += get_scrollbar_width(data.sb_data[0]);
                    }
                    return calculated_layout_requirements{
                        required_height, 0, 0};
                }
            });
    }

    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override
    {
        update_relative_assignment(
            *this,
            cacher,
            last_content_change,
            assignment,
            [&](auto const& resolved_assignment) {
                layout_vector available_size = resolved_assignment.region.size;

                // Track whether or not we're going to include the scrollbar
                // width in the available width for content.
                // bool exclude_scrollbar_

                calculated_layout_requirements x
                    = logic->get_horizontal_requirements(children);
                if (available_size[0] < x.size)
                {
                    set_scrollbar_on(data, 0, true);
                    available_size[1] -= get_scrollbar_width(data.sb_data[0]);
                }
                else
                {
                    set_scrollbar_on(data, 0, false);
                }

                // Attempted hack to avoid calling get_vertical_requirements
                // twice for long scrolled content...
                calculated_layout_requirements y_without_scrollbar;
                layout_scalar available_width_with_scrollbar
                    = available_size[0] - get_scrollbar_width(data.sb_data[1]);
                calculated_layout_requirements y_with_scrollbar
                    = logic->get_vertical_requirements(
                        children,
                        (std::max)(available_width_with_scrollbar, x.size));

                if (available_size[1] * available_size[0]
                    < y_with_scrollbar.size * available_width_with_scrollbar)
                {
                    set_scrollbar_on(data, 1, true);
                    available_size[0] -= get_scrollbar_width(data.sb_data[1]);
                    // The introduction of the vertical scrollbar may have
                    // triggered the need for a horizontal scrollbar as
                    // well.
                    if (!is_scrollbar_on(data, 0)
                        && available_size[0] < x.size)
                    {
                        set_scrollbar_on(data, 0, true);
                        available_size[1]
                            -= get_scrollbar_width(data.sb_data[0]);
                    }
                }
                else
                {
                    y_without_scrollbar = logic->get_vertical_requirements(
                        children, (std::max)(available_size[0], x.size));
                    if (available_size[1] < y_without_scrollbar.size)
                    {
                        set_scrollbar_on(data, 1, true);
                        available_size[0]
                            -= get_scrollbar_width(data.sb_data[1]);
                        // The introduction of the vertical scrollbar may have
                        // triggered the need for a horizontal scrollbar as
                        // well.
                        if (!is_scrollbar_on(data, 0)
                            && available_size[0] < x.size)
                        {
                            set_scrollbar_on(data, 0, true);
                            available_size[1]
                                -= get_scrollbar_width(data.sb_data[0]);
                        }
                    }
                    else
                    {
                        set_scrollbar_on(data, 1, false);
                    }
                }

                if ((data.reserved_axes & 1) != 0 && !is_scrollbar_on(data, 0))
                    available_size[1] -= get_scrollbar_width(data.sb_data[0]);
                if ((data.reserved_axes & 2) != 0 && !is_scrollbar_on(data, 1))
                    available_size[0] -= get_scrollbar_width(data.sb_data[1]);

                layout_scalar content_width
                    = (std::max)(available_size[0], x.size);

                calculated_layout_requirements y
                    = available_size[0] == available_width_with_scrollbar
                          ? y_with_scrollbar
                          : y_without_scrollbar;

                layout_scalar content_height
                    = (std::max)(available_size[1], y.size);

                layout_vector content_size
                    = make_layout_vector(content_width, content_height);

                // Determine if either scrollbar is scrolled to the end of the
                // content and the content is growing. If that's the case, we
                // consider it pinned to the end of the content.
                bool pin_to_end[2] = {false, false};
                for (unsigned axis = 0; axis != 2; ++axis)
                {
                    layout_scalar sp
                        = data.sb_data[axis].smoothed_scroll_position;
                    if (sp != 0
                        && sp + data.window_size[axis]
                               >= data.content_size[axis]
                        && sp + available_size[axis] < content_size[axis])
                    {
                        pin_to_end[axis] = true;
                    }
                }

                // Update the persistent copies of the content and window
                // sizes.
                data.content_size = content_size;
                data.window_size = available_size;

                // If we determined that either of the scrollbars are pinned to
                // the end, keep them that way.
                for (unsigned axis = 0; axis != 2; ++axis)
                {
                    if (pin_to_end[axis])
                    {
                        set_logical_position_abruptly(
                            sb(axis),
                            content_size[axis] - available_size[axis]);
                    }
                }

                // If the scroll position needs to be clamped because of
                // changes in content size, then do it abruptly, not
                // smoothly.
                for (unsigned i = 0; i != 2; ++i)
                {
                    if (is_scrollbar_on(data, i))
                    {
                        auto clamped_position = clamp_scroll_position(
                            sb(i), data.sb_data[i].scroll_position);
                        if (clamped_position
                            != data.sb_data[i].scroll_position)
                        {
                            set_logical_position_abruptly(
                                sb(i), clamped_position);
                        }
                    }
                }

                logic->set_relative_assignment(
                    children, content_size, content_height - y.descent);
            });
    }

    void
    render(render_event& event) override
    {
        auto const& region = get_assignment(this->cacher).region;
        SkRect bounds;
        bounds.fLeft = SkScalar(region.corner[0] + event.current_offset[0]);
        bounds.fTop = SkScalar(region.corner[1] + event.current_offset[1]);
        bounds.fRight = bounds.fLeft + SkScalar(region.size[0]);
        bounds.fBottom = bounds.fTop + SkScalar(region.size[1]);
        if (!event.canvas->quickReject(bounds))
        {
            event.canvas->save();
            auto original_offset = event.current_offset;
            event.canvas->clipRect(bounds);
            event.canvas->translate(
                -data.sb_data[0].smoothed_scroll_position,
                -data.sb_data[1].smoothed_scroll_position);
            event.current_offset += region.corner;
            alia::render_children(event, *this);
            event.current_offset = original_offset;
            event.canvas->restore();
            // {
            //     SkPaint paint;
            //     paint.setColor(SK_ColorDKGRAY);
            //     auto thumb_area = get_thumb_area();
            //     SkRect box;
            //     box.fLeft
            //         = SkScalar(thumb_area.corner[0] +
            //         event.current_offset[0]);
            //     box.fTop
            //         = SkScalar(thumb_area.corner[1] +
            //         event.current_offset[1]);
            //     box.fRight = box.fLeft + SkScalar(thumb_area.size[0]);
            //     box.fBottom = box.fTop + SkScalar(thumb_area.size[1]);
            //     event.canvas->drawRect(box, paint);
            // }
            render_scrollbar(event, sb(1));
        }
    }

    void
    hit_test(
        hit_test_base& test, vector<2, double> const& point) const override
    {
        auto const& region = get_assignment(this->cacher).region;
        if (is_inside(region, vector<2, float>(point)))
        {
            hit_test_box(test, point, internal_element_ref{*this, 0}, region);

            if (test.type == alia::hit_test_type::WHEEL)
            {
                static_cast<alia::wheel_hit_test&>(test).result
                    = externalize(internal_element_ref{*this, 0});
            }

            auto local_point = point - vector<2, double>(region.corner);
            local_point[0] += data.sb_data[0].smoothed_scroll_position;
            local_point[1] += data.sb_data[1].smoothed_scroll_position;
            for (widget* node = this->widget_container::children; node;
                 node = node->next)
            {
                node->hit_test(test, local_point);
            }

            hit_test_scrollbar(sb(1), test, point);
        }
    }

    void
    process_input(ui_event_context ctx) override
    {
        auto delta = detect_scroll(ctx, internal_element_ref{*this, 0});
        if (delta)
        {
            set_logical_position_abruptly(
                sb(1),
                data.sb_data[1].scroll_position -= float((*delta)[1] * 120));
        }

        process_scrollbar_input(ctx, sb(1));

        focus_on_click(ctx, internal_element_ref{*this, 0});
        auto key = detect_key_press(ctx);
        if (key)
            handle_scrolling_key_press(*key);
    }

    matrix<3, 3, double>
    transformation() const override
    {
        // TODO
        return parent->transformation();
    }

    layout_box
    bounding_box() const override
    {
        return this->cacher.relative_assignment.region;
    }

    void
    reveal_region(region_reveal_request const& request) override
    {
        auto box = request.region;
        auto const inverse_transform = inverse(this->transformation());
        auto const transformed_box
            = transform_box(inverse_transform, alia::box<2, double>(box));
        auto const region_ul = transformed_box.corner;
        auto const region_lr = get_high_corner(transformed_box);
        auto const window_ul = make_vector<double>(
            data.sb_data[0].scroll_position, data.sb_data[1].scroll_position);
        auto const window_lr = window_ul + vector<2, double>(data.window_size);
        for (int i = 0; i != 2; ++i)
        {
            layout_scalar correction = 0;
            if (request.move_to_top)
            {
                correction
                    = round_to_layout_scalar(region_ul[i] - window_ul[i]);
            }
            else if (transformed_box.size[i] <= double(data.window_size[i]))
            {
                if (region_ul[i] < window_ul[i] && region_lr[i] < window_lr[i])
                {
                    correction
                        = -round_to_layout_scalar(window_ul[i] - region_ul[i]);
                }
                else if (
                    region_ul[i] > window_ul[i] && region_lr[i] > window_lr[i])
                {
                    correction = round_to_layout_scalar((std::min)(
                        region_ul[i] - window_ul[i],
                        region_lr[i] - window_lr[i]));
                }
            }
            else
            {
                if (region_lr[i] < window_ul[i]
                    || region_ul[i] >= window_lr[i])
                {
                    correction
                        = round_to_layout_scalar(region_ul[i] - window_ul[i]);
                }
            }
            if (correction != 0)
            {
                auto sb = this->sb(i);
                layout_scalar clamped = clamp_scroll_position(
                    sb, data.sb_data[i].scroll_position + correction);
                box.corner[i] += data.sb_data[i].scroll_position - clamped;
                set_logical_position(sb, clamped);
                if (request.abrupt)
                    reset_smoothing(data.sb_data[i]);
            }
        }

        parent->reveal_region({box, request.abrupt, request.move_to_top});
    }

    layout_box
    get_scrollbar_region(unsigned axis) const
    {
        layout_box region = get_assignment(this->cacher).region;
        region.corner[1 - axis]
            += region.size[1 - axis] - get_scrollbar_width(data.sb_data[axis]);
        region.size[1 - axis] = get_scrollbar_width(data.sb_data[axis]);
        return region;
    }

    scrollbar_parameters
    sb(unsigned axis) const
    {
        return scrollbar_parameters{
            &data.sb_data[axis],
            axis,
            this->get_scrollbar_region(axis),
            data.content_size[axis],
            data.window_size[axis],
            internal_widget_ref{*this},
            1 + int(axis) * scrollbar_element_id_count};
    }

    void
    handle_scrolling_key_press(modded_key const& key)
    {
        if (key.mods != 0)
            return;
        switch (key.code)
        {
            case key_code::UP:
                offset_logical_position(sb(1), -data.line_size);
                break;
            case key_code::DOWN:
                offset_logical_position(sb(1), data.line_size);
                break;
            case key_code::PAGE_UP:
                offset_logical_position(
                    sb(1),
                    -(std::max)(
                        data.window_size[1] - data.line_size, data.line_size));
                break;
            case key_code::PAGE_DOWN:
                offset_logical_position(
                    sb(1),
                    (std::max)(
                        data.window_size[1] - data.line_size, data.line_size));
                break;
            case key_code::LEFT:
                offset_logical_position(sb(0), -data.line_size);
                break;
            case key_code::RIGHT:
                offset_logical_position(sb(0), data.line_size);
                break;
            case key_code::HOME:
                set_logical_position(sb(1), 0);
                break;
            case key_code::END:
                set_logical_position(
                    sb(1), data.content_size[1] - data.window_size[1]);
                break;
        }
    }

    mutable scrollable_view_data data;

    column_layout_logic* logic;
    layout_cacher cacher;
    layout_vector assigned_size;

    value_smoother<float> smoother;
};

#if 0

void
scrollable_region::begin(
    context ctx,
    layout const& layout_spec,
    unsigned scrollable_axes,
    widget_id id,
    optional_storage<layout_vector> const& scroll_position_storage,
    unsigned reserved_axes)
{
    ctx_ = &ctx;
    id_ = id;

    if (get_cached_data(ctx, &data_))
    {
        data_->scroll_position = data_->smoothed_scroll_position
            = make_layout_vector(0, 0);
        data_->container.data_ = data_;
    }
    scrollable_view_data& data = *data_;

    // Determine where the scroll position is actually supposed to be stored,
    // and handle requests to set its value.
    accessor_mux<
        input_accessor<bool>,
        indirect_accessor<layout_vector>,
        inout_accessor<layout_vector>>
        position
        = resolve_storage(scroll_position_storage, &data.scroll_position);
    if (data.scroll_position_changed)
    {
        position.set(data.scroll_position);
        data.scroll_position_changed = false;
    }

    // Get the smoothed version of the scroll position.
    for (unsigned i = 0; i != 2; ++i)
    {
        layout_scalar smoothed = smooth_raw_value(
            ctx,
            data.smoothers[i],
            data.scroll_position[i],
            animated_transition(default_curve, 350));
        data.smoothed_scroll_position[i]
            = clamp_scroll_position(data, i, smoothed);
    }

    slc_.begin(get_layout_traversal(ctx), &data.container);

    srr_.begin(ctx.routing);

    alia_untracked_if(is_refresh_pass(ctx))
    {
        if (is_gettable(position))
            data.scroll_position = get(position);

        refresh_scrollbar_data(ctx, data.hsb_data);
        refresh_scrollbar_data(ctx, data.vsb_data);

        detect_layout_change(
            get_layout_traversal(ctx), &data.scrollable_axes, scrollable_axes);
        detect_layout_change(
            get_layout_traversal(ctx), &data.reserved_axes, reserved_axes);

        update_layout_cacher(
            get_layout_traversal(ctx),
            data.container.cacher_,
            layout_spec,
            FILL | UNPADDED);

        detect_layout_change(
            ctx, &data.scrollbar_width, get_scrollbar_width(data.vsb_data));
        detect_layout_change(
            ctx,
            &data.minimum_window_size,
            get_minimum_scrollbar_length(data.vsb_data));

        data.line_size = as_layout_size(resolve_absolute_length(
            get_layout_traversal(ctx), 0, absolute_length(6, EM)));
    }
    alia_untracked_else
    {
        layout_vector window_corner
            = get_assignment(data.container.cacher_).region.corner;

        hit_test_box_region(
            ctx,
            id,
            layout_box(window_corner, data.window_size),
            HIT_TEST_WHEEL);

        float movement;
        if (detect_wheel_movement(ctx, &movement, id))
        {
            set_scroll_position(
                data,
                1,
                clamp_scroll_position(
                    data,
                    1,
                    data.scroll_position[1]
                        - round_to_layout_scalar(data.line_size * movement)));
        }

        if (data.hsb_on)
        {
            state_proxy<layout_scalar> proxy(data.smoothed_scroll_position[0]);
            do_scrollbar(
                ctx,
                data.hsb_data,
                0,
                make_accessor(proxy),
                layout_box(
                    window_corner + make_layout_vector(0, data.window_size[1]),
                    make_layout_vector(
                        data.window_size[0], data.scrollbar_width)),
                data.content_size[0],
                data.window_size[0],
                data.line_size,
                data.window_size[0]);
            if (proxy.was_set())
                set_scroll_position_abruptly(data, 0, proxy.get());
        }
        if (data.vsb_on)
        {
            state_proxy<layout_scalar> proxy(data.smoothed_scroll_position[1]);
            do_scrollbar(
                ctx,
                data.vsb_data,
                1,
                make_accessor(proxy),
                layout_box(
                    window_corner + make_layout_vector(data.window_size[0], 0),
                    make_layout_vector(
                        data.scrollbar_width, data.window_size[1])),
                data.content_size[1],
                data.window_size[1],
                data.line_size,
                data.window_size[1]);
            if (proxy.was_set())
                set_scroll_position_abruptly(data, 1, proxy.get());
        }
        if (data.hsb_on && data.vsb_on)
        {
            scrollbar_junction_renderer const* junction_renderer;
            static default_scrollbar_junction_renderer
                default_junction_renderer;
            get_themed_renderer(
                ctx,
                data.junction_rendering,
                &junction_renderer,
                &default_junction_renderer);
            alia_tracked_block(data.junction_rendering.drawing_block)
            {
                junction_renderer->draw(
                    ctx, layout_box(window_corner, data.window_size));
            }
            alia_end
        }

        scr_.begin(*get_layout_traversal(ctx).geometry);
        scr_.set(box<2, double>(
            vector<2, double>(window_corner),
            vector<2, double>(data.window_size)));

        transform_.begin(*get_layout_traversal(ctx).geometry);
        transform_.set(translation_matrix(
            vector<2, double>(window_corner - data.smoothed_scroll_position)));
    }
    alia_end
}

#endif

void
get_scrollable_view(
    ui_context ctx,
    std::shared_ptr<scrollable_view>** container,
    layout const& layout_spec)
{
    if (get_data(ctx, container))
        **container = std::make_shared<scrollable_view>();

    if (get_layout_traversal(ctx).is_refresh_pass)
    {
        (**container)->refresh(ctx);

        if (update_layout_cacher(
                get_layout_traversal(ctx),
                (**container)->cacher,
                layout_spec,
                FILL | UNPADDED))
        {
            // Since this container isn't active yet, it didn't get marked
            // as needing recalculation, so we need to do that manually
            // here.
            (**container)->last_content_change
                = get_layout_traversal(ctx).refresh_counter;
        }
    }
}

void
scoped_scrollable_view::begin(ui_context ctx, layout const& layout_spec)
{
    std::shared_ptr<scrollable_view>* container;
    get_scrollable_view(ctx, &container, layout_spec);
    container_.begin(get_layout_traversal(ctx), container->get());
}

void
scoped_scrollable_view::end()
{
    container_.end();
}

} // namespace alia
