#include <alia/layout.hpp>
#include <alia/layout_logic.hpp>
#include <alia/context.hpp>
#include <alia/surface.hpp>
#include <alia/location.hpp>

namespace alia {

bool do_layout(context& ctx)
{
    {
    ++ctx.refresh_counter;
    refresh_event e;
    e.layout_needed = false;
    issue_event(ctx, e);
    if (!e.layout_needed)
        return false;
    }
    {
    layout_pass0_event e;
    issue_event(ctx, e);
    }
    {
    layout_pass1_event e;
    issue_event(ctx, e);
    }
    {
    layout_pass2_event e;
    issue_event(ctx, e);
    }
    return true;
}

bool operator==(size const& a, size const& b)
{
    return a.width == b.width && a.height == b.height &&
        a.x_units == b.x_units && a.y_units == b.y_units;
}
bool operator!=(size const& a, size const& b)
{
    return !(a == b);
}

bool operator==(layout const& a, layout const& b)
{
    return a.size == b.size && a.flags == b.flags &&
        a.proportion == b.proportion;
}
bool operator!=(layout const& a, layout const& b)
{
    return !(a == b);
}

bool operator==(resolved_layout_spec const& a, resolved_layout_spec const& b)
{
    return a.size == b.size && a.ascent == b.ascent &&
        a.descent == b.descent && a.alignment == b.alignment &&
        a.proportion == b.proportion && a.padding_size == b.padding_size;
}
bool operator!=(resolved_layout_spec const& a, resolved_layout_spec const& b)
{
    return !(a == b);
}

layout default_layout;

void record_layout_change(context& ctx, layout_data& data)
{
    refresh_event& e = get_event<refresh_event>(ctx);
    e.layout_needed = true;
    layout_data* d = &data;
    while (!d->dirty)
    {
        d->dirty = true;
        d = d->parent;
        if (!d)
            break;
    }
}

void diff_widget_layout(context& ctx, layout_data& data,
    resolved_layout_spec const& resolved)
{
    refresh_event& e = get_event<refresh_event>(ctx);
    assert(e.active_logic && e.active_data && e.next_ptr);
    if (&data == e.active_data)
        assert(&data != e.active_data);
    if (data.parent != e.active_data ||
        &data != *e.next_ptr ||
        data.spec != resolved)
    {
        data.parent = e.active_data;
        *e.next_ptr = &data;
        record_layout_change(ctx, data);
        return;
    }
    e.next_ptr = &data.next;
}

void diff_widget_location(context& ctx, layout_data& data)
{
    refresh_event& e = get_event<refresh_event>(ctx);
    assert(e.active_logic && e.active_data && e.next_ptr);
    if (&data == e.active_data)
        assert(&data != e.active_data);
    if (data.parent != e.active_data ||
        &data != *e.next_ptr)
    {
        data.parent = e.active_data;
        *e.next_ptr = &data;
        record_layout_change(ctx, data);
        return;
    }
    e.next_ptr = &data.next;
}

void diff_layout_spec(context& ctx, layout_data& data,
    resolved_layout_spec const& resolved)
{
    refresh_event& e = get_event<refresh_event>(ctx);
    assert(e.active_logic && e.active_data && e.next_ptr);
    if (data.spec != resolved)
    {
        // TODO: store spec here instead of in record_layout
        record_layout_change(ctx, data);
        return;
    }
}

vector2i resolve_size(context& ctx, size const& s)
{
    vector2i r;
    switch (s.x_units)
    {
     case PIXELS:
        r[0] = int(s.width + 0.5);
        break;
     case INCHES:
        r[0] = int(s.width * ctx.surface->get_ppi()[0] + 0.5);
        break;
     case CM:
        r[0] = int(s.width * ctx.surface->get_ppi()[0] / 2.54 + 0.5);
        break;
     case MM:
        r[0] = int(s.width * ctx.surface->get_ppi()[0] / 25.4 + 0.5);
        break;
     case CHARS:
        r[0] = int(s.width * get_font_metrics(ctx,
            ctx.pass_state.active_font).average_width + 0.5);
        break;
     case EM:
        r[0] = int(s.width * get_font_metrics(ctx,
            ctx.pass_state.active_font).height + 0.5);
        break;
    }
    switch (s.y_units)
    {
     case PIXELS:
        r[1] = int(s.height + 0.5);
        break;
     case INCHES:
        r[1] = int(s.height * ctx.surface->get_ppi()[1] + 0.5);
        break;
     case CM:
        r[1] = int(s.height * ctx.surface->get_ppi()[1] / 2.54 + 0.5);
        break;
     case MM:
        r[1] = int(s.height * ctx.surface->get_ppi()[1] / 25.4 + 0.5);
        break;
     case CHARS:
     case EM:
        r[1] = int(s.height * get_font_metrics(ctx,
            ctx.pass_state.active_font).height + 0.5);
        break;
    }
    return r;
}

void resolve_layout_spec(context& ctx, resolved_layout_spec* resolved,
    layout const& spec, vector2i const& requested_size,
    widget_layout_info const& info)
{
    assert(info.minimum_size[1] >= info.minimum_ascent + info.minimum_descent);
    resolved->padding_size =
        ((spec.flags & PADDED) ||
         !(spec.flags & NOT_PADDED) && info.padded_by_default) ?
        ctx.pass_state.padding_size : vector2i(0, 0);
    for (int i = 0; i < 2; ++i)
    {
        resolved->size[i] = (std::max)(
            requested_size[i] > 0 ? requested_size[i] : info.default_size[i],
            info.minimum_size[i]);
    }
    resolved->proportion = spec.proportion;
    resolved->alignment.code = 0;
    {
        unsigned x_alignment =
            (spec.flags.code & X_ALIGNMENT_MASK_CODE) != 0 ?
            (spec.flags.code & X_ALIGNMENT_MASK_CODE) :
            (info.default_alignment.code & X_ALIGNMENT_MASK_CODE);
        resolved->alignment.code |= x_alignment;
    }
    {
        unsigned y_alignment =
            (spec.flags.code & Y_ALIGNMENT_MASK_CODE) != 0 ?
            (spec.flags.code & Y_ALIGNMENT_MASK_CODE) :
            (info.default_alignment.code & Y_ALIGNMENT_MASK_CODE);
        resolved->alignment.code |= y_alignment;
        if (y_alignment == BASELINE_Y_CODE)
        {
            resolved->ascent = info.minimum_ascent;
            resolved->descent = info.minimum_descent;
        }
        else
        {
            resolved->ascent = 0;
            resolved->descent = 0;
        }
    }
    if ((resolved->alignment.code & 0x88) != 0)
    {
        if (resolved->proportion == 0)
            resolved->proportion = 1;
        resolved->alignment.code &= ~0x88;
    }
}

void record_layout(context& ctx, layout_data& data,
    resolved_layout_spec const& resolved)
{
    layout_pass2_event& e = get_event<layout_pass2_event>(ctx);
    assert(e.next_ptr);
    *e.next_ptr = &data;
    e.next_ptr = &data.next;
    assert(e.active_logic);
    if (&data == e.active_data)
        assert(&data != e.active_data);
    data.parent = e.active_data;
    data.spec = resolved;
    data.dirty = false;
}

void request_horizontal_space(context& ctx,
    resolved_layout_spec const& resolved)
{
    layout_pass0_event& e = get_event<layout_pass0_event>(ctx);
    e.active_logic->request_horizontal_space_for_node(
        resolved.size[0] + resolved.padding_size[0] * 2,
        resolved.proportion);
}

int get_assigned_width(context& ctx, resolved_layout_spec const& resolved)
{
    layout_pass1_event& e = get_event<layout_pass1_event>(ctx);
    int assigned_width =
        e.active_logic->get_width_for_node(
            resolved.size[0] + resolved.padding_size[0] * 2,
            resolved.proportion);
    if ((resolved.alignment & X_ALIGNMENT_MASK) == FILL_X)
        return assigned_width - resolved.padding_size[0] * 2;
    else
        return resolved.size[0];
}

void request_vertical_space(context& ctx, resolved_layout_spec const& resolved)
{    
    layout_pass1_event& e = get_event<layout_pass1_event>(ctx);
    e.active_logic->request_vertical_space_for_node(
        resolved.size[1] + resolved.padding_size[1] * 2,
        resolved.ascent + resolved.padding_size[1],
        resolved.descent + resolved.padding_size[1],
        resolved.proportion);
}

void get_assigned_region(context& ctx, box2i* region,
    resolved_layout_spec const& resolved)
{
    layout_pass2_event& e = get_event<layout_pass2_event>(ctx);

    int baseline_y;
    e.active_logic->get_region_for_node(region, &baseline_y,
        resolved.size + resolved.padding_size * 2,
        resolved.proportion);

    region->corner += resolved.padding_size;
    region->size -= resolved.padding_size * 2;
    baseline_y -= resolved.padding_size[1];

    if ((resolved.alignment & ALIGNMENT_MASK) == PROPORTIONAL_FILL)
    {
        double x_ratio = double(region->size[0]) / resolved.size[0];
        double y_ratio = double(region->size[1]) / resolved.size[1];
        if (x_ratio > y_ratio)
        {
            int x_size = int(resolved.size[0] * y_ratio);
            region->corner[0] += (region->size[0] - x_size) / 2;
            region->size[0] = x_size;
        }
        else
        {
            int y_size = int(resolved.size[1] * x_ratio);
            region->corner[1] += (region->size[1] - y_size) / 2;
            region->size[1] = y_size;
        }
    }
    else
    {
        switch (resolved.alignment.code & X_ALIGNMENT_MASK_CODE)
        {
         case LEFT_CODE:
            region->size[0] = resolved.size[0];
            break;
         case RIGHT_CODE:
            region->corner[0] += region->size[0] - resolved.size[0];
            region->size[0] = resolved.size[0];
            break;
         case CENTER_X_CODE:
            region->corner[0] += (region->size[0] - resolved.size[0]) / 2;
            region->size[0] = resolved.size[0];
            break;
        }
        switch (resolved.alignment.code & Y_ALIGNMENT_MASK_CODE)
        {
         case TOP_CODE:
            region->size[1] = resolved.size[1];
            break;
         case BOTTOM_CODE:
            region->corner[1] += region->size[1] - resolved.size[1];
            region->size[1] = resolved.size[1];
            break;
         case BASELINE_Y_CODE:
            // Some layout objects ignore the baseline and always set
            // baseline_y to 0. In those cases, this case falls through and
            // the widget is centered.
            if (baseline_y >= resolved.ascent)
            {
                region->corner[1] +=
                     baseline_y + resolved.descent - resolved.size[1];
                region->size[1] = resolved.size[1];
                break;
            }
         case CENTER_Y_CODE:
            region->corner[1] += (region->size[1] - resolved.size[1]) / 2;
            region->size[1] = resolved.size[1];
            break;
        }
    }
}

void process_jump_event(context& ctx, box2i const& region,
    resolved_layout_spec const& resolved)
{
    jump_event& e = get_event<jump_event>(ctx);
    if (e.state == jump_event::WAITING)
    {
        e.point = region.corner - resolved.padding_size;
        e.state = jump_event::JUMPING;
    }
}

void layout_widget(context& ctx, layout_data& data,
    layout const& spec, vector2i const& requested_size,
    widget_layout_info const& info)
{
    // TODO: merge with rest
    //if (ctx.event->type == JUMP_TO_LOCATION)
    //    process_jump_event(ctx, data.assigned_region, resolved);

    if (!get_event<layout_event>(ctx).active_logic)
        return;
    resolved_layout_spec resolved;
    resolve_layout_spec(ctx, &resolved, spec, requested_size, info);
    switch (ctx.event->type)
    {
     case REFRESH_EVENT:
        diff_widget_layout(ctx, data, resolved);
        break;
     case LAYOUT_PASS_0:
        request_horizontal_space(ctx, resolved);
        break;
     case LAYOUT_PASS_1:
        data.assigned_region.size[0] = get_assigned_width(ctx, resolved);
        request_vertical_space(ctx, resolved);
        break;
     case LAYOUT_PASS_2:
        get_assigned_region(ctx, &data.assigned_region, resolved);
        record_layout(ctx, data, resolved);
        break;
    }
}

}
