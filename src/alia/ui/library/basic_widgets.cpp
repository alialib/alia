#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

// SEPARATOR

struct separator_data
{
    keyed_data<float> width;
    layout_leaf layout_node;
    caching_renderer_data rendering;
};

void do_separator(ui_context& ctx, layout const& layout_spec)
{
    ALIA_GET_CACHED_DATA(separator_data)

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        refresh_keyed_data(data.width, *ctx.style.id);
        if (!data.width.is_valid)
            set(data.width, get_float_property(ctx, "separator_width", 2));
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec,
            leaf_layout_requirements(
                make_layout_vector(
                    as_layout_size(data.width.value),
                    as_layout_size(data.width.value)),
                0, 0),
            FILL | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        layout_box const& region = data.layout_node.assignment().region;
        caching_renderer cache(ctx, data.rendering, *ctx.style.id, region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), region.size);
            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);
            paint.setStrokeWidth(2);
            paint.setStrokeCap(SkPaint::kRound_Cap);
            set_color(paint, get_color_property(ctx, "separator_color"));
            renderer.canvas().drawLine(
                SkIntToScalar(1), SkIntToScalar(1),
                layout_scalar_as_skia_scalar(region.size[0] - 1),
                layout_scalar_as_skia_scalar(region.size[1] - 1),
                paint);
            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
        break;
      }
    }
}

// COLOR

void do_color(ui_context& ctx, getter<rgba8> const& color,
    layout const& layout_spec)
{
    ALIA_GET_CACHED_DATA(simple_display_data)

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx),
            add_default_size(layout_spec, size(1.4f, 1.4f, EM)),
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        layout_box const& region = data.layout_node.assignment().region;
        caching_renderer cache(ctx, data.rendering, color.id(), region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), region.size);
            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);
            paint.setStyle(SkPaint::kFill_Style);
            set_color(paint, get(color));
            SkRect rect;
            rect.fLeft = 0;
            rect.fRight = layout_scalar_as_skia_scalar(region.size[0]);
            rect.fTop = 0;
            rect.fBottom = layout_scalar_as_skia_scalar(region.size[1]);
            SkScalar radius =
                SkScalarDiv(
                    layout_scalar_as_skia_scalar(
                        (std::min)(region.size[0], region.size[1])),
                    SkIntToScalar(3));
            renderer.canvas().drawRoundRect(rect, radius, radius, paint);
            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
        break;
      }
    }
}

// BULLETED LIST

void do_bullet(ui_context& ctx, layout const& layout_spec)
{
    ALIA_GET_CACHED_DATA(simple_display_data)

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        layout_scalar size =
            resolve_layout_height(get_layout_traversal(ctx), 1, EX);
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec,
            leaf_layout_requirements(
                make_layout_vector(size, size),
                size, 0),
            CENTER_X | BASELINE_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        layout_box const& region = data.layout_node.assignment().region;
        caching_renderer cache(ctx, data.rendering, *ctx.style.id, region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), region.size);
            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);
            paint.setStyle(SkPaint::kFill_Style);
            set_color(paint, ctx.style.properties->text_color);
            draw_round_rect(renderer.canvas(), paint, region.size);
            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
        break;
      }
    }
}

void bulleted_list::begin(ui_context& ctx, layout const& layout_spec)
{
    ctx_ = &ctx;

    grid_.begin(ctx, layout_spec);

    // Add an empty row (with no height) at the top to force the content
    // column to grab any extra space in the grid.
    {
        grid_row r(grid_);
        do_spacer(ctx, layout(size(0, 0, PIXELS), UNPADDED));
        do_spacer(ctx, layout(size(0, 0, PIXELS), UNPADDED | GROW));
    }
}
void bulleted_list::end()
{
    grid_.end();
}
void bulleted_item::begin(bulleted_list& list, layout const& layout_spec)
{
    row_.begin(list.grid_);
    do_bullet(*list.ctx_);
}
void bulleted_item::end()
{
    row_.end();
}

// CHECK BOX

struct check_box_renderer : boolean_widget_renderer
{};

struct check_box_data
{
    layout_leaf layout_node;
    themed_rendering_data<check_box_renderer> rendering;
    button_input_state input;
};

struct default_check_box_renderer : check_box_renderer
{
    layout_vector default_size(ui_context& ctx) const
    {
        return resolve_layout_size(get_layout_traversal(ctx),
            size(1.2f, 1.2f, EM));
    }
    void draw(
        ui_context& ctx, renderer_data_ptr& data_ptr, layout_box const& region,
        getter<bool> const& value, widget_state state) const
    {
        caching_renderer_data* data;
        cast_data_ptr(&data, data_ptr);

        layout_vector const& padding_size = get_padding_size(ctx);
        layout_box padded_region = add_border(region, padding_size);
        layout_vector const& unpadded_size = region.size;

        caching_renderer cache(ctx, *data,
            combine_ids(combine_ids(ref(value.id()), ref(*ctx.style.id)),
                make_id(state)),
            padded_region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), padded_region.size);

            style_tree const* control_style =
                find_substyle(ctx.style.path, "control", state);

            rgba8 bg_color =
                get_color_property(control_style, "background_color");
            rgba8 fg_color =
                get_color_property(control_style, "foreground_color");

            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);

            renderer.canvas().translate(
                layout_scalar_as_skia_scalar(padding_size[0]),
                layout_scalar_as_skia_scalar(padding_size[1]));

            paint.setStyle(SkPaint::kFill_Style);
            set_color(paint, bg_color);
            draw_round_rect(renderer.canvas(), paint, unpadded_size);

            if (value.is_gettable() && get(value))
            {
                set_color(paint, fg_color);
                paint.setStrokeCap(SkPaint::kRound_Cap);
                paint.setStrokeJoin(SkPaint::kRound_Join);
                SkScalar dx =
                    SkScalarDiv(
                        layout_scalar_as_skia_scalar(unpadded_size[0]),
                        SkDoubleToScalar(10.));
                SkScalar dy =
                    SkScalarDiv(
                        layout_scalar_as_skia_scalar(unpadded_size[1]),
                        SkDoubleToScalar(10.));
                paint.setStrokeWidth(SkScalarMul(dx, SkDoubleToScalar(1.6)));
                renderer.canvas().drawLine(
                    SkScalarMul(dx, SkIntToScalar(3)),
                    SkScalarMul(dy, SkIntToScalar(3)),
                    SkScalarMul(dx, SkIntToScalar(7)),
                    SkScalarMul(dy, SkIntToScalar(7)),
                    paint);
                renderer.canvas().drawLine(
                    SkScalarMul(dx, SkIntToScalar(3)),
                    SkScalarMul(dy, SkIntToScalar(7)),
                    SkScalarMul(dx, SkIntToScalar(7)),
                    SkScalarMul(dy, SkIntToScalar(3)),
                    paint);
            }

            if (state & WIDGET_FOCUSED)
                draw_round_focus_rect(ctx, renderer.canvas(), unpadded_size);

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }
};

check_box_result do_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec,
    widget_id id)
{
    ALIA_GET_CACHED_DATA(check_box_data)

    init_optional_widget_id(ctx, id, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        static default_check_box_renderer default_renderer;
        refresh_themed_rendering_data(ctx, data.rendering, &default_renderer);
        check_box_renderer const* renderer = data.rendering.renderer;
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(renderer->default_size(ctx), 0, 0),
            LEFT | CENTER_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        data.rendering.renderer->draw(ctx, data.rendering.data,
            data.layout_node.assignment().region, value,
            get_button_state(ctx, id, data.input));
        break;
      }

     case REGION_CATEGORY:
        do_box_region(ctx, id, data.layout_node.assignment().region);
        break;

     case INPUT_CATEGORY:
        if (do_button_input(ctx, id, data.input))
        {
            check_box_result result;
            set_new_value(value, result,
                value.is_gettable() ? !value.get() : true);
            return result;
        }
        break;
    }

    check_box_result result;
    result.changed = false;
    return result;
}

check_box_result do_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    getter<string> const& text,
    layout const& layout_spec,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    row_layout row(ctx, add_default_alignment(layout_spec, LEFT, BASELINE_Y));
    check_box_result result = do_check_box(ctx, value, CENTER, id);
    do_text(ctx, text);
    do_box_region(ctx, id, row.region());
    return result;
}

// RADIO BUTTON

struct radio_button_renderer : boolean_widget_renderer
{};

struct radio_button_data
{
    layout_leaf layout_node;
    themed_rendering_data<radio_button_renderer> rendering;
    button_input_state input;
};

struct default_radio_button_renderer : radio_button_renderer
{
    layout_vector default_size(ui_context& ctx) const
    {
        return resolve_layout_size(get_layout_traversal(ctx),
            size(1.2f, 1.2f, EM));
    }
    void draw(
        ui_context& ctx, renderer_data_ptr& data_ptr, layout_box const& region,
        getter<bool> const& value, widget_state state) const
    {
        caching_renderer_data* data;
        cast_data_ptr(&data, data_ptr);

        layout_vector const& padding_size = get_padding_size(ctx);
        layout_box padded_region = add_border(region, padding_size);
        layout_vector const& unpadded_size = region.size;

        caching_renderer cache(ctx, *data,
            combine_ids(combine_ids(ref(value.id()), ref(*ctx.style.id)),
                make_id(state)),
            padded_region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), padded_region.size);

            style_tree const* control_style =
                find_substyle(ctx.style.path, "control", state);

            rgba8 bg_color =
                get_color_property(control_style, "background_color");
            rgba8 fg_color =
                get_color_property(control_style, "foreground_color");

            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);

            renderer.canvas().translate(
                layout_scalar_as_skia_scalar(padding_size[0]),
                layout_scalar_as_skia_scalar(padding_size[1]));

            double scale = (unpadded_size[0] + unpadded_size[1]) / 20.;
            layout_vector center = unpadded_size / 2;

            if (state & WIDGET_FOCUSED)
            {
                set_color(paint, get_color_property(ctx, "focus_color"));
                paint.setStyle(SkPaint::kFill_Style);
                renderer.canvas().drawCircle(
                    layout_scalar_as_skia_scalar(center[0]),
                    layout_scalar_as_skia_scalar(center[1]),
                    SkDoubleToScalar(6.4 * scale), paint);
            }

            if (bg_color.a)
            {
                set_color(paint, bg_color);
                paint.setStyle(SkPaint::kFill_Style);
                renderer.canvas().drawCircle(
                    layout_scalar_as_skia_scalar(center[0]),
                    layout_scalar_as_skia_scalar(center[1]),
                    SkDoubleToScalar(5 * scale), paint);
            }

            if (value.is_gettable() && get(value))
            {
                set_color(paint, fg_color);
                paint.setStyle(SkPaint::kFill_Style);
                renderer.canvas().drawCircle(
                    layout_scalar_as_skia_scalar(center[0]),
                    layout_scalar_as_skia_scalar(center[1]),
                    SkDoubleToScalar(2.5 * scale), paint);
            }

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }
};

radio_button_result
do_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec,
    widget_id id)
{
    ALIA_GET_CACHED_DATA(radio_button_data)

    init_optional_widget_id(ctx, id, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        static default_radio_button_renderer default_renderer;
        refresh_themed_rendering_data(ctx, data.rendering, &default_renderer);
        radio_button_renderer const* renderer = data.rendering.renderer;
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(renderer->default_size(ctx), 0, 0),
            LEFT | CENTER_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        data.rendering.renderer->draw(ctx, data.rendering.data,
            data.layout_node.assignment().region, value,
            get_button_state(ctx, id, data.input));
        break;
      }

     case REGION_CATEGORY:
        do_box_region(ctx, id, data.layout_node.assignment().region);
        break;

     case INPUT_CATEGORY:
        if (do_button_input(ctx, id, data.input))
        {
            radio_button_result result;
            set_new_value(value, result, true);
            return result;
        }
        break;
    }

    radio_button_result result;
    result.changed = false;
    return result;
}

radio_button_result do_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    getter<string> const& text,
    layout const& layout_spec,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    row_layout row(ctx, add_default_alignment(layout_spec, LEFT, BASELINE_Y));
    radio_button_result result = do_radio_button(ctx, value, CENTER, id);
    do_text(ctx, text);
    do_box_region(ctx, id, row.region());
    return result;
}

// NODE EXPANDER

struct node_expander_renderer : boolean_widget_renderer
{};

struct node_expander_data
{
    layout_leaf layout_node;
    themed_rendering_data<node_expander_renderer> rendering;
    button_input_state input;
};

struct default_node_expander_renderer : node_expander_renderer
{
    layout_vector default_size(ui_context& ctx) const
    {
        return resolve_layout_size(get_layout_traversal(ctx),
            size(1.2f, 1.2f, EM));
    }
    void draw(
        ui_context& ctx, renderer_data_ptr& data_ptr, layout_box const& region,
        getter<bool> const& value, widget_state state) const
    {
        caching_renderer_data* data;
        cast_data_ptr(&data, data_ptr);

        layout_vector const& padding_size = get_padding_size(ctx);
        layout_box padded_region = add_border(region, padding_size);
        layout_vector const& unpadded_size = region.size;

        caching_renderer cache(ctx, *data,
            combine_ids(combine_ids(ref(value.id()), ref(*ctx.style.id)),
                make_id(state)),
            padded_region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), padded_region.size);

            style_tree const* control_style =
                find_substyle(ctx.style.path, "control", state);

            rgba8 bg_color =
                get_color_property(control_style, "background_color");
            rgba8 fg_color =
                get_color_property(control_style, "foreground_color");

            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);

            renderer.canvas().translate(
                layout_scalar_as_skia_scalar(padding_size[0]),
                layout_scalar_as_skia_scalar(padding_size[1]));

            if ((state & WIDGET_PRIMARY_STATE_MASK) != WIDGET_NORMAL)
            {
                paint.setStyle(SkPaint::kFill_Style);
                set_color(paint, bg_color);
                draw_round_rect(renderer.canvas(), paint, unpadded_size);
            }

            if (state & WIDGET_FOCUSED)
                draw_round_focus_rect(ctx, renderer.canvas(), unpadded_size);

            renderer.canvas().translate(
                SkScalarDiv(
                    layout_scalar_as_skia_scalar(unpadded_size[0]),
                    SkIntToScalar(2)),
                SkScalarDiv(
                    layout_scalar_as_skia_scalar(unpadded_size[1]),
                    SkIntToScalar(2)));

            if (value.is_gettable() && get(value))
                renderer.canvas().rotate(90);

            {
                set_color(paint, fg_color);
                paint.setStyle(SkPaint::kFill_Style);
                SkScalar a =
                    SkScalarDiv(
                        layout_scalar_as_skia_scalar(unpadded_size[0]),
                        SkDoubleToScalar(1.8));
                SkPath path;
                path.incReserve(4);
                SkPoint p0;
                p0.fX = SkScalarMul(a, SkDoubleToScalar(-0.34));
                p0.fY = SkScalarMul(a, SkDoubleToScalar(-0.5));
                path.moveTo(p0);
                SkPoint p1;
                p1.fX = p0.fX;
                p1.fY = SkScalarMul(a, SkDoubleToScalar(0.5));
                path.lineTo(p1);
                SkPoint p2;
                p2.fX = p0.fX + SkScalarMul(a, SkDoubleToScalar(0.866));
                p2.fY = 0;
                path.lineTo(p2);
                path.lineTo(p0);
                renderer.canvas().drawPath(path, paint);
            }

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }
};

node_expander_result do_node_expander(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec,
    widget_id id)
{
    ALIA_GET_CACHED_DATA(node_expander_data)

    init_optional_widget_id(ctx, id, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        static default_node_expander_renderer default_renderer;
        refresh_themed_rendering_data(ctx, data.rendering, &default_renderer);
        node_expander_renderer const* renderer = data.rendering.renderer;
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(renderer->default_size(ctx), 0, 0),
            LEFT | CENTER_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        data.rendering.renderer->draw(ctx, data.rendering.data,
            data.layout_node.assignment().region, value,
            get_button_state(ctx, id, data.input));
        break;
      }

     case REGION_CATEGORY:
        do_box_region(ctx, id, data.layout_node.assignment().region);
        break;

     case INPUT_CATEGORY:
        if (do_button_input(ctx, id, data.input))
        {
            node_expander_result result;
            set_new_value(value, result,
                value.is_gettable() ? !value.get() : true);
            return result;
        }
        break;
    }

    node_expander_result result;
    result.changed = false;
    return result;
}

// PROGRESS BAR

void do_progress_bar(ui_context& ctx, getter<double> const& progress,
    layout const& layout_spec)
{
    ALIA_GET_CACHED_DATA(simple_display_data)

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx),
            // Note that this is effectively a default minimum width, since
            // the true default behavior is to fill the allotted width.
            add_default_size(layout_spec, size(10.f, 1.4f, EM)),
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0),
            FILL_X | TOP | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        layout_box const& region = data.layout_node.assignment().region;
        caching_renderer cache(ctx, data.rendering, progress.id(), region);
        if (cache.needs_rendering())
        {
            skia_renderer sr(ctx, cache.image(), region.size);

            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);
            paint.setStyle(SkPaint::kFill_Style);

            style_tree const* progress_bar_style =
                find_substyle(ctx.style.path, "progress_bar", WIDGET_NORMAL);

            rgba8 outline_color =
                get_color_property(progress_bar_style, "outline_color");
            rgba8 background_color = ctx.style.properties->background_color;
            rgba8 bar_color =
                get_color_property(progress_bar_style, "bar_color");

            SkScalar trim =
                SkScalarDiv(
                    layout_scalar_as_skia_scalar(region.size[1]),
                    SkDoubleToScalar(12.));

            // The following is kinda iffy with regards to treating SkScalars
            // abstractly, but I think it works in any case.

            box<2,SkScalar> full_box(
                make_vector<SkScalar>(0, 0),
                make_vector<SkScalar>(
                    layout_scalar_as_skia_scalar(region.size[0]),
                    layout_scalar_as_skia_scalar(region.size[1])));

            set_color(paint, outline_color);
            draw_rect(sr.canvas(), paint, full_box);

            set_color(paint, background_color);
            draw_rect(sr.canvas(), paint, add_border(full_box, -trim));

            box<2,SkScalar> bar_box = add_border(full_box, -trim * 2);
            bar_box.size[0] = SkDoubleToScalar(bar_box.size[0] *
                (progress.is_gettable() ? get(progress) : 0.));
            set_color(paint, bar_color);
            draw_rect(sr.canvas(), paint, bar_box);

            sr.cache();
            cache.mark_valid();
        }
        cache.draw();
        break;
      }
    }
}

// BUTTON

struct button_data
{
    button_input_state input;
    focus_rect_data focus_rect;
};

button_result
do_button(
    ui_context& ctx,
    getter<string> const& label,
    layout const& layout_spec,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    ALIA_GET_CACHED_DATA(button_data)
    widget_state state = get_button_state(ctx, id, data.input);
    panel p(ctx, text("button"),
        add_default_alignment(layout_spec, LEFT, TOP), NO_FLAGS, id, state);
    do_text(ctx, label, CENTER);
    if (is_render_pass(ctx) && (state & WIDGET_FOCUSED))
        draw_focus_rect(ctx, data.focus_rect, p.outer_region());
    return do_button_input(ctx, id, data.input);
}

// ICON BUTTON

struct icon_button_renderer : dispatch_interface
{
    virtual layout_vector default_size(ui_context& ctx) const = 0;

    virtual void draw(
        ui_context& ctx, renderer_data_ptr& data_ptr, layout_box const& region,
        icon_type icon, widget_state state) const = 0;
};

struct icon_button_data
{
    layout_leaf layout_node;
    themed_rendering_data<icon_button_renderer> rendering;
    button_input_state input;
};

struct default_icon_button_renderer : icon_button_renderer
{
    layout_vector default_size(ui_context& ctx) const
    {
        return resolve_layout_size(get_layout_traversal(ctx),
            size(1.2f, 1.2f, EM));
    }
    void draw(
        ui_context& ctx, renderer_data_ptr& data_ptr, layout_box const& region,
        icon_type icon, widget_state state) const
    {
        caching_renderer_data* data;
        cast_data_ptr(&data, data_ptr);

        layout_vector const& padding_size = get_padding_size(ctx);
        layout_box padded_region = add_border(region, padding_size);
        layout_vector const& unpadded_size = region.size;

        caching_renderer cache(ctx, *data,
            combine_ids(combine_ids(make_id(icon), ref(*ctx.style.id)),
                make_id(state)),
            padded_region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), padded_region.size);

            style_tree const* control_style =
                find_substyle(ctx.style.path, "control", state);

            rgba8 bg_color =
                get_color_property(control_style, "background_color");
            rgba8 fg_color =
                get_color_property(control_style, "foreground_color");

            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);

            renderer.canvas().translate(
                layout_scalar_as_skia_scalar(padding_size[0]),
                layout_scalar_as_skia_scalar(padding_size[1]));

            if ((state & WIDGET_PRIMARY_STATE_MASK) != WIDGET_NORMAL)
            {
                paint.setStyle(SkPaint::kFill_Style);
                set_color(paint, bg_color);
                draw_round_rect(renderer.canvas(), paint, unpadded_size);
            }

            if (state & WIDGET_FOCUSED)
                draw_round_focus_rect(ctx, renderer.canvas(), unpadded_size);

            renderer.canvas().translate(
                SkScalarDiv(
                    layout_scalar_as_skia_scalar(unpadded_size[0]),
                    SkIntToScalar(2)),
                SkScalarDiv(
                    layout_scalar_as_skia_scalar(unpadded_size[1]),
                    SkIntToScalar(2)));

            set_color(paint, fg_color);

            switch (icon)
            {
             case REMOVE_ICON:
              {
                SkScalar a =
                    SkScalarDiv(
                        layout_scalar_as_skia_scalar(unpadded_size[0]),
                        SkIntToScalar(5));
                paint.setStrokeWidth(a);
                paint.setStrokeCap(SkPaint::kRound_Cap);
                paint.setStyle(SkPaint::kFill_Style);
                renderer.canvas().drawLine(-a, -a,  a,  a, paint);
                renderer.canvas().drawLine(-a,  a,  a, -a, paint);
                break;
              }
             default:
                break;
            }

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }
};

icon_button_result
do_icon_button(
    ui_context& ctx,
    icon_type icon,
    layout const& layout_spec,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);

    ALIA_GET_CACHED_DATA(icon_button_data)

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        static default_icon_button_renderer default_renderer;
        refresh_themed_rendering_data(ctx, data.rendering, &default_renderer);
        icon_button_renderer const* renderer = data.rendering.renderer;
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(renderer->default_size(ctx), 0, 0),
            LEFT | CENTER_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        data.rendering.renderer->draw(ctx, data.rendering.data,
            data.layout_node.assignment().region, icon,
            get_button_state(ctx, id, data.input));
        break;
      }

     case REGION_CATEGORY:
        do_box_region(ctx, id, data.layout_node.assignment().region);
        break;

     case INPUT_CATEGORY:
        if (do_button_input(ctx, id, data.input))
            return true;
        break;
    }

    return false;
}

}
