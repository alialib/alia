#include <alia/indie/scrolling.hpp>

#include <alia/indie/common.hpp>
#include <alia/indie/context.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/layout/utilities.hpp>
#include <alia/indie/styling.hpp>
#include <alia/indie/utilities/hit_testing.hpp>
#include <alia/indie/utilities/keyboard.hpp>
#include <alia/indie/utilities/mouse.hpp>
#include <alia/indie/utilities/skia.hpp>
#include <alia/indie/widget.hpp>

#include <include/core/SkColor.h>

#include <algorithm>

namespace alia { namespace indie {

// TODO: Move

element_state
get_element_state(
    system& sys,
    internal_element_ref element,
    element_state overrides = NO_FLAGS)
{
    element_state state;
    if (!(overrides & WIDGET_DISABLED))
    {
        if (overrides & WIDGET_SELECTED)
        {
            state = WIDGET_SELECTED;
        }
        else if (
            is_click_in_progress(sys, element, mouse_button::LEFT)
            || (overrides & WIDGET_DEPRESSED))
        {
            state = WIDGET_DEPRESSED;
        }
        else if (is_click_possible(sys, element))
        {
            state = WIDGET_HOT;
        }
        else
        {
            state = WIDGET_NORMAL;
        }
        if (element_has_focus(sys, element) && sys.input.window_has_focus
            && sys.input.keyboard_interaction)
        {
            state |= WIDGET_FOCUSED;
        }
    }
    else
    {
        state = WIDGET_DISABLED;
    }
    return state;
}

struct scrollbar_renderer
{
    virtual scrollbar_metrics
    get_metrics(dataless_context ctx) const
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
    get_metrics(dataless_context ctx) const override
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
        metrics.width = as_layout_size(resolve_absolute_length(
            get_layout_traversal(ctx), 0, absolute_length(0.8f, EM)));
        metrics.button_length = 0;
        metrics.minimum_thumb_length = 0;
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
        render_event&,
        scrollbar_metrics const&,
        layout_box const&,
        unsigned,
        unsigned,
        element_state) const override
    {
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
        // ALIA_GET_CACHED_DATA(caching_renderer_data);

        // if (!is_render_pass(ctx))
        //     return;

        // caching_renderer cache(
        //     ctx, data, combine_ids(ref(ctx.style.id), make_id(state)),
        //     rect);
        // if (cache.needs_rendering())
        // {
        //     skia_renderer renderer(ctx, cache.image(), rect.size);

        //     stateful_style_path_storage storage;
        //     style_search_path const* path = add_substyle_to_path(
        //         &storage, ctx.style.path, 0, "scrollbar", state);

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SkColorSetARGB(64, 0, 0, 0));

        render.canvas->drawRect(as_skrect(rect), paint);

        // SkScalar scrollbar_width =
        // layout_scalar_as_skia_scalar(metrics.width); SkScalar const r =
        // scrollbar_width / SkIntToScalar(2);
        // paint.setStrokeWidth(scrollbar_width - SkIntToScalar(2));
        // paint.setStrokeCap(SkPaint::kRound_Cap);
        // render.canvas->drawLine(
        //     r,
        //     r,
        //     layout_scalar_as_skia_scalar(rect.size[0]) - r,
        //     layout_scalar_as_skia_scalar(rect.size[1]) - r,
        //     paint);

        //     renderer.cache();
        //     cache.mark_valid();
        // }
        // cache.draw();
    }

    // button
    void
    draw_button(
        render_event&,
        scrollbar_metrics const&,
        layout_vector const&,
        unsigned,
        unsigned,
        element_state) const override
    {
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
set_logical_position(scrollbar_parameters const& sb, layout_scalar position)
{
    layout_scalar clamped
        = std::clamp(position, layout_scalar(0), get_max_logical_position(sb));
    write_signal(*sb.scroll_position, clamped);
    sb.data->physical_position = logical_position_to_physical(sb, clamped);
}

void
set_physical_position(scrollbar_parameters const& sb, layout_scalar position)
{
    layout_scalar clamped = std::clamp(
        position, layout_scalar(0), get_max_physical_position(sb));
    sb.data->physical_position = clamped;
    write_signal(
        *sb.scroll_position, physical_position_to_logical(sb, clamped));
}

void
process_button_input(
    event_context ctx,
    scrollbar_parameters const& sb,
    internal_element_ref element,
    layout_scalar increment)
{
    // TODO: static int const delay_after_first_increment = 400;
    // static int const delay_after_other_increment = 40;

    if (detect_mouse_press(ctx, element, mouse_button::LEFT))
    {
        set_logical_position(sb, read_signal(*sb.scroll_position) + increment);
        // TODO: start_timer(ctx, element, delay_after_first_increment);
    }
    // else if (
    //     is_click_in_progress(ctx, element, mouse_button::LEFT)
    //     && detect_timer_event(ctx, element))
    // {
    //     set_logical_position(sb, read_signal(*sb.scroll_position) +
    //     increment); restart_timer(ctx, element,
    //     delay_after_other_increment);
    // }
}

static default_scrollbar_renderer default_renderer;

bool
scrollbar_is_valid(scrollbar_parameters const& sb)
{
    return sb.content_size > 0 && sb.window_size > 0
           && sb.window_size < sb.content_size
           && get_max_physical_position(sb) >= 0;
}

void
refresh_scrollbar_data(dataless_context ctx, scrollbar_data& data)
{
    scrollbar_renderer const* renderer;
    // TODO: get_themed_renderer(ctx, data.rendering, &renderer,
    // &default_renderer);
    renderer = &default_renderer;

    data.metrics.refresh(/* TODO: *ctx.style.id */ unit_id, [&] {
        return renderer->get_metrics(ctx);
    });
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
                event, *data.metrics, sb.area, sb.axis, 0, WIDGET_NORMAL);
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
process_scrollbar_input(event_context ctx, scrollbar_parameters const& sb)
{
    scrollbar_data& data = *sb.data;

    process_button_input(ctx, sb, get_bg0_ref(sb), -sb.page_increment);
    process_button_input(ctx, sb, get_bg1_ref(sb), sb.page_increment);

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
        set_physical_position(sb, mouse + data.drag_start_delta);
    }

    process_button_input(ctx, sb, get_button0_ref(sb), -sb.line_increment);
    process_button_input(ctx, sb, get_button1_ref(sb), sb.line_increment);
}

void
refresh_scrollbar(dataless_context ctx, scrollbar_parameters const& sb)
{
    scrollbar_data& data = *sb.data;

    // If any of these is true, the scrollbar is nonsensical.
    if (!scrollbar_is_valid(sb))
        return;

    // If the thumb isn't being dragged, then the physical position should
    // stay in sync with the logical position.
    if (!is_drag_in_progress(ctx, get_thumb_ref(sb), mouse_button::LEFT)
        && signal_has_value(*sb.scroll_position))
    {
        data.physical_position = logical_position_to_physical(
            sb, read_signal(*sb.scroll_position));
    }
}

struct scrolling_data;

struct scrollable_layout_container : layout_container
{
    // implementation of layout interface
    layout_requirements
    get_horizontal_requirements(layout_calculation_context& ctx);
    layout_requirements
    get_vertical_requirements(
        layout_calculation_context& ctx, layout_scalar assigned_width);
    void
    set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment);

    // associated data
    scrolling_data* data_;

    // layout cacher
    layout_cacher cacher_;
};

// persistent data required for a scrollable region
struct scrolling_data
{
    // This is the actual, unsmoothed scroll position.
    // If the user supplies external storage, then this is a copy of the value
    // stored there. Otherwise, this is the actual value.
    // (Either way, it's OK to read it, but writing should go through the
    // set_scroll_position function.)
    layout_vector scroll_position;

    // If this is true, the scroll_position has changed internally and needs
    // to be communicated to the external storage.
    bool scroll_position_changed;

    // the smoothed version of the scroll position
    layout_vector smoothed_scroll_position;
    // for smoothing the scroll position
    value_smoother<layout_scalar> smoothers[2];

    // set by caller and copied here
    unsigned scrollable_axes, reserved_axes;

    // determined at usage site and needed by layout
    layout_scalar scrollbar_width, minimum_window_size, line_size;

    // determined by layout and stored here to communicate back to usage site
    bool hsb_on, vsb_on;
    layout_vector content_size, window_size;

    // data for scrollbars
    scrollbar_data hsb_data, vsb_data;

    // rendering data for junction
    // themed_rendering_data junction_rendering;

    // layout container
    scrollable_layout_container container;
};

layout_requirements
scrollable_layout_container::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    horizontal_layout_query query(ctx, cacher_, last_content_change);
    alia_if(query.update_required())
    {
        alia_if((data_->scrollable_axes & 1) != 0 && !ctx.for_measurement)
        {
            // If the window is horizontally scrollable, then we only need
            // enough space for scrolling to happen.
            query.update(calculated_layout_requirements(
                data_->minimum_window_size, 0, 0));
        }
        alia_else
        {
            // Otherwise, we need to calculate the requirements.
            assert(children && !children->next); // one and only one child
            layout_requirements r
                = alia::get_horizontal_requirements(ctx, *children);
            layout_scalar required_width = r.size;
            if ((data_->scrollable_axes & 2) != 0)
                required_width += data_->scrollbar_width;
            query.update(calculated_layout_requirements(required_width, 0, 0));
        }
        alia_end
    }
    alia_end
    return query.result();
}

layout_requirements
scrollable_layout_container::get_vertical_requirements(
    layout_calculation_context& ctx, layout_scalar assigned_width)
{
    vertical_layout_query query(
        ctx, cacher_, last_content_change, assigned_width);
    alia_if(query.update_required())
    {
        alia_if((data_->scrollable_axes & 2) != 0 && !ctx.for_measurement)
        {
            // If the window is vertically scrollable, then we only need
            // enough space for scrolling to happen.
            query.update(calculated_layout_requirements(
                data_->minimum_window_size, 0, 0));
        }
        alia_else
        {
            // Otherwise, we need to calculate the requirements.
            assert(children && !children->next); // one and only one child
            layout_scalar resolved_width = resolve_assigned_width(
                this->cacher_.resolved_spec,
                assigned_width,
                this->get_horizontal_requirements(ctx));
            layout_requirements x
                = alia::get_horizontal_requirements(ctx, *children);
            layout_scalar actual_width = (std::max)(resolved_width, x.size);
            layout_requirements y = alia::get_vertical_requirements(
                ctx, *children, actual_width);
            layout_scalar required_height = y.size;
            if ((data_->scrollable_axes & 1) != 0 && x.size > resolved_width)
                required_height += data_->scrollbar_width;
            query.update(
                calculated_layout_requirements(required_height, 0, 0));
        }
        alia_end
    }
    alia_end
    return query.result();
}

layout_scalar
clamp_scroll_position(
    scrolling_data& data, unsigned axis, layout_scalar position)
{
    return data.content_size[axis] > data.window_size[axis] ? clamp(
               position, 0, data.content_size[axis] - data.window_size[axis])
                                                            : 0;
}

void
reset_smoothing_for_axis(scrolling_data& data, unsigned axis)
{
    data.smoothed_scroll_position[axis] = data.scroll_position[axis];
    reset_smoothing(data.smoothers[axis], data.scroll_position[axis]);
}

void
set_scroll_position(
    scrolling_data& data, unsigned axis, layout_scalar position)
{
    data.scroll_position[axis] = position;
    data.scroll_position_changed = true;
}

void
set_scroll_position_abruptly(
    scrolling_data& data, unsigned axis, layout_scalar position)
{
    set_scroll_position(data, axis, position);
    reset_smoothing_for_axis(data, axis);
}

void
scrollable_layout_container::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    relative_region_assignment rra(
        ctx, *this, cacher_, last_content_change, assignment);
    alia_if(rra.update_required())
    {
        scrolling_data& data = *data_;

        layout_vector available_size = rra.resolved_assignment().region.size;

        assert(children && !children->next); // one and only one child
        layout_requirements x
            = alia::get_horizontal_requirements(ctx, *children);
        if (available_size[0] < x.size)
        {
            data.hsb_on = true;
            available_size[1] -= data.scrollbar_width;
        }
        else
            data.hsb_on = false;

        layout_requirements y = alia::get_vertical_requirements(
            ctx, *children, (std::max)(available_size[0], x.size));
        if (available_size[1] < y.size)
        {
            data.vsb_on = true;
            available_size[0] -= data.scrollbar_width;
            if (!data.hsb_on && available_size[0] < x.size)
            {
                data.hsb_on = true;
                available_size[1] -= data.scrollbar_width;
            }
        }
        else
            data.vsb_on = false;

        if ((data.reserved_axes & 1) != 0 && !data.hsb_on)
            available_size[1] -= data.scrollbar_width;
        if ((data.reserved_axes & 2) != 0 && !data.vsb_on)
            available_size[0] -= data.scrollbar_width;

        layout_scalar content_width = (std::max)(available_size[0], x.size);

        y = alia::get_vertical_requirements(ctx, *children, content_width);

        layout_scalar content_height = (std::max)(available_size[1], y.size);

        layout_vector content_size
            = make_layout_vector(content_width, content_height);

        // If the panel is scrolled all the way to the end, and the content
        // grows, scroll to show the new content.
        for (unsigned i = 0; i != 2; ++i)
        {
            layout_scalar sp = data.smoothed_scroll_position[i];
            if (sp != 0 && sp + data.window_size[i] >= data.content_size[i]
                && sp + available_size[i] < data.content_size[i])
            {
                set_scroll_position_abruptly(
                    data, i, content_size[i] - available_size[i]);
            }
        }

        data.content_size = content_size;
        data.window_size = available_size;

        // If the scroll position needs to be clamped because of changes in
        // content size, then do it abruptly, not smoothly.
        for (unsigned i = 0; i != 2; ++i)
        {
            layout_scalar original = data.smoothed_scroll_position[i];
            layout_scalar clamped = clamp_scroll_position(data, i, original);
            if (clamped != original)
                set_scroll_position_abruptly(data, i, clamped);
        }

        relative_layout_assignment assignment(
            layout_box(make_layout_vector(0, 0), content_size),
            content_height - y.descent);

        alia::set_relative_assignment(ctx, *children, assignment);
        rra.update();
    }
    alia_end
}

void
handle_visibility_request(
    dataless_context ctx,
    scrolling_data& data,
    make_widget_visible_event& event)
{
    matrix<3, 3, double> inverse_transform = inverse(get_transformation(ctx));
    // TODO: This doesn't handle rotations properly.
    vector<2, double> region_ul
        = transform(inverse_transform, event.region.corner),
        region_lr
        = transform(inverse_transform, get_high_corner(event.region)),
        window_ul = vector<2, double>(data.scroll_position),
        window_lr = window_ul + vector<2, double>(data.window_size);
    for (int i = 0; i != 2; ++i)
    {
        layout_scalar correction = 0;
        if (event.request.move_to_top)
        {
            correction = round_to_layout_scalar(region_ul[i] - window_ul[i]);
        }
        else if (event.region.size[i] <= double(data.window_size[i]))
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
            layout_scalar clamped = clamp_scroll_position(
                data, i, data.scroll_position[i] + correction);
            event.region.corner[i] += data.scroll_position[i] - clamped;
            set_scroll_position(data, i, clamped);
            if (event.request.abrupt)
                reset_smoothing_for_axis(data, i);
        }
    }
}

void
handle_scrolling_key_press(scrolling_data& data, key_event_info const& info)
{
    if (info.mods != 0)
        return;
    layout_vector new_position = data.scroll_position;
    switch (info.code)
    {
        case KEY_UP:
            new_position[1] -= data.line_size;
            break;
        case KEY_DOWN:
            new_position[1] += data.line_size;
            break;
        case KEY_PAGEUP:
            new_position[1] -= (std::max)(
                data.window_size[1] - data.line_size, data.line_size);
            break;
        case KEY_PAGEDOWN:
            new_position[1] += (std::max)(
                data.window_size[1] - data.line_size, data.line_size);
            break;
        case KEY_LEFT:
            new_position[0] -= data.line_size;
            break;
        case KEY_RIGHT:
            new_position[0] += data.line_size;
            break;
        case KEY_HOME:
            new_position[1] = 0;
            break;
        case KEY_END:
            new_position[1] = data.content_size[1] - data.window_size[1];
            break;
    }
    for (unsigned i = 0; i != 2; ++i)
    {
        if (new_position[i] != data.scroll_position[i])
        {
            set_scroll_position(
                data, i, clamp_scroll_position(data, i, new_position[i]));
        }
    }
}

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
    scrolling_data& data = *data_;

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
void
scrollable_region::end()
{
    if (ctx_)
    {
        dataless_context ctx = *ctx_;

        switch (ctx.event->category)
        {
            case REGION_CATEGORY:
                if (ctx.event->type == MAKE_WIDGET_VISIBLE_EVENT
                    && srr_.is_relevant())
                {
                    make_widget_visible_event& e
                        = get_event<make_widget_visible_event>(ctx);
                    if (e.acknowledged)
                        handle_visibility_request(ctx, *data_, e);
                }
                break;

            case INPUT_CATEGORY:
                if (srr_.is_relevant() || id_has_focus(ctx, id_))
                {
                    key_event_info info;
                    if (detect_key_press(ctx, &info))
                        handle_scrolling_key_press(*data_, info);
                }
                break;
        }

        transform_.end();
        scr_.end();

        srr_.end();
        slc_.end();

        ctx_ = 0;
    }
}

#endif

}} // namespace alia::indie
