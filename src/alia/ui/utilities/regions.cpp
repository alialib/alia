#include <alia/ui/utilities/regions.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

void handle_mouse_hit(ui_context& ctx, widget_id id, hit_test_flag_set flags,
    mouse_cursor cursor)
{
    if (ctx.event->type == MOUSE_HIT_TEST_EVENT && (flags & HIT_TEST_MOUSE))
    {
        mouse_hit_test_event& e = get_event<mouse_hit_test_event>(ctx);
	e.id = make_routable_widget_id(ctx, id);
	e.cursor = cursor;
    }
    else if (ctx.event->type == WHEEL_HIT_TEST_EVENT &&
        (flags & HIT_TEST_WHEEL))
    {
        wheel_hit_test_event& e = get_event<wheel_hit_test_event>(ctx);
	e.id = make_routable_widget_id(ctx, id);
    }
}

void hit_test_box_region(ui_context& ctx, widget_id id,
    box<2,int> const& box, hit_test_flag_set flags, mouse_cursor cursor)
{
    if (mouse_is_inside_box(ctx, alia::box<2,double>(box)))
	handle_mouse_hit(ctx, id, flags, cursor);
}

void do_region_visibility(ui_context& ctx, widget_id id,
    box<2,int> const& region)
{
    make_widget_visible_event& e = get_event<make_widget_visible_event>(ctx);
    if (e.id == id)
    {
        // TODO: This doesn't handle rotations properly.
        e.region = box<2,double>(
            transform(get_transformation(ctx),
                vector<2,double>(region.corner - get_padding_size(ctx))),
            vector<2,double>(region.size + get_padding_size(ctx) * 2));
        e.acknowledged = true;
    }
}

void do_box_region(ui_context& ctx, widget_id id, box<2,int> const& region,
    mouse_cursor cursor)
{
    switch (ctx.event->type)
    {
     case MOUSE_HIT_TEST_EVENT:
        hit_test_box_region(ctx, id, region, HIT_TEST_MOUSE, cursor);
        break;
     case MAKE_WIDGET_VISIBLE_EVENT:
        do_region_visibility(ctx, id, region);
        break;
    }
}

void override_mouse_cursor(ui_context& ctx, widget_id id, mouse_cursor cursor)
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

bool is_region_active(ui_context& ctx, widget_id id)
{
    return ctx.system->input.active_id.id == id;
}

bool is_region_hot(ui_context& ctx, widget_id id)
{
    return ctx.system->input.hot_id.id == id;
}

}
