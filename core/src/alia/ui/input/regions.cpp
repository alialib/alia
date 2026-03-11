#include <alia/abi/ui/input/regions.h>

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/style.h>
#include <alia/impl/events.hpp>
#include <alia/ui/system/object.h>

using namespace alia::operators;
using namespace alia;

extern "C" {

void
alia_element_box_region(
    alia_context* ctx,
    alia_element_id id,
    alia_box const* region,
    alia_cursor_t cursor)
{
    // TODO: Shouldn't this allow handling wheel hit tests too?
    switch (get_event_type(*ctx))
    {
        case ALIA_EVENT_MOUSE_HIT_TEST:
            alia_element_hit_test_box_region(
                ctx, id, region, ALIA_HIT_TEST_MOUSE, cursor);
            break;
        case ALIA_EVENT_MAKE_WIDGET_VISIBLE:
            alia_element_handle_visibility(ctx, id, region);
            break;
    }
}

void
alia_element_hit_test_box_region(
    alia_context* ctx,
    alia_element_id id,
    alia_box const* box,
    alia_hit_test_flags_t flags,
    alia_cursor_t cursor)
{
    if (alia_input_pointer_in_box(ctx, box))
        alia_element_report_mouse_hit(ctx, id, box, flags, cursor);
}

void
alia_element_report_mouse_hit(
    alia_context* ctx,
    alia_element_id id,
    alia_box const* bounding_box,
    alia_hit_test_flags_t flags,
    alia_cursor_t cursor)
{
    if (get_event_type(*ctx) == ALIA_EVENT_MOUSE_HIT_TEST
        && (flags & ALIA_HIT_TEST_MOUSE))
    {
        auto& e = as_mouse_hit_test_event(*ctx);
        e.result = {make_routable_element_id(*ctx, id), cursor};
        // TODO: Do we need the region?
        // transform_aabb(get_transformation(ctx), bounding_box)
    }
    else if (
        get_event_type(*ctx) == ALIA_EVENT_WHEEL_HIT_TEST
        && (flags & ALIA_HIT_TEST_WHEEL))
    {
        auto& e = as_wheel_hit_test_event(*ctx);
        e.result = make_routable_element_id(*ctx, id);
    }
}

void
alia_element_handle_visibility(
    alia_context* ctx, alia_element_id id, alia_box const* region)
{
    if (get_event_target(*ctx) == id)
    {
        auto& e = as_make_widget_visible_event(*ctx);
        e.region = alia_box_translate(
            // TODO: Do we really want to expand here?
            alia_box_expand(
                *region,
                {alia_ctx_style(ctx)->padding, alia_ctx_style(ctx)->padding}),
            ctx->geometry->offset);
        e.acknowledged = true;
    }
}

void
alia_element_override_cursor(
    alia_context* ctx, alia_element_id id, alia_cursor_t cursor)
{
    if (get_event_type(*ctx) == ALIA_EVENT_MOUSE_HIT_TEST)
    {
        auto& e = as_mouse_hit_test_event(*ctx);
        if (get_event_target(*ctx) == id)
            e.result.cursor = cursor;
    }
    else if (get_event_type(*ctx) == ALIA_EVENT_CURSOR_QUERY)
    {
        auto& e = as_cursor_query_event(*ctx);
        if (get_event_target(*ctx) == id)
            e.cursor = cursor;
    }
}

#if 0

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

#endif

} // extern "C"
