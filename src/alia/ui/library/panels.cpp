#include <alia/ui/library/panels.hpp>

namespace alia {

void refresh_panel_style_info(
    ui_context& ctx, keyed_data<panel_style_info>& stored_info,
    getter<string> const& substyle, widget_state state,
    add_substyle_flag_set flags)
{
    refresh_keyed_data(stored_info, combine_ids(ref(*ctx.style.id),
        combine_ids(ref(substyle.id()), make_id(state))));
    if (!is_valid(stored_info))
    {
        panel_style_info info;

        stateful_style_path_storage storage;
        style_search_path const* path =
            add_substyle_to_path(&storage, ctx.style.path, ctx.style.path,
                get(substyle), state, flags);

        info.size = make_vector(
            get_property(path, "width", UNINHERITED_PROPERTY,
                absolute_length(0, PIXELS)),
            get_property(path, "height", UNINHERITED_PROPERTY,
                absolute_length(0, PIXELS)));

        info.margin =
            resolve_box_border_width(get_layout_traversal(ctx),
                get_margin_property(path));

        info.border_width =
            resolve_box_border_width(get_layout_traversal(ctx),
                get_border_width_property(path));

        info.padding =
            resolve_box_border_width(get_layout_traversal(ctx),
                get_padding_property(path));

        info.border_color = get_color_property(path, "border-color");
        info.background_color = get_color_property(path, "background");

        info.border_radii = get_border_radius_property(path);
        info.is_rounded = false;
        for (int i = 0; i != 4; ++i)
        {
            for (int j = 0; j != 2; ++j)
            {
                if (info.border_radii.corners[i][j].length > 0)
                    info.is_rounded = true;
            }
        }

        set(stored_info, info);
    }
}

static void
draw_panel_focus_border(
    ui_context& ctx, caching_renderer_data& rendering,
    getter<panel_style_info> const& style_info,
    layout_box const& outer_region)
{
    layout_box padded_region = add_border(outer_region, get_padding_size(ctx));
    caching_renderer cache(ctx, rendering, *ctx.style.id, padded_region);
    if (cache.needs_rendering())
    {
        skia_renderer renderer(ctx, cache.image(), padded_region.size);
        SkPaint paint;
        paint.setFlags(SkPaint::kAntiAlias_Flag);
        setup_focus_drawing(ctx, paint);
        renderer.canvas().translate(
            layout_scalar_as_skia_scalar(get_padding_size(ctx)[0]),
            layout_scalar_as_skia_scalar(get_padding_size(ctx)[1]));
        resolved_box_corner_sizes border_radii =
            resolve_box_corner_sizes(
                get_layout_traversal(ctx),
                get(style_info).border_radii,
                vector<2,float>(outer_region.size));
        draw_rect(renderer.canvas(), paint,
            layout_box_as_skia_box(
                layout_box(make_layout_vector(0, 0), outer_region.size)),
            border_radii);
        renderer.cache();
        cache.mark_valid();
    }
    cache.draw();
}

static void
begin_outer_panel(
    ui_context& ctx, custom_panel_data& data,
    getter<panel_style_info> const& style_info,
    bordered_layout& outer, layout const& layout_spec,
    panel_flag_set flags, widget_id id, widget_state state)
{
    box_border_width<layout_scalar> total_border =
        as_layout_size(get(style_info).margin) +
        as_layout_size(get(style_info).border_width);
    if (!(flags & PANEL_IGNORE_STYLE_PADDING))
        total_border += as_layout_size(get(style_info).padding);
    outer.begin(ctx,
        box_border_width<absolute_length>(
            absolute_length(float(total_border.top), PIXELS),
            absolute_length(float(total_border.right), PIXELS),
            absolute_length(float(total_border.bottom), PIXELS),
            absolute_length(float(total_border.left), PIXELS)),
        add_default_size(layout_spec, get(style_info).size));

    switch (ctx.event->category)
    {
     case RENDER_CATEGORY:
      {
        layout_box outer_region = remove_border(outer.region(),
            as_layout_size(get(style_info).margin));

        if (get(style_info).is_rounded)
        {
            caching_renderer cache(ctx, data.rendering, style_info.id(),
                outer_region);
            if (cache.needs_rendering())
            {
                skia_renderer renderer(ctx, cache.image(), outer_region.size);

                renderer.canvas().translate(
                    -layout_scalar_as_skia_scalar(outer_region.corner[0]),
                    -layout_scalar_as_skia_scalar(outer_region.corner[1]));

                SkPaint paint;
                paint.setFlags(SkPaint::kAntiAlias_Flag);

                resolved_box_corner_sizes border_radii =
                    resolve_box_corner_sizes(
                        get_layout_traversal(ctx),
                        get(style_info).border_radii,
                        vector<2,float>(outer_region.size));

                paint.setStyle(SkPaint::kFill_Style);

                skia_box background_region = remove_border(
                    layout_box_as_skia_box(outer_region),
                    box_border_width<SkScalar>(
                        get(style_info).border_width.top,
                        get(style_info).border_width.right,
                        get(style_info).border_width.bottom,
                        get(style_info).border_width.left));

                if (get(style_info).border_width !=
                    box_border_width<float>(0, 0, 0, 0))
                {
                    set_color(paint, get(style_info).border_color);
                    draw_rect(renderer.canvas(), paint,
                        layout_box_as_skia_box(outer_region), border_radii);
                }

                set_color(paint, get(style_info).background_color);
                    draw_rect(renderer.canvas(), paint,
                        background_region,
                        adjust_border_radii_for_border_width(border_radii,
                            get(style_info).border_width));

                renderer.cache();
                cache.mark_valid();
            }
            cache.draw();
        }
        else
        {
            box<2,float> background_region = remove_border(
                box<2,float>(outer_region),
                get(style_info).border_width);
            if (get(style_info).border_width !=
                box_border_width<float>(0, 0, 0, 0))
            {
                ctx.surface->draw_filled_box(
                    get(style_info).border_color,
                    box<2,double>(outer_region));
            }
            ctx.surface->draw_filled_box(
                get(style_info).background_color,
                box<2,double>(background_region));
        }

        if ((state & WIDGET_FOCUSED) && !(flags & PANEL_HIDE_FOCUS))
        {
            draw_panel_focus_border(ctx, data.focus_rendering,
                style_info, outer_region);
        }

        break;
      }

     case REGION_CATEGORY:
        // So the panel will block mouse events on things behind it.
        do_box_region(ctx, id, remove_border(outer.region(),
            as_layout_size(get(style_info).margin)));
        break;

     case INPUT_CATEGORY:
        // So the panel will steal the focus if clicked on.
        if (!(flags && PANEL_NO_CLICK_DETECTION) &&
            ctx.event->type == MOUSE_PRESS_EVENT && is_region_hot(ctx, id))
        {
            set_focus(ctx, id);
        }
        break;
    }
}

static void
begin_inner_panel(
    ui_context& ctx, custom_panel_data& data, linear_layout& inner,
    layout const& layout_spec, panel_flag_set flags)
{
    layout_flag_set inner_layout_flags =
        FILL_X |
        ((layout_spec.flags & Y_ALIGNMENT_MASK) == BASELINE_Y ?
            BASELINE_Y : FILL_Y) |
        ((flags & PANEL_NO_INTERNAL_PADDING) ? UNPADDED : PADDED);
    inner.begin(ctx,
        (flags & PANEL_HORIZONTAL) ? HORIZONTAL_LAYOUT : VERTICAL_LAYOUT,
	layout(inner_layout_flags, 1));
}

void custom_panel::begin(
    ui_context& ctx, custom_panel_data& data,
    getter<panel_style_info> const& style,
    layout const& layout_spec, panel_flag_set flags, widget_id id,
    widget_state state)
{
    ctx_ = &ctx;
    flags_ = flags;

    init_optional_widget_id(ctx, id, &data);

    begin_outer_panel(ctx, data, style, outer_,
        add_default_padding(layout_spec, PADDED), flags, id, state);

    begin_inner_panel(ctx, data, inner_, layout_spec, flags);
}

struct panel_data
{
    custom_panel_data panel;
    keyed_data<panel_style_info> style_info;
};

void panel::begin(
    ui_context& ctx, getter<string> const& style,
    layout const& layout_spec, panel_flag_set flags, widget_id id,
    widget_state state)
{
    ctx_ = &ctx;

    get_cached_data(ctx, &data_);

    refresh_panel_style_info(ctx, data_->style_info, style, state);

    init_optional_widget_id(ctx, id, &data_);

    begin_outer_panel(ctx, data_->panel,
        make_custom_getter(&get(data_->style_info),
            ref(data_->style_info.key.get())),
        outer_, add_default_padding(layout_spec, PADDED), flags, id, state);

    substyle_.begin(ctx, style, state);

    begin_inner_panel(ctx, data_->panel, inner_, layout_spec, flags);
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
    return remove_border(region,
        as_layout_size(get(data_->style_info).margin));
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
};

void clickable_panel::begin(
    ui_context& ctx, getter<string> const& style,
    layout const& layout_spec,
    panel_flag_set flags, widget_id id)
{
    ALIA_GET_CACHED_DATA(clickable_panel_data)
    get_widget_id_if_needed(ctx, id);
    widget_state state;
    if (flags & PANEL_SELECTED)
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
}

void scrollable_panel::begin(
    ui_context& ctx, getter<string> const& style,
    layout const& layout_spec, panel_flag_set flags)
{
    widget_id id = get_widget_id(ctx);
    panel_data* data;
    get_cached_data(ctx, &data);
    refresh_panel_style_info(ctx, data->style_info, style, WIDGET_NORMAL);
    begin_outer_panel(ctx, data->panel,
        make_custom_getter(&get(data->style_info),
            ref(data->style_info.key.get())),
        outer_, layout_spec, flags | PANEL_IGNORE_STYLE_PADDING,
        id, WIDGET_NORMAL);
    substyle_.begin(ctx, style, WIDGET_NORMAL);
    unsigned scrollable_axes =
        ((flags & PANEL_NO_HORIZONTAL_SCROLLING) ? 0 : 1) |
        ((flags & PANEL_NO_VERTICAL_SCROLLING) ? 0 : 2);
    unsigned reserved_axes =
        ((flags & PANEL_RESERVE_HORIZONTAL_SCROLLBAR) ? 1 : 0) |
        ((flags & PANEL_RESERVE_VERTICAL_SCROLLBAR) ? 2 : 0);
    region_.begin(ctx, GROW | UNPADDED, scrollable_axes, id, reserved_axes);   
    panel_style_info const& style_info = get(data->style_info);
    padding_border_.begin(ctx, 
        box_border_width<absolute_length>(
            absolute_length(float(style_info.padding.top), PIXELS),
            absolute_length(float(style_info.padding.right), PIXELS),
            absolute_length(float(style_info.padding.bottom), PIXELS),
            absolute_length(float(style_info.padding.left), PIXELS)));
    begin_inner_panel(ctx, data->panel, inner_, layout_spec, flags);
}
void scrollable_panel::end()
{
    inner_.end();
    padding_border_.end();
    region_.end();
    substyle_.end();
    outer_.end();
}

}
