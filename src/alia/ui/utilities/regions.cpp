#include <alia/ui/utilities/regions.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

bool is_mouse_inside_box(dataless_ui_context& ctx, box<2,double> const& box)
{
    return
        ctx.system->input.mouse_inside_window &&
        is_inside(box, get_mouse_position(ctx)) &&
        is_inside(get_geometry_context(ctx).clip_region,
            vector<2,double>(ctx.system->input.mouse_position));
}

void
handle_mouse_hit(
    dataless_ui_context& ctx,
    widget_id id,
    box<2,double> const& bounding_box,
    hit_test_flag_set flags,
    mouse_cursor cursor)
{
    if (ctx.event->type == MOUSE_HIT_TEST_EVENT && (flags & HIT_TEST_MOUSE))
    {
        mouse_hit_test_event& e = get_event<mouse_hit_test_event>(ctx);
        e.id = make_routable_widget_id(ctx, id);
        e.cursor = cursor;
        e.hit_box = layout_box(transform_box(get_transformation(ctx), bounding_box));
    }
    else if (ctx.event->type == WHEEL_HIT_TEST_EVENT &&
        (flags & HIT_TEST_WHEEL))
    {
        wheel_hit_test_event& e = get_event<wheel_hit_test_event>(ctx);
        e.id = make_routable_widget_id(ctx, id);
    }
}

void hit_test_box_region(dataless_ui_context& ctx, widget_id id,
    box<2,int> const& box, hit_test_flag_set flags, mouse_cursor cursor)
{
    if (is_mouse_inside_box(ctx, alia::box<2,double>(box)))
        handle_mouse_hit(ctx, id, alia::box<2,double>(box), flags, cursor);
}

void hit_test_box_region(dataless_ui_context& ctx, widget_id id,
    box<2,double> const& box, hit_test_flag_set flags, mouse_cursor cursor)
{
    if (is_mouse_inside_box(ctx, box))
        handle_mouse_hit(ctx, id, box, flags, cursor);
}

void handle_region_visibility(dataless_ui_context& ctx, widget_id id,
    box<2,int> const& region)
{
    make_widget_visible_event& e = get_event<make_widget_visible_event>(ctx);
    if (e.request.widget.id == id)
    {
        // TODO: This doesn't handle rotations properly.
        e.region = box<2,double>(
            transform(get_transformation(ctx),
                vector<2,double>(region.corner - get_padding_size(ctx))),
            vector<2,double>(region.size + get_padding_size(ctx) * 2));
        e.acknowledged = true;
    }
}

void handle_region_visibility(dataless_ui_context& ctx, widget_id id,
    box<2, double> const& region)
{
    make_widget_visible_event& e = get_event<make_widget_visible_event>(ctx);
    if (e.request.widget.id == id)
    {
        // TODO: This doesn't handle rotations properly.
        e.region =
            box<2, double>(
                transform(get_transformation(ctx),
                    vector<2, double>(region.corner - (vector<2, double>)get_padding_size(ctx))),
                vector<2, double>(region.size + (vector<2, double>)get_padding_size(ctx) * 2));
        e.acknowledged = true;
    }
}

void do_box_region(
    dataless_ui_context& ctx, widget_id id, box<2,int> const& region,
    mouse_cursor cursor)
{
    switch (ctx.event->type)
    {
     case MOUSE_HIT_TEST_EVENT:
        hit_test_box_region(ctx, id, region, HIT_TEST_MOUSE, cursor);
        break;
     case MAKE_WIDGET_VISIBLE_EVENT:
        handle_region_visibility(ctx, id, region);
        break;
    }
}

void do_box_region(
    dataless_ui_context& ctx, widget_id id, box<2,double> const& region,
    mouse_cursor cursor)
{
    switch (ctx.event->type)
    {
    case MOUSE_HIT_TEST_EVENT:
        hit_test_box_region(ctx, id, region, HIT_TEST_MOUSE, cursor);
        break;
    case MAKE_WIDGET_VISIBLE_EVENT:
        handle_region_visibility(ctx, id, region);
        break;
    }
}

void override_mouse_cursor(
    dataless_ui_context& ctx, widget_id id, mouse_cursor cursor)
{
    if (ctx.event->type == MOUSE_HIT_TEST_EVENT)
    {
        mouse_hit_test_event& e = get_event<mouse_hit_test_event>(ctx);
        if (e.id.id == id)
            e.cursor = cursor;
    }
    else if (ctx.event->type == MOUSE_CURSOR_QUERY_EVENT)
    {
        mouse_cursor_query& e = get_event<mouse_cursor_query>(ctx);
        if (e.id == id)
            e.cursor = cursor;
    }
}

bool is_region_active(dataless_ui_context& ctx, widget_id id)
{
    return ctx.system->input.active_id.id == id;
}

void set_hot_region(ui_system& ui, routable_widget_id const& hot_id)
{
    // If there's no active widget and the mouse is moving to a different widget, set the
    // hover_start_time.
    if (!is_valid(ui.input.active_id) && ui.input.hot_id.id != hot_id.id)
    {
        ui.input.hover_start_time = ui.millisecond_tick_count;
    }

    ui.input.hot_id = hot_id;
}

bool is_region_hot(dataless_ui_context& ctx, widget_id id)
{
    return ctx.system->input.hot_id.id == id;
}

void set_active_region(ui_system& ui, routable_widget_id const& active_id)
{
    // If there was an active widget before, but we're removing it, this means that the
    // mouse is starting to hover over whatever it's over.
    if (is_valid(ui.input.active_id) && !is_valid(active_id))
    {
        ui.input.hover_start_time = ui.millisecond_tick_count;
    }

    ui.input.active_id = active_id;
}

void make_widget_visible(dataless_ui_context& ctx, widget_id id,
    make_widget_visible_flag_set flags)
{
    widget_visibility_request request;
    request.widget = make_routable_widget_id(ctx, id);
    request.abrupt = (flags & MAKE_WIDGET_VISIBLE_ABRUPTLY) ? true : false;
    request.move_to_top = false;
    ctx.system->pending_visibility_requests.push_back(request);
}

box<2,double>
region_to_surface_coordinates(dataless_ui_context& ctx,
    box<2,double> const& region)
{
    vector<2,double> corner0 =
        transform(get_geometry_context(ctx).transformation_matrix,
            region.corner);
    vector<2,double> corner1 =
        transform(get_geometry_context(ctx).transformation_matrix,
            get_high_corner(region));
    box<2,double> region_in_root_frame;
    for (unsigned i = 0; i != 2; ++i)
    {
        region_in_root_frame.corner[i] =
            corner0[i] < corner1[i] ? corner0[i] : corner1[i];
        region_in_root_frame.size[i] =
            std::fabs(corner1[i] - corner0[i]);
    }
    return region_in_root_frame;
}

void
set_tooltip_message(
    ui_context& ctx,
    widget_id region_id,
    accessor<string> const& tooltip_message)
{
    if (ctx.event->type == MOUSE_HIT_TEST_EVENT && is_gettable(tooltip_message))
    {
        mouse_hit_test_event& e = get_event<mouse_hit_test_event>(ctx);
        if (e.id.id == region_id)
            e.tooltip_message = get(tooltip_message);
    }
}

}
