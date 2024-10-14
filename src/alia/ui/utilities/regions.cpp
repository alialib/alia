#include <alia/ui/utilities/regions.hpp>

#include <alia/ui/events.hpp>
#include <alia/ui/system/object.hpp>
#include <alia/ui/utilities/mouse.hpp>

namespace alia {

bool
is_mouse_inside_box(dataless_ui_context ctx, box<2, double> const& box)
{
    return get_system(ctx).input.mouse_inside_window
        && is_inside(box, get_mouse_position(ctx))
        && is_inside(
               get_geometry_context(ctx).clip_region,
               vector<2, double>(get_system(ctx).input.mouse_position));
}

void
handle_mouse_hit(
    dataless_ui_context ctx,
    widget_id id,
    box<2, double> const& bounding_box,
    hit_test_flag_set flags,
    mouse_cursor cursor)
{
    if (get_event_type(ctx) == MOUSE_HIT_TEST_EVENT
        && (flags & HIT_TEST_MOUSE))
    {
        auto& e = cast_event<mouse_hit_test_event>(ctx);
        e.result = mouse_hit_test_result{
            make_routable_widget_id(ctx, id),
            cursor,
            layout_box(transform_box(get_transformation(ctx), bounding_box)),
            ""};
    }
    else if (
        get_event_type(ctx) == WHEEL_HIT_TEST_EVENT
        && (flags & HIT_TEST_WHEEL))
    {
        auto& e = cast_event<wheel_hit_test_event>(ctx);
        e.result = make_routable_widget_id(ctx, id);
    }
}

void
hit_test_box_region(
    dataless_ui_context ctx,
    widget_id id,
    box<2, float> const& box,
    hit_test_flag_set flags,
    mouse_cursor cursor)
{
    if (is_mouse_inside_box(ctx, alia::box<2, double>(box)))
        handle_mouse_hit(ctx, id, alia::box<2, double>(box), flags, cursor);
}

void
hit_test_box_region(
    dataless_ui_context ctx,
    widget_id id,
    box<2, double> const& box,
    hit_test_flag_set flags,
    mouse_cursor cursor)
{
    if (is_mouse_inside_box(ctx, box))
        handle_mouse_hit(ctx, id, box, flags, cursor);
}

void
handle_region_visibility(
    dataless_ui_context, // ctx,
    widget_id, // id,
    box<2, double> const&) // region)
{
    // make_widget_visible_event& e =
    // get_event<make_widget_visible_event>(ctx); if (e.request.widget.id ==
    // id)
    // {
    //     // TODO: This doesn't handle rotations properly.
    //     e.region = box<2, double>(
    //         transform(
    //             get_transformation(ctx),
    //             vector<2, double>(
    //                 region.corner
    //                 - (vector<2, double>) get_padding_size(ctx))),
    //         vector<2, double>(
    //             region.size + (vector<2, double>) get_padding_size(ctx) *
    //             2));
    //     e.acknowledged = true;
    // }
}

void
do_box_region(
    dataless_ui_context ctx,
    widget_id id,
    box<2, float> const& region,
    mouse_cursor cursor)
{
    switch (get_event_type(ctx))
    {
        case MOUSE_HIT_TEST_EVENT:
            hit_test_box_region(ctx, id, region, HIT_TEST_MOUSE, cursor);
            break;
        case MAKE_WIDGET_VISIBLE_EVENT:
            // handle_region_visibility(ctx, id, region);
            break;
    }
}

void
do_box_region(
    dataless_ui_context ctx,
    widget_id id,
    box<2, double> const& region,
    mouse_cursor cursor)
{
    switch (get_event_type(ctx))
    {
        case MOUSE_HIT_TEST_EVENT:
            hit_test_box_region(ctx, id, region, HIT_TEST_MOUSE, cursor);
            break;
        case MAKE_WIDGET_VISIBLE_EVENT:
            // handle_region_visibility(ctx, id, region);
            break;
    }
}

void
override_mouse_cursor(
    dataless_ui_context ctx, widget_id id, mouse_cursor cursor)
{
    if (get_event_type(ctx) == MOUSE_HIT_TEST_EVENT)
    {
        auto& e = cast_event<mouse_hit_test_event>(ctx);
        if (e.result && e.result->id.matches(id))
            e.result->cursor = cursor;
    }
    else if (get_event_type(ctx) == MOUSE_CURSOR_QUERY_EVENT)
    {
        auto& e = cast_event<mouse_cursor_query>(ctx);
        if (e.target == id)
            e.cursor = cursor;
    }
}

void
make_widget_visible(
    dataless_ui_context ctx,
    widget_id id,
    widget_visibility_request_flag_set flags)
{
    widget_visibility_request request;
    request.widget = make_routable_widget_id(ctx, id);
    request.flags = flags;
    get_system(ctx).pending_visibility_requests.push_back(request);
}

box<2, double>
region_to_surface_coordinates(
    dataless_ui_context ctx, box<2, double> const& region)
{
    vector<2, double> corner0 = transform(
        get_geometry_context(ctx).transformation_matrix, region.corner);
    vector<2, double> corner1 = transform(
        get_geometry_context(ctx).transformation_matrix,
        get_high_corner(region));
    box<2, double> region_in_root_frame;
    for (unsigned i = 0; i != 2; ++i)
    {
        region_in_root_frame.corner[i]
            = corner0[i] < corner1[i] ? corner0[i] : corner1[i];
        region_in_root_frame.size[i] = std::fabs(corner1[i] - corner0[i]);
    }
    return region_in_root_frame;
}

void
set_tooltip_message(
    ui_context& ctx,
    widget_id region_id,
    readable<std::string> const& tooltip_message)
{
    if (get_event_type(ctx) == MOUSE_HIT_TEST_EVENT
        && signal_has_value(tooltip_message))
    {
        auto& e = cast_event<mouse_hit_test_event>(ctx);
        if (e.result && e.result->id.matches(region_id))
            e.result->tooltip_message = read_signal(tooltip_message);
    }
}

} // namespace alia