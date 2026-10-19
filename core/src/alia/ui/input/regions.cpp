#include <alia/abi/ui/input/regions.h>

#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/geometry.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/styling.h>
#include <alia/impl/events.hpp>
#include <alia/ui/system/object.h>

using namespace alia::operators;
using namespace alia;

namespace {

bool
passes_z_test(alia_hit_target const& current, alia_z_index candidate_z)
{
    return !alia_element_id_is_valid(current.id) || candidate_z >= current.z;
}

void
record_hit(alia_hit_target& target, alia_element_id id, alia_z_index z)
{
    target.id = id;
    target.z = z;
}

void
hit_test_rect_region(
    alia_ui_context* ctx,
    alia_element_id id,
    alia_box const* box,
    alia_z_index z_plane,
    alia_hit_test_flags_t flags,
    alia_cursor_t cursor)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(box);
    if (alia_input_pointer_in_box(ctx, box))
        alia_element_report_hit(ctx, id, box, z_plane, flags, cursor);
}

} // namespace

extern "C" {

void
alia_element_box_region(
    alia_ui_context* ctx,
    alia_element_id id,
    alia_box const* region,
    alia_z_index z_plane,
    alia_hit_test_flags_t flags,
    alia_cursor_t cursor)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(region);
    switch (get_event_type(*ctx))
    {
        case ALIA_EVENT_MOUSE_HIT_TEST:
            if (flags & ALIA_HIT_TEST_MOUSE)
            {
                hit_test_rect_region(
                    ctx, id, region, z_plane, ALIA_HIT_TEST_MOUSE, cursor);
            }
            break;
        case ALIA_EVENT_SCROLL_INPUT_HIT_TEST:
            if (flags & ALIA_HIT_TEST_SCROLL_INPUT)
            {
                hit_test_rect_region(
                    ctx,
                    id,
                    region,
                    z_plane,
                    ALIA_HIT_TEST_SCROLL_INPUT,
                    cursor);
            }
            break;
        case ALIA_EVENT_TOUCH_GESTURE_HIT_TEST:
            hit_test_rect_region(ctx, id, region, z_plane, flags, cursor);
            break;
        case ALIA_EVENT_MAKE_WIDGET_VISIBLE:
            alia_element_handle_visibility(ctx, id, region);
            break;
    }
}

void
alia_element_report_hit(
    alia_ui_context* ctx,
    alia_element_id id,
    alia_box const* bounding_box,
    alia_z_index z_plane,
    alia_hit_test_flags_t flags,
    alia_cursor_t cursor)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(bounding_box);
    alia_z_index const z = alia_z(ctx, z_plane);
    if (get_event_type(*ctx) == ALIA_EVENT_MOUSE_HIT_TEST
        && (flags & ALIA_HIT_TEST_MOUSE))
    {
        auto& e = as_mouse_hit_test_event(*ctx);
        if (passes_z_test(e.result.target, z))
        {
            record_hit(e.result.target, id, z);
            e.result.cursor = cursor;
            e.result.region
                = alia_box_translate(*bounding_box, ctx->geometry->offset);
        }
    }
    else if (
        get_event_type(*ctx) == ALIA_EVENT_SCROLL_INPUT_HIT_TEST
        && (flags & ALIA_HIT_TEST_SCROLL_INPUT))
    {
        auto& e = as_scroll_input_hit_test_event(*ctx);
        if (passes_z_test(e.result.target, z))
            record_hit(e.result.target, id, z);
    }
    else if (get_event_type(*ctx) == ALIA_EVENT_TOUCH_GESTURE_HIT_TEST)
    {
        auto& e = as_touch_gesture_hit_test_event(*ctx);
        if ((flags & ALIA_HIT_TEST_MOUSE)
            && passes_z_test(e.result.pointer, z))
        {
            record_hit(e.result.pointer, id, z);
        }
        if ((flags & ALIA_HIT_TEST_SCROLL_INPUT)
            && passes_z_test(e.result.scroll, z))
        {
            record_hit(e.result.scroll, id, z);
        }
        if ((flags & ALIA_HIT_TEST_TOUCH_DRAG)
            && passes_z_test(e.result.touch_drag, z))
        {
            record_hit(e.result.touch_drag, id, z);
        }
    }
}

void
alia_element_handle_visibility(
    alia_ui_context* ctx, alia_element_id id, alia_box const* region)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(region);
    if (get_event_target(*ctx) == id)
    {
        auto& e = as_make_widget_visible_event(*ctx);
        e.region = alia_box_translate(
            // TODO: Do we really want to expand here?
            alia_box_expand(
                *region,
                alia_edge_offsets_make_uniform(
                    alia_px(ctx, alia_layout_style_active(ctx)->spacing))),
            ctx->geometry->offset);
        e.acknowledged = true;
    }
}

void
alia_element_override_cursor(
    alia_ui_context* ctx, alia_element_id id, alia_cursor_t cursor)
{
    ALIA_ASSERT(ctx);
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
