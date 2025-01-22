#include <alia/ui/scrolling.hpp>

#include <algorithm>

#include <alia/core/flow/components.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/core/signals/core.hpp>
#include <alia/core/timing/smoothing.hpp>
#include <alia/core/timing/timer.hpp>
#include <alia/ui/common.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/simple.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/styling.hpp>
#include <alia/ui/system/input_constants.hpp>
#include <alia/ui/system/object.hpp>
#include <alia/ui/utilities/keyboard.hpp>
#include <alia/ui/utilities/mouse.hpp>
#include <alia/ui/utilities/regions.hpp>
#include <alia/ui/utilities/rendering.hpp>
#include <alia/ui/utilities/skia.hpp>
#include <alia/ui/utilities/widgets.hpp>

#ifdef _WIN32
#pragma warning(push, 0)
#endif

#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>

#ifdef _WIN32
#pragma warning(pop)
#endif

namespace alia {

scrollbar_metrics
get_scrollbar_metrics(dataless_ui_context)
{
    // TODO: Adjust this for DPI and UI scaling.
    scrollbar_metrics metrics;
    metrics.width = 30;
    metrics.button_length = 30;
    metrics.minimum_thumb_length = 30;
    return metrics;
}

scrollbar_style_info
extract_scrollbar_style_info(dataless_ui_context ctx)
{
    auto const& theme = get_system(ctx).theme;
    return {
        .track_color = theme.surface_container_levels[3],
        .thumb_color = interpolate(
            theme.surface_container_levels[4], theme.on_surface_variant, 0.5f),
        .thumb_highlight_color = theme.primary,
        .button_background_color = theme.surface_container_levels[4],
        .button_foreground_color = theme.secondary,
        .button_highlight_color = theme.primary,
    };
}

void
draw_scrollbar_background(
    dataless_ui_context ctx,
    scrollbar_metrics const&,
    scrollbar_style_info const& style,
    layout_box const& rect,
    interaction_status)
{
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(as_skcolor(style.track_color));
    cast_event<render_event>(ctx).canvas->drawRect(as_skrect(rect), paint);
}

void
draw_scrollbar_thumb(
    dataless_ui_context ctx,
    scrollbar_metrics const&,
    scrollbar_style_info const& style,
    layout_box const& rect,
    interaction_status status)
{
    SkPaint paint;
    paint.setAntiAlias(true);
    if (is_hovered(status))
    {
        paint.setColor(as_skcolor(interpolate(
            style.thumb_color, style.thumb_highlight_color, 0.2f)));
    }
    else if (is_active(status))
    {
        paint.setColor(as_skcolor(interpolate(
            style.thumb_color, style.thumb_highlight_color, 0.4f)));
    }
    else
    {
        paint.setColor(as_skcolor(style.thumb_color));
    }
    cast_event<render_event>(ctx).canvas->drawRect(as_skrect(rect), paint);
}

void
draw_scrollbar_button(
    dataless_ui_context ctx,
    scrollbar_metrics const&,
    scrollbar_style_info const& style,
    layout_box const& rect,
    interaction_status status)
{
    SkPaint paint;
    paint.setAntiAlias(true);
    if (is_hovered(status))
    {
        paint.setColor(as_skcolor(interpolate(
            style.button_background_color,
            style.button_highlight_color,
            0.2f)));
    }
    else if (is_active(status))
    {
        paint.setColor(as_skcolor(interpolate(
            style.button_background_color,
            style.button_highlight_color,
            0.4f)));
    }
    else
    {
        paint.setColor(as_skcolor(style.button_background_color));
    }
    cast_event<render_event>(ctx).canvas->drawRect(as_skrect(rect), paint);
}

// persistent data maintained for a scrollbar
struct scrollbar_data
{
    // the actual scroll position - If the caller wants to provide an external
    // signal, it will be sync'd with this.
    layout_scalar scroll_position;

    // If this is true, the scroll_position has changed internally and needs
    // to be communicated to the external storage.
    bool scroll_position_changed = false;

    // the smoothed version of the scroll position
    layout_scalar smoothed_scroll_position;
    // for smoothing the scroll position
    value_smoother<layout_scalar> smoother;

    // cached metrics
    scrollbar_metrics metrics;

    // the relative position of the thumb within its track, in pixels
    layout_scalar physical_position = 0;

    // While dragging, this stores the offset from the mouse cursor to the
    // top of the thumb.
    double drag_start_delta = 0;

    // for timing button repeats
    timer_data timer;
};

// all the parameters that are necessary to define a scrollbar
struct scrollbar_parameters
{
    // persistent data for the scrollbar
    scrollbar_data* data;

    // 0 for horizontal, 1 for vertical
    unsigned axis;

    // surface region assigned to the scrollbar
    layout_box area;

    // total size of the content along the scrolling axis
    layout_scalar content_size;

    // size of the window through which we view the content
    layout_scalar view_size;
};

layout_scalar
get_scrollbar_width(scrollbar_data const& data)
{
    return data.metrics.width;
}

layout_scalar
get_minimum_scrollbar_length(scrollbar_data const& data)
{
    return data.metrics.minimum_thumb_length + 2 * data.metrics.button_length;
}

scrollbar_metrics
get_metrics(scrollbar_parameters const& sb)
{
    return sb.data->metrics;
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
        sb.view_size * bg_area.size[sb.axis] / sb.content_size);
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
    return sb.content_size - sb.view_size;
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
    return max_physical <= 0
             ? 0
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
    return sb.view_size;
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

bool
scrollbar_is_valid(scrollbar_parameters const& sb)
{
    return sb.content_size > 0 && sb.view_size > 0
        && sb.view_size < sb.content_size
        && get_max_physical_position(sb) >= 0;
}

layout_scalar
clamp_scroll_position(scrollbar_parameters const& sb, layout_scalar position)
{
    return sb.content_size > sb.view_size
             ? std::clamp(
                   position, layout_scalar(0), sb.content_size - sb.view_size)
             : 0;
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
                    // TODO
                    // renderer->draw_background(
                    //     ctx, data.metrics, sb.area, sb.axis, 0, NO_FLAGS);
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

            auto style = extract_scrollbar_style_info(ctx);

            draw_scrollbar_background(
                ctx,
                data.metrics,
                style,
                get_bg0_area(sb),
                get_interaction_status(ctx, get_bg0_id(sb)));
            draw_scrollbar_background(
                ctx,
                data.metrics,
                style,
                get_bg1_area(sb),
                get_interaction_status(ctx, get_bg1_id(sb)));
            draw_scrollbar_thumb(
                ctx,
                data.metrics,
                style,
                get_thumb_area(sb),
                get_interaction_status(ctx, get_thumb_id(sb)));
            draw_scrollbar_button(
                ctx,
                data.metrics,
                style,
                get_button0_area(sb),
                get_interaction_status(ctx, get_button0_id(sb)));
            draw_scrollbar_button(
                ctx,
                data.metrics,
                style,
                get_button1_area(sb),
                get_interaction_status(ctx, get_button1_id(sb)));
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
    // persistent data for scrollbars
    scrollbar_data sb_data[2];

    // layout container
    scrollable_layout_container container;

    // set by caller and copied here...

    // bit flags to indicate which axes are scrollable -
    unsigned scrollable_axes = 0;

    // bit flags to indicate which axes are 'reserved' - Reserved axes are
    // scrollable and have space reserved for their scrollbars (i.e., content
    // will not fill that area even when there are no scrollbars active).
    unsigned reserved_axes = 0;

    // the amount to scroll by when the user scrolls one 'line'
    layout_scalar line_size;

    // determined at usage site and needed by layout...

    // minimum size of the view in the direction that it scrolls
    layout_scalar minimum_view_size;

    // determined by layout and stored here for general usage...

    // bit flags to indicate which scrollbars are currently active
    unsigned scrollbars_on;

    // current size of the scrollable content
    layout_vector content_size;

    // current size of the window through which the content is visible
    layout_vector view_size;
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
    auto const& cacher = data.container.cacher;
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
        data.view_size[axis]};
}

layout_requirements
scrollable_layout_container::get_horizontal_requirements()
{
    auto& data = *data_;
    return cache_horizontal_layout_requirements(this->cacher, [&] {
        if ((data.scrollable_axes & 1) != 0)
        {
            // If the view is horizontally scrollable, then we only
            // need enough space for scrolling to happen.
            return calculated_layout_requirements{
                data.minimum_view_size, 0, 0};
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
        this->cacher, assigned_width, [&] {
            if ((data.scrollable_axes & 2) != 0)
            {
                // If the view is vertically scrollable, then we only
                // need enough space for scrolling to happen.
                return calculated_layout_requirements{
                    data.minimum_view_size, 0, 0};
            }
            else
            {
                // Otherwise, we need to calculate the requirements.
                auto x = this->get_horizontal_requirements();
                layout_scalar resolved_width = resolve_assigned_width(
                    this->cacher.resolved_spec, assigned_width, x);
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
    update_relative_assignment(
        *this, this->cacher, assignment, [&](auto const& resolved_assignment) {
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
                    && sp + data.view_size[axis] >= data.content_size[axis]
                    && sp + available_size[axis] < content_size[axis])
                {
                    pin_to_end[axis] = true;
                }
            }

            // Update the persistent copies of the content and view sizes.
            data.content_size = content_size;
            data.view_size = available_size;

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

static void
handle_visibility_request(
    dataless_ui_context& ctx,
    scrollable_view_data& data,
    make_widget_visible_event const& event)
{
    auto box = *event.region;
    auto const inverse_transform = inverse(get_transformation(ctx));
    auto const transformed_box
        = transform_box(inverse_transform, alia::box<2, double>(box));
    auto const region_ul = transformed_box.corner;
    auto const region_lr = get_high_corner(transformed_box);
    auto const window_ul = make_vector<double>(
        data.sb_data[0].scroll_position, data.sb_data[1].scroll_position);
    auto const window_lr = window_ul + vector<2, double>(data.view_size);
    for (int i = 0; i != 2; ++i)
    {
        layout_scalar correction = 0;
        if (event.flags & MOVE_TO_TOP)
        {
            correction = round_to_layout_scalar(region_ul[i] - window_ul[i]);
        }
        else if (transformed_box.size[i] <= double(data.view_size[i]))
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
                    region_ul[i] - window_ul[i], region_lr[i] - window_lr[i]));
            }
        }
        else
        {
            if (region_lr[i] < window_ul[i] || region_ul[i] >= window_lr[i])
            {
                correction
                    = round_to_layout_scalar(region_ul[i] - window_ul[i]);
            }
        }
        if (correction != 0)
        {
            auto sb = construct_scrollbar(data, i);
            layout_scalar clamped = clamp_scroll_position(
                sb, data.sb_data[i].scroll_position + correction);
            box.corner[i] += data.sb_data[i].scroll_position - clamped;
            set_logical_position(sb, clamped);
            if (event.flags & ABRUPT)
                reset_smoothing(data.sb_data[i]);
        }
    }
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
                    data.view_size[1] - data.line_size, data.line_size));
            break;
        case key_code::PAGE_DOWN:
            offset_logical_position(
                construct_scrollbar(data, 1),
                (std::max)(
                    data.view_size[1] - data.line_size, data.line_size));
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
                data.content_size[1] - data.view_size[1]);
            break;
        default:
            break;
    }
}

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

    component_.begin(ctx);

    alia_untracked_if (is_refresh_event(ctx))
    {
        detect_layout_change(
            get_layout_traversal(ctx), &data.scrollable_axes, scrollable_axes);
        detect_layout_change(
            get_layout_traversal(ctx), &data.reserved_axes, reserved_axes);

        for (unsigned axis = 0; axis != 2; ++axis)
        {
            auto const metrics = get_scrollbar_metrics(ctx);
            detect_layout_change(ctx, &data.sb_data[axis].metrics, metrics);
        }

        detect_layout_change(
            ctx,
            &data.minimum_view_size,
            get_minimum_scrollbar_length(data.sb_data[1]));

        for (unsigned axis = 0; axis != 2; ++axis)
        {
            if (is_scrollbar_on(data, axis))
                update_smoothed_position(ctx, data.sb_data[axis]);
        }

        // TODO: Make this a parameter and/or query it from the system.
        // TODO: Factor in DPI and UI scaling for the default value.
        data.line_size = 72;

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

        update_layout_cacher(
            get_layout_traversal(ctx),
            data.container.cacher,
            layout_spec,
            FILL | UNPADDED);
    }
    alia_untracked_else
    {
        layout_vector window_corner
            = get_assignment(data.container.cacher).region.corner;

        if (get_event_category(ctx) == REGION_CATEGORY)
        {
            hit_test_box_region(
                ctx,
                id,
                layout_box(window_corner, data.view_size),
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
                // TODO
                // junction_renderer->draw(
                //     ctx, layout_box(window_corner, data.view_size));
            }
        }

        clip_region_.begin(*get_layout_traversal(ctx).geometry);
        clip_region_.set(box<2, double>(
            vector<2, double>(window_corner),
            vector<2, double>(data.view_size)));

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
            case REGION_CATEGORY:
                if (get_event_type(ctx) == MAKE_WIDGET_VISIBLE_EVENT
                    && component_.is_on_route())
                {
                    auto const& e = cast_event<make_widget_visible_event>(ctx);
                    if (e.region)
                        handle_visibility_request(ctx, *data_, e);
                }
                break;

            case INPUT_CATEGORY:
                if (component_.is_on_route())
                {
                    if (auto key = detect_key_press(ctx))
                        handle_scrolling_key_press(*data_, *key);
                }
                break;
        }

        transform_.end();
        clip_region_.end();
        component_.end();
        container_.end();

        ctx_.reset();
    }
}

} // namespace alia
