#include "alia/core/flow/events.hpp"
#include "alia/ui/utilities/regions.hpp"
#include <alia/ui/scrolling.hpp>

#include <algorithm>

#include <alia/core/signals/core.hpp>
#include <alia/core/timing/smoothing.hpp>
#include <alia/core/timing/timer.hpp>
#include <alia/ui/common.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/library.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/styling.hpp>
#include <alia/ui/system/input_constants.hpp>
#include <alia/ui/system/object.hpp>
#include <alia/ui/utilities/keyboard.hpp>
#include <alia/ui/utilities/mouse.hpp>
#include <alia/ui/utilities/regions.hpp>
#include <alia/ui/utilities/skia.hpp>
#include <alia/ui/utilities/widgets.hpp>

#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>

#include <ratio>

namespace alia {

struct scrollbar_renderer
{
    virtual scrollbar_metrics
    get_metrics(dataless_ui_context ctx) const
        = 0;

    virtual void
    draw_background(
        dataless_ui_context ctx,
        scrollbar_metrics const& metrics,
        layout_box const& rect,
        unsigned axis,
        unsigned which,
        widget_state state) const
        = 0;

    virtual void
    draw_thumb(
        dataless_ui_context ctx,
        scrollbar_metrics const& metrics,
        layout_box const& rect,
        unsigned axis,
        widget_state state) const
        = 0;

    virtual void
    draw_button(
        dataless_ui_context ctx,
        scrollbar_metrics const& metrics,
        layout_vector const& position,
        unsigned axis,
        unsigned which,
        widget_state state) const
        = 0;
};

// A scrollbar junction is the little square where two scrollbars meet.
struct scrollbar_junction_renderer
{
    virtual void
    draw(dataless_ui_context ctx, layout_box const& position) const
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
        dataless_ui_context ctx,
        scrollbar_metrics const&,
        layout_box const& rect,
        unsigned,
        unsigned,
        widget_state) const override
    {
        SkPaint paint;
        paint.setAntiAlias(true);
        // paint.setColor(SkColorSetARGB(12, 0, 0, 0));
        paint.setColor(SkColorSetARGB(12, 0xff, 0xff, 0xff));

        cast_event<render_event>(ctx).canvas->drawRect(as_skrect(rect), paint);
    }

    // thumb
    void
    draw_thumb(
        dataless_ui_context ctx,
        scrollbar_metrics const&,
        layout_box const& rect,
        unsigned,
        widget_state) const override
    {
        SkPaint paint;
        paint.setAntiAlias(true);
        // paint.setColor(SkColorSetARGB(60, 0, 0, 0));
        paint.setColor(SkColorSetARGB(60, 0xff, 0xff, 0xff));

        cast_event<render_event>(ctx).canvas->drawRect(as_skrect(rect), paint);
    }

    // button
    void
    draw_button(
        dataless_ui_context ctx,
        scrollbar_metrics const&,
        layout_vector const& p,
        unsigned,
        unsigned,
        widget_state) const override
    {
        SkPaint paint;
        paint.setAntiAlias(true);
        // paint.setColor(SkColorSetARGB(40, 0, 0, 0));
        paint.setColor(SkColorSetARGB(40, 0xff, 0xff, 0xff));

        cast_event<render_event>(ctx).canvas->drawRect(
            as_skrect(layout_box(p, make_layout_vector(30, 30))), paint);
    }
};

struct default_scrollbar_junction_renderer : scrollbar_junction_renderer
{
    void
    draw(dataless_ui_context, layout_box const&) const override
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

widget_id
get_thumb_id(scrollbar_parameters const& sb)
{
    return offset_id(sb.data, 0);
}

widget_id
get_button0_id(scrollbar_parameters const& sb)
{
    return offset_id(sb.data, 1);
}

widget_id
get_button1_id(scrollbar_parameters const& sb)
{
    return offset_id(sb.data, 2);
}

widget_id
get_bg0_id(scrollbar_parameters const& sb)
{
    return offset_id(sb.data, 3);
}

widget_id
get_bg1_id(scrollbar_parameters const& sb)
{
    return offset_id(sb.data, 4);
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
    dataless_ui_context ctx,
    scrollbar_parameters const& sb,
    widget_id id,
    layout_scalar increment)
{
    static int const delay_after_first_increment = 400;
    static int const delay_after_other_increment = 40;

    if (detect_mouse_press(ctx, id, mouse_button::LEFT))
    {
        offset_logical_position(sb, increment);
        start_timer(ctx, sb.data->timer, delay_after_first_increment);
    }
    else if (
        is_click_in_progress(ctx, id, mouse_button::LEFT)
        && detect_timer_event(ctx, sb.data->timer))
    {
        offset_logical_position(sb, increment);
        restart_timer(ctx, sb.data->timer, delay_after_other_increment);
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
    return sb.content_size > sb.window_size
               ? std::clamp(
                     position,
                     layout_scalar(0),
                     sb.content_size - sb.window_size)
               : 0;
}

inline void
refresh_scrollbar_metrics(dataless_ui_context ctx, scrollbar_data& data)
{
    // TODO: Parameterize renderer.
    auto* renderer = &default_renderer;

    data.metrics.refresh(/* TODO: *ctx.style.id */ unit_id, [&] {
        return renderer->get_metrics(ctx);
    });
}

inline void
update_smoothed_position(dataless_ui_context ctx, scrollbar_data& data)
{
    // Update the smoothed version of the scroll position.
    for (unsigned i = 0; i != 2; ++i)
    {
        data.smoothed_scroll_position = smooth_raw(
            ctx,
            data.smoother,
            data.scroll_position,
            animated_transition{linear_curve, 120});
    }
}

void
do_scrollbar_pass(dataless_ui_context ctx, scrollbar_parameters const& sb)
{
    assert(sb.axis == 0 || sb.axis == 1);

    if (!scrollbar_is_valid(sb))
        return;

    auto& data = *sb.data;

    // TODO: Parameterize renderer.
    auto* renderer = &default_renderer;

    switch (get_event_category(ctx))
    {
        case REGION_CATEGORY:
            do_box_region(ctx, get_bg0_id(sb), get_bg0_area(sb));
            do_box_region(ctx, get_bg1_id(sb), get_bg1_area(sb));
            do_box_region(ctx, get_thumb_id(sb), get_thumb_area(sb));
            do_box_region(ctx, get_button0_id(sb), get_button0_area(sb));
            do_box_region(ctx, get_button1_id(sb), get_button1_area(sb));
            break;

        case INPUT_CATEGORY:
            process_button_input(
                ctx, sb, get_bg0_id(sb), -get_page_increment(sb));
            process_button_input(
                ctx, sb, get_bg1_id(sb), get_page_increment(sb));

            if (detect_mouse_press(ctx, get_thumb_id(sb), mouse_button::LEFT))
            {
                data.drag_start_delta = data.physical_position
                                        - get_mouse_position(ctx)[sb.axis];
            }
            if (detect_drag(ctx, get_thumb_id(sb), mouse_button::LEFT))
            {
                set_physical_position_abruptly(
                    sb,
                    layout_scalar(
                        get_mouse_position(ctx)[sb.axis]
                        + data.drag_start_delta));
            }

            process_button_input(
                ctx, sb, get_button0_id(sb), -get_line_increment(sb));
            process_button_input(
                ctx, sb, get_button1_id(sb), get_line_increment(sb));

            break;

        case RENDER_CATEGORY:
            if (get_max_physical_position(sb) < 0)
            {
                // In this case, the scrollbar is too small to function, so
                // just draw the background and return.
                if (sb.area.size[0] > 0 && sb.area.size[1] > 0)
                {
                    renderer->draw_background(
                        ctx,
                        *data.metrics,
                        sb.area,
                        sb.axis,
                        0,
                        WIDGET_NORMAL);
                }
                return;
            }

            // If the thumb isn't being dragged, then the physical position
            // should stay in sync with the logical position.
            if (!is_drag_in_progress(
                    ctx, get_thumb_id(sb), mouse_button::LEFT))
            {
                data.physical_position
                    = logical_position_to_physical(sb, data.scroll_position);
            }

            renderer->draw_background(
                ctx,
                *data.metrics,
                get_bg0_area(sb),
                sb.axis,
                0,
                get_widget_state(ctx, get_bg0_id(sb)));
            renderer->draw_background(
                ctx,
                *data.metrics,
                get_bg1_area(sb),
                sb.axis,
                1,
                get_widget_state(ctx, get_bg1_id(sb)));
            renderer->draw_thumb(
                ctx,
                *data.metrics,
                get_thumb_area(sb),
                sb.axis,
                get_widget_state(ctx, get_thumb_id(sb)));
            renderer->draw_button(
                ctx,
                *data.metrics,
                get_button0_area(sb).corner,
                sb.axis,
                0,
                get_widget_state(ctx, get_button0_id(sb)));
            renderer->draw_button(
                ctx,
                *data.metrics,
                get_button1_area(sb).corner,
                sb.axis,
                1,
                get_widget_state(ctx, get_button1_id(sb)));
    }
}

struct scrollable_view_data;

struct scrollable_layout_container : layout_container
{
    // implementation of layout interface
    layout_requirements
    get_horizontal_requirements() override;
    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width) override;
    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override;

    // associated data
    scrollable_view_data* data_;

    // layout cacher
    layout_cacher cacher_;

    // logic for internal layout
    column_layout_logic logic_;
};

// TODO
// constexpr unsigned scrollable_bit_offset = 0;
// constexpr unsigned reserved_axes_bit_offset = 2;
// constexpr unsigned scrollbars_on_bit_offset = 2;

// persistent data required for a scrollable view
struct scrollable_view_data
{
    // set by caller and copied here
    unsigned scrollable_axes = 0, reserved_axes = 0;

    // determined at usage site and needed by layout
    layout_scalar minimum_window_size, line_size;

    // determined by layout and stored here for general usage
    unsigned scrollbars_on;
    layout_vector content_size, window_size;

    // persistent data for scrollbars
    scrollbar_data sb_data[2];

    // layout container
    scrollable_layout_container container;
};

void
set_scrollbar_on(scrollable_view_data& data, unsigned axis, bool on)
{
    unsigned const masks[] = {2, 1};
    data.scrollbars_on
        = (data.scrollbars_on & masks[axis]) | ((on ? 1 : 0) << axis);
}

bool
is_scrollbar_on(scrollable_view_data const& data, unsigned axis)
{
    return (data.scrollbars_on & (1 << axis)) != 0;
}

inline bool
is_axis_reserved(scrollable_view_data const& data, unsigned axis)
{
    return (data.reserved_axes & (1 << axis)) != 0;
}

layout_box
get_scrollbar_region(scrollable_view_data const& data, unsigned axis)
{
    auto const& cacher = data.container.cacher_;
    layout_box region = get_assignment(cacher).region;
    region.corner[1 - axis]
        += region.size[1 - axis] - get_scrollbar_width(data.sb_data[axis]);
    region.size[1 - axis] = get_scrollbar_width(data.sb_data[axis]);
    return region;
}

scrollbar_parameters
construct_scrollbar(scrollable_view_data& data, unsigned axis)
{
    return scrollbar_parameters{
        &data.sb_data[axis],
        axis,
        get_scrollbar_region(data, axis),
        data.content_size[axis],
        data.window_size[axis]};
}

layout_requirements
scrollable_layout_container::get_horizontal_requirements()
{
    auto& data = *data_;
    return cache_horizontal_layout_requirements(
        cacher_, this->last_content_change, [&] {
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
                    = logic_.get_horizontal_requirements(children);
                layout_scalar required_width = x.size;
                if ((data.scrollable_axes & 2) != 0)
                    required_width += get_scrollbar_width(data.sb_data[1]);
                return calculated_layout_requirements{required_width, 0, 0};
            }
        });
}

layout_requirements
scrollable_layout_container::get_vertical_requirements(
    layout_scalar assigned_width)
{
    auto& data = *data_;
    return cache_vertical_layout_requirements(
        cacher_, this->last_content_change, assigned_width, [&] {
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
                auto x = this->get_horizontal_requirements();
                layout_scalar resolved_width = resolve_assigned_width(
                    cacher_.resolved_spec, assigned_width, x);
                layout_scalar actual_width
                    = (std::max)(resolved_width, x.size);
                calculated_layout_requirements y
                    = logic_.get_vertical_requirements(children, actual_width);
                layout_scalar required_height = y.size;
                // Add space for the horizontal scrollbar, if applicable.
                if ((data.scrollable_axes & 1) != 0 && x.size > resolved_width)
                    required_height += get_scrollbar_width(data.sb_data[0]);
                return calculated_layout_requirements{required_height, 0, 0};
            }
        });
}

void
scrollable_layout_container::set_relative_assignment(
    relative_layout_assignment const& assignment)
{
    auto& data = *data_;
    // std::cout << "(scrollable) sra: " << assignment.region << std::endl;
    update_relative_assignment(
        *this,
        cacher_,
        this->last_content_change,
        assignment,
        [&](auto const& resolved_assignment) {
            // Assigning a region to scrolled content means establishing the
            // dimensions of the virtual region that we're scrolling over, and
            // this means we need to decide which scrollbars will actually be
            // turned on...

            layout_vector available_size = resolved_assignment.region.size;

            // Some helpers for reserving space for scrollbars in both
            // dimensions...
            bool reserving_scrollbar_space[2] = {false, false};
            auto reserve_scrollbar_space = [&](unsigned axis) {
                if (!reserving_scrollbar_space[axis])
                {
                    reserving_scrollbar_space[axis] = true;
                    available_size[axis]
                        -= get_scrollbar_width(data.sb_data[1 - axis]);
                }
            };
            auto release_scrollbar_space = [&](unsigned axis) {
                if (reserving_scrollbar_space[axis])
                {
                    reserving_scrollbar_space[axis] = false;
                    available_size[axis]
                        += get_scrollbar_width(data.sb_data[1 - axis]);
                }
            };

            // First determine the horizontal requirements of the content,
            // since these are (by definition) independent of everything else.
            calculated_layout_requirements x
                = logic_.get_horizontal_requirements(children);

            // helper to handle the horizontal scrollbar and its space
            auto update_horizontal_scrollbar_state = [&] {
                set_scrollbar_on(data, 0, available_size[0] < x.size);
                if (available_size[0] < x.size || is_axis_reserved(data, 0))
                    reserve_scrollbar_space(1);
                else
                    release_scrollbar_space(1);
            };

            // We'll need to determine the vertical requirements based on
            // whether or not there is space reserved for the vertical
            // scrollbar...
            calculated_layout_requirements y;

            // If space will always be reserved for the vertical scrollbar,
            // then we know the available width already and the logic is
            // fairly simple.
            if (is_axis_reserved(data, 1))
            {
                reserve_scrollbar_space(0);

                y = logic_.get_vertical_requirements(
                    children, available_size[0]);

                update_horizontal_scrollbar_state();

                // Now we have enough info to decide if we need a vertical
                // scrollbar.
                if (available_size[1] < y.size)
                    set_scrollbar_on(data, 1, true);
            }
            // Otherwise, we're not reserving space for the vertical
            // scrollbar, so we need to determine dynamically if we want one.
            else
            {
                // If the scrollbar is currently on, try measuring the vertical
                // requirements assuming that it will stay on...
                if (is_scrollbar_on(data, 1))
                {
                    reserve_scrollbar_space(0);
                    y = logic_.get_vertical_requirements(
                        children, available_size[0]);

                    update_horizontal_scrollbar_state();

                    // If we were right about still needing the vertical
                    // scrollbar, we are done.
                    // TODO: Add a way to detect cases where removing the
                    // vertical scrollbar would actually allow the content to
                    // fit without scrolling.
                    if (available_size[1] < y.size)
                        goto scrollbar_logic_done;

                    // Otherwise, give back the space and try without a
                    // scrollbar...
                    release_scrollbar_space(0);
                }

                // Attempt to fit the content without using a scrollbar.
                y = logic_.get_vertical_requirements(
                    children, available_size[0]);

                update_horizontal_scrollbar_state();

                // If we have enough space, we can leave the scrollbar off.
                if (available_size[1] >= y.size)
                {
                    set_scrollbar_on(data, 1, false);
                    goto scrollbar_logic_done;
                }

                // Otherwise, we need to turn the vertical scrollbar on and
                // recalculate the vertical requirements.
                set_scrollbar_on(data, 1, true);
                reserve_scrollbar_space(0);
                y = logic_.get_vertical_requirements(
                    children, available_size[0]);
                update_horizontal_scrollbar_state();
            }

        scrollbar_logic_done:

            layout_scalar content_width
                = (std::max)(available_size[0], x.size);
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
                layout_scalar sp = data.sb_data[axis].smoothed_scroll_position;
                if (sp != 0
                    && sp + data.window_size[axis] >= data.content_size[axis]
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
                        construct_scrollbar(*data_, axis),
                        content_size[axis] - available_size[axis]);
                }
            }

            // If the scroll position needs to be clamped because of
            // changes in content size, then do it abruptly, not
            // smoothly.
            for (unsigned i = 0; i != 2; ++i)
            {
                layout_scalar original
                    = data.sb_data[i].smoothed_scroll_position;
                auto clamped = clamp_scroll_position(
                    construct_scrollbar(*data_, i), original);
                if (clamped != original)
                {
                    set_logical_position_abruptly(
                        construct_scrollbar(*data_, i), clamped);
                }
            }

            logic_.set_relative_assignment(
                children, content_size, content_height - y.descent);
        });
}

void
handle_scrolling_key_press(scrollable_view_data& data, modded_key const& key)
{
    if (key.mods.code != 0)
        return;
    switch (key.code)
    {
        case key_code::UP:
            offset_logical_position(
                construct_scrollbar(data, 1), -data.line_size);
            break;
        case key_code::DOWN:
            offset_logical_position(
                construct_scrollbar(data, 1), data.line_size);
            break;
        case key_code::PAGE_UP:
            offset_logical_position(
                construct_scrollbar(data, 1),
                -(std::max)(
                    data.window_size[1] - data.line_size, data.line_size));
            break;
        case key_code::PAGE_DOWN:
            offset_logical_position(
                construct_scrollbar(data, 1),
                (std::max)(
                    data.window_size[1] - data.line_size, data.line_size));
            break;
        case key_code::LEFT:
            offset_logical_position(
                construct_scrollbar(data, 0), -data.line_size);
            break;
        case key_code::RIGHT:
            offset_logical_position(
                construct_scrollbar(data, 0), data.line_size);
            break;
        case key_code::HOME:
            set_logical_position(construct_scrollbar(data, 1), 0);
            break;
        case key_code::END:
            set_logical_position(
                construct_scrollbar(data, 1),
                data.content_size[1] - data.window_size[1]);
            break;
        default:
            break;
    }
}

#if 0

struct scrollable_view : widget_container
{
    void
    process_input(ui_event_context ctx) override
    {
        auto delta = detect_scroll(ctx, widget_id{*this, 0});
        if (delta)
        {
            set_logical_position_abruptly(
                sb(1),
                data.sb_data[1].scroll_position -= float((*delta)[1] * 120));
        }

        process_scrollbar_input(ctx, sb(1));

        focus_on_click(ctx, widget_id{*this, 0});
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

    mutable scrollable_view_data data;

    column_layout_logic* logic;
    layout_cacher cacher;
    layout_vector assigned_size;

    value_smoother<float> smoother;
};

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

#endif

void
scoped_scrollable_view::begin(
    ui_context ctx,
    layout const& layout_spec,
    unsigned scrollable_axes,
    unsigned reserved_axes)
{
    ctx_.reset(ctx);

    if (get_cached_data(ctx, &data_))
        data_->container.data_ = data_;
    auto& data = *data_;

    widget_id id = data_;

    container_.begin(get_layout_traversal(ctx), &data.container);

    alia_untracked_if(is_refresh_pass(ctx))
    {
        detect_layout_change(
            get_layout_traversal(ctx), &data.scrollable_axes, scrollable_axes);
        detect_layout_change(
            get_layout_traversal(ctx), &data.reserved_axes, reserved_axes);

        for (unsigned axis = 0; axis != 2; ++axis)
        {
            refresh_scrollbar_metrics(ctx, data.sb_data[axis]);
            if (is_scrollbar_on(data, axis))
                update_smoothed_position(ctx, data.sb_data[axis]);
        }

        // TODO
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

        // TODO
        // detect_layout_change(
        //     ctx, &data.scrollbar_width, get_scrollbar_width(data.vsb_data));
        data.minimum_window_size = 120;

        // TODO
        // detect_layout_change(
        //     ctx,
        //     &data.minimum_window_size,
        //     get_minimum_scrollbar_length(data.vsb_data));

        // TODO
        // data.line_size = as_layout_size(resolve_absolute_length(
        //     get_layout_traversal(ctx), 0, absolute_length(6, EM)));
        data.line_size = 60;

        detect_layout_change(
            get_layout_traversal(ctx), &data.scrollable_axes, scrollable_axes);
        detect_layout_change(
            get_layout_traversal(ctx), &data.reserved_axes, reserved_axes);

        update_layout_cacher(
            get_layout_traversal(ctx),
            data.container.cacher_,
            layout_spec,
            FILL | UNPADDED);
    }
    alia_untracked_else
    {
        layout_vector window_corner
            = get_assignment(data.container.cacher_).region.corner;

        if (get_event_category(ctx) == REGION_CATEGORY)
        {
            hit_test_box_region(
                ctx,
                id,
                layout_box(window_corner, data.window_size),
                HIT_TEST_WHEEL);
        }

        if (get_event_category(ctx) == INPUT_CATEGORY)
        {
            auto delta = detect_scroll(ctx, id);
            if (delta)
            {
                for (unsigned axis = 0; axis != 2; ++axis)
                {
                    if ((*delta)[axis] != 0)
                    {
                        set_logical_position_abruptly(
                            construct_scrollbar(data, axis),
                            data.sb_data[axis].scroll_position
                            -= float((*delta)[axis] * 120));
                    }
                }
            }

            focus_on_click(ctx, id);
            auto key = detect_key_press(ctx, id);
            if (key)
                handle_scrolling_key_press(data, *key);
        }

        for (unsigned axis = 0; axis != 2; ++axis)
        {
            if (is_scrollbar_on(data, axis))
                do_scrollbar_pass(ctx, construct_scrollbar(data, axis));
        }

        if (get_event_category(ctx) == RENDER_CATEGORY)
        {
            if (is_scrollbar_on(data, 0) && is_scrollbar_on(data, 1))
            {
                static default_scrollbar_junction_renderer
                    default_junction_renderer;
                scrollbar_junction_renderer const* junction_renderer
                    = &default_junction_renderer;
                junction_renderer->draw(
                    ctx, layout_box(window_corner, data.window_size));
            }
        }

        clip_region_.begin(*get_layout_traversal(ctx).geometry);
        clip_region_.set(box<2, double>(
            vector<2, double>(window_corner),
            vector<2, double>(data.window_size)));

        auto scroll_position = make_vector<double>(0, 0);
        for (unsigned axis = 0; axis != 2; ++axis)
        {
            if (is_scrollbar_on(data, axis))
                scroll_position[axis]
                    = data.sb_data[axis].smoothed_scroll_position;
        }

        transform_.begin(*get_layout_traversal(ctx).geometry);
        transform_.set(translation_matrix(
            vector<2, double>(window_corner) - scroll_position));
    }
    alia_end
}

void
scoped_scrollable_view::end()
{
    if (ctx_)
    {
        dataless_ui_context ctx = *ctx_;
        switch (get_event_category(ctx))
        {
                // case REGION_CATEGORY:
                //     if (get_event_type(ctx) == MAKE_WIDGET_VISIBLE_EVENT
                //         && srr_.is_relevant())
                //     {
                //         make_widget_visible_event& e
                //             = get_event<make_widget_visible_event>(ctx);
                //         if (e.acknowledged)
                //             handle_visibility_request(ctx, *data_, e);
                //     }
                //     break;

            case INPUT_CATEGORY:
                // if (srr_.is_relevant() || id_has_focus(ctx, id_))
                // {
                //     key_event_info info;
                //     if (detect_background_key_press(ctx, &info))
                //         handle_scrolling_key_press(*data_, info);
                // }
                break;
        }
    }
}

} // namespace alia
