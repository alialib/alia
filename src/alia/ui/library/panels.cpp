#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

struct panel_style_info
{
    resolved_box_border_width border_width;
    bool is_rounded;
    rgba8 border_color, background_color;
    side_selection hidden_border_sides;
};

struct panel_data
{
    caching_renderer_data rendering;
    keyed_data<panel_style_info> style;
};

box_border_width
get_border_width(
    style_search_path const* path,
    absolute_length const& default_width = absolute_length(0, PIXELS))
{
    box_border_width border_width =
        get_property(path, "border-width", UNINHERITED_PROPERTY,
            box_border_width(default_width));
    return box_border_width(
        get_property(path, "border-top-width", UNINHERITED_PROPERTY,
            border_width.top),
        get_property(path, "border-right-width", UNINHERITED_PROPERTY,
            border_width.right),
        get_property(path, "border-bottom-width", UNINHERITED_PROPERTY,
            border_width.bottom),
        get_property(path, "border-left-width", UNINHERITED_PROPERTY,
            border_width.left));
}

box_corner_sizes
get_border_radius_spec(
    style_search_path const* path,
    relative_length const& default_radius = relative_length(0, PIXELS))
{
    box_corner_sizes border_radius =
        get_property(path, "border-radius", UNINHERITED_PROPERTY,
            box_corner_sizes(
                make_vector(default_radius, default_radius)));
    return box_corner_sizes(
        get_property(path, "border-top-left-radius", UNINHERITED_PROPERTY,
            border_radius.corners[0]),
        get_property(path, "border-top-right-radius", UNINHERITED_PROPERTY,
            border_radius.corners[1]),
        get_property(path, "border-bottom-right-radius", UNINHERITED_PROPERTY,
            border_radius.corners[2]),
        get_property(path, "border-bottom-left-radius", UNINHERITED_PROPERTY,
            border_radius.corners[3]));
}

static void
update_panel_style_info(ui_context& ctx, panel_data& data,
    getter<string> const& substyle, widget_state state, ui_flag_set flags)
{
    refresh_keyed_data(data.style, combine_ids(ref(*ctx.style.id),
        combine_ids(ref(substyle.id()), make_id(state))));

    if (!is_valid(data.style))
    {
        panel_style_info info;

        stateful_style_path_storage storage;
        style_search_path const* path =
            add_substyle_to_path(&storage, ctx.style.path, ctx.style.path,
                get(substyle), state, flags);

        info.border_width =
            resolve_box_border_width(get_layout_traversal(ctx),
                get_border_width(path));

        info.border_color = get_color_property(path, "border-color");
        info.background_color = get_color_property(path, "background");

        box_corner_sizes border_radius = get_border_radius_spec(path);
        info.is_rounded = false;
        for (int i = 0; i != 4; ++i)
        {
            for (int j = 0; j != 2; ++j)
            {
                if (border_radius.corners[i][j].length > 0)
                    info.is_rounded = true;
            }
        }

        info.hidden_border_sides =
            get_property(path, "hide-border", UNINHERITED_PROPERTY,
                NO_SIDES);

        set(data.style, info);
    }
}

static void
begin_outer_panel(
    ui_context& ctx, panel_data& data, bordered_layout& outer,
    getter<string> const& style, widget_state state,
    layout const& layout_spec, ui_flag_set flags, widget_id id)
{
    if (is_refresh_pass(ctx))
        update_panel_style_info(ctx, data, style, state, flags);

    resolved_box_border_width const& border = get(data.style).border_width;
    outer.begin(ctx,
        box_border_width(
            absolute_length(float(border.top), PIXELS),
            absolute_length(float(border.right), PIXELS),
            absolute_length(float(border.bottom), PIXELS),
            absolute_length(float(border.left), PIXELS)),
        layout_spec);

    switch (ctx.event->category)
    {
     case RENDER_CATEGORY:
      {
        layout_box outer_region = outer.region();

        layout_box inner_region = remove_border(outer_region, border);
        side_selection hidden_sides = get(data.style).hidden_border_sides;
        if (hidden_sides != NO_SIDES)
        {
            if (hidden_sides & LEFT_SIDE)
            {
                inner_region.corner[0] -= border.left;
                inner_region.size[0] += border.left;
            }
            if (hidden_sides & RIGHT_SIDE)
            {
                inner_region.size[0] += border.right;
            }
            if (hidden_sides & TOP_SIDE)
            {
                inner_region.corner[1] -= border.top;
                inner_region.size[1] += border.top;
            }
            if (hidden_sides & BOTTOM_SIDE)
            {
                inner_region.size[1] += border.bottom;
            }
        }

        if (get(data.style).is_rounded)
        {
            caching_renderer cache(ctx, data.rendering,
                combine_ids(ref(*ctx.style.id),
                    combine_ids(ref(style.id()), make_id(state))),
                outer_region);
            if (cache.needs_rendering())
            {
                skia_renderer renderer(ctx, cache.image(), outer_region.size);

                SkPaint paint;
                paint.setFlags(SkPaint::kAntiAlias_Flag);

                stateful_style_path_storage storage;
                style_search_path const* path =
                    add_substyle_to_path(&storage, ctx.style.path,
                        ctx.style.path, get(style), state, flags);

                resolved_box_corner_sizes border_radii =
                    resolve_box_corner_sizes(
                        get_layout_traversal(ctx),
                        get_border_radius_spec(path),
                        vector<2,float>(outer_region.size));

                paint.setStyle(SkPaint::kFill_Style);

                if (outer_region != inner_region)
                {
                    set_color(paint, get(data.style).border_color);
                    draw_rect(renderer.canvas(), paint, outer_region,
                        border_radii);
                }

                set_color(paint, get(data.style).background_color);
                draw_rect(renderer.canvas(), paint, inner_region,
                    border_radii);

                renderer.cache();
                cache.mark_valid();
            }
            cache.draw();
        }
        else
        {
            if (outer_region != inner_region)
            {
                ctx.surface->draw_filled_box(
                    get(data.style).border_color,
                    box<2,double>(outer_region));
            }
            ctx.surface->draw_filled_box(
                get(data.style).background_color,
                box<2,double>(inner_region));
        }

        break;
      }

     case REGION_CATEGORY:
        // So the panel will block mouse events on things behind it.
        do_box_region(ctx, id, outer.region());
        break;

     case INPUT_CATEGORY:
        // So the panel will steal the focus if clicked on.
        if (!(flags && NO_CLICK_DETECTION) &&
            ctx.event->type == MOUSE_PRESS_EVENT && is_region_hot(ctx, id))
        {
            set_focus(ctx, id);
        }
        break;
    }
}

static void
begin_inner_panel(
    ui_context& ctx, panel_data& data, linear_layout& inner,
    layout const& layout_spec, ui_flag_set flags)
{
    layout_flag_set inner_layout_flags =
        FILL_X |
        ((layout_spec.flags & Y_ALIGNMENT_MASK) == BASELINE_Y ?
            BASELINE_Y : FILL_Y) |
        ((flags & NO_INTERNAL_PADDING) ? UNPADDED : PADDED);
    inner.begin(ctx, (flags & HORIZONTAL) ? 0 : 1,
	layout(inner_layout_flags, 1));
}

void panel::begin(
    ui_context& ctx, getter<string> const& style,
    layout const& layout_spec, ui_flag_set flags, widget_id id,
    widget_state state)
{
    ctx_ = &ctx;
    flags_ = flags;

    panel_data* data;
    get_cached_data(ctx, &data);

    init_optional_widget_id(ctx, id, data);

    begin_outer_panel(ctx, *data, outer_, style, state,
        add_default_padding(layout_spec, PADDED), flags, id);

    substyle_.begin(ctx, style, state, flags);

    begin_inner_panel(ctx, *data, inner_, layout_spec, flags);
}
void panel::end()
{
    if (ctx_)
    {
        inner_.end();
        substyle_.end();
        outer_.end();
        ctx_ = 0;
    }
}

layout_box panel::outer_region() const
{
    layout_box region = outer_.region();
    // When this is called, we're already inside the inner region, so we're
    // using a different transformation matrix than the outer region expects.
    // Thus, we need to adjust the region's corner to compensate.
    region.corner -= inner_.offset();
    return region;
}

layout_box panel::padded_region() const
{
    layout_box region = outer_.padded_region();
    // Same as above.
    region.corner -= inner_.offset();
    return region;
}

struct clickable_panel_data
{
    button_input_state input;
    caching_renderer_data rendering;
};

static void
draw_panel_focus_border(
    ui_context& ctx, panel& p, ui_flag_set flags,
    caching_renderer_data& rendering)
{
    if (!(flags & HIDE_FOCUS))
    {
        layout_box rect = p.outer_region();
        caching_renderer cache(ctx, rendering, *ctx.style.id,
            add_border(rect, get_padding_size(ctx)));
        if (cache.needs_rendering())
        {
            layout_vector const& padding = get_padding_size(ctx);
            skia_renderer renderer(ctx, cache.image(),
                rect.size + padding * 2);
            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);
            setup_focus_drawing(ctx, paint);
            renderer.canvas().translate(
                layout_scalar_as_skia_scalar(padding[0]),
                layout_scalar_as_skia_scalar(padding[1]));
            box_corner_sizes corners_spec =
                get_property(ctx, "border-radius", UNINHERITED_PROPERTY,
                    box_corner_sizes(make_vector(
                        relative_length(0, PIXELS),
                        relative_length(0, PIXELS))));
            resolved_box_corner_sizes corners =
                resolve_box_corner_sizes(get_layout_traversal(ctx),
                    corners_spec, vector<2,float>(rect.size));
            draw_rect(renderer.canvas(), paint, rect, corners);
            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }
}

void clickable_panel::begin(
    ui_context& ctx, getter<string> const& style,
    layout const& layout_spec,
    ui_flag_set flags, widget_id id)
{
    ALIA_GET_CACHED_DATA(clickable_panel_data)
    get_widget_id_if_needed(ctx, id);
    widget_state state;
    if (flags & SELECTED)
    {
        state = WIDGET_SELECTED;
        clicked_ = false;
    }
    else
    {
        state = get_button_state(ctx, id, data.input);
        clicked_ = do_button_input(ctx, id, data.input);
    }
    panel_.begin(ctx, style, layout_spec, flags, id, state);
    if (is_render_pass(ctx) && (state & WIDGET_FOCUSED))
        draw_panel_focus_border(ctx, panel_, flags, data.rendering);
}

void scrollable_panel::begin(
    ui_context& ctx, getter<string> const& style,
    layout const& layout_spec, ui_flag_set flags)
{
    widget_id id = get_widget_id(ctx);
    panel_data* data;
    get_cached_data(ctx, &data);
    begin_outer_panel(ctx, *data, outer_, style, WIDGET_NORMAL,
        layout_spec, flags, id);
    substyle_.begin(ctx, style, WIDGET_NORMAL, flags);
    unsigned scrollable_axes =
        ((flags & NO_HORIZONTAL_SCROLL) ? 0 : 1) |
        ((flags & NO_VERTICAL_SCROLL) ? 0 : 2);
    unsigned reserved_axes =
        ((flags & RESERVE_HORIZONTAL) ? 1 : 0) |
        ((flags & RESERVE_VERTICAL) ? 2 : 0);
    region_.begin(ctx, GROW | UNPADDED, scrollable_axes, id, reserved_axes);
    begin_inner_panel(ctx, *data, inner_, layout_spec, flags);
}
void scrollable_panel::end()
{
    inner_.end();
    region_.end();
    substyle_.end();
    outer_.end();
}

}
