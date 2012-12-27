#include <alia/ui_library.hpp>
#include <alia/ui_utilities.hpp>
#include <alia/ui_system.hpp>

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
            rgba8 bg_color = ctx.style.properties->bg_color;
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

            set_color(paint, bg_color);
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

// BORDERED BOX

void bordered_box::begin(
    ui_context& ctx, layout const& layout_spec, ui_flag_set flags)
{
    box_.begin(ctx, (flags & HORIZONTAL) ? 0 : 1, layout_spec);
    if (is_render_pass(ctx))
    {
        ctx.surface->draw_filled_box(ctx.style.properties->border_color,
            box<2,double>(add_border(box_.region(), get_padding_size(ctx))));
    }
}
void bordered_box::end()
{
    box_.end();
}

// PANELS

struct panel_data
{
    caching_renderer_data rendering;
};

static void begin_panel(
    ui_context& ctx, column_layout& outer, widget_id id, ui_flag_set flags,
    panel_data& data)
{
    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        break;

     case RENDER_CATEGORY:
      {
        if (flags & ROUNDED)
        {
            layout_box const& rect = outer.region();
            caching_renderer cache(ctx, data.rendering, *ctx.style.id, rect);
            if (cache.needs_rendering())
            {
                int padding = get_padding_size(ctx)[0];
                skia_renderer renderer(ctx, cache.image(), rect.size);
                SkPaint paint;
                paint.setFlags(SkPaint::kAntiAlias_Flag);
                set_color(paint, ctx.style.properties->bg_color);
                SkRect sr;
                sr.fLeft = 0;
                sr.fRight = layout_scalar_as_skia_scalar(rect.size[0]);
                sr.fTop = 0;
                sr.fBottom = layout_scalar_as_skia_scalar(rect.size[1]);
                if (flags & ROUNDED)
                {
                    SkScalar radius =
                        layout_scalar_as_skia_scalar(padding * 2);
                    renderer.canvas().drawRoundRect(sr, radius, radius, paint);
                }
                else
                    renderer.canvas().drawRect(sr, paint);
                renderer.cache();
                cache.mark_valid();
            }
            cache.draw();
        }
        else
        {
            ctx.surface->draw_filled_box(ctx.style.properties->bg_color,
                box<2,double>(outer.region()));
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

    outer_.begin(ctx, add_default_padding(layout_spec, PADDED));

    substyle_.begin(ctx, style, state);

    begin_panel(ctx, outer_, id, flags, *data);

    layout_flag_set inner_layout_flags =
        FILL_X |
        ((layout_spec.flags & Y_ALIGNMENT_MASK) == BASELINE_Y ?
            BASELINE_Y : FILL_Y) |
        ((flags & NO_INTERNAL_PADDING) ? UNPADDED : PADDED);
    inner_.begin(ctx, (flags & HORIZONTAL) ? 0 : 1,
	layout(inner_layout_flags, 1));
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
    // outer_.region() returns its region in its own frame of reference, which
    // isn't valid for panel users, so this is used instead.
    if (flags_ & NO_INTERNAL_PADDING)
        return inner_.region();
    else
        return add_border(inner_.region(), get_padding_size(*ctx_));
}

struct clickable_panel_data
{
    button_input_state input;
    caching_renderer_data rendering;
};

static void draw_panel_focus_border(
    ui_context& ctx, panel& p, ui_flag_set flags,
    caching_renderer_data& rendering)
{
    if (!(flags & NO_FOCUS_INDICATOR))
    {
        layout_box rect = p.outer_region();
        if (flags & ROUNDED)
        {
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
                SkRect sr;
                sr.fLeft = 0;
                sr.fRight = layout_scalar_as_skia_scalar(rect.size[0]);
                sr.fTop = 0;
                sr.fBottom = layout_scalar_as_skia_scalar(rect.size[1]);
                SkScalar radius = layout_scalar_as_skia_scalar(padding[0] * 2);
                renderer.canvas().drawRoundRect(sr, radius, radius, paint);
                renderer.cache();
                cache.mark_valid();
            }
            cache.draw();
        }
        else
            draw_focus_rect(ctx, rendering, rect);
    }
}

void clickable_panel::begin(
    ui_context& ctx, layout const& layout_spec,
    ui_flag_set flags, widget_id id)
{
    ALIA_GET_CACHED_DATA(clickable_panel_data)

    get_widget_id_if_needed(ctx, id);

    widget_state state = get_button_state(ctx, id, data.input);
    panel_.begin(ctx, text("clickable_panel"), layout_spec, flags, id, state);
    if ((flags & DISABLED) == 0)
    {
        clicked_ = do_button_input(ctx, id, data.input);
        if (is_render_pass(ctx) && (state & WIDGET_FOCUSED))
            draw_panel_focus_border(ctx, panel_, flags, data.rendering);
    }
    else
        clicked_ = false;
}

void scrollable_panel::begin(
    ui_context& ctx, getter<string> const& style,
    unsigned scrollable_axes,
    layout const& layout_spec, ui_flag_set flags)
{
    widget_id id = get_widget_id(ctx);
    outer_.begin(ctx, layout_spec);
    substyle_.begin(ctx, style, WIDGET_NORMAL);
    panel_data* data;
    get_cached_data(ctx, &data);
    begin_panel(ctx, outer_, id, flags, *data);
    region_.begin(ctx, GROW | UNPADDED, scrollable_axes, id);
    inner_.begin(ctx, (flags & HORIZONTAL) ? 0 : 1, GROW | PADDED);
}
void scrollable_panel::end()
{
    inner_.end();
    region_.end();
    substyle_.end();
    outer_.end();
}

// TREE NODE

struct tree_node_data
{
    bool expanded;
};

void tree_node::begin(
    ui_context& ctx,
    layout const& layout_spec,
    ui_flag_set flags,
    optional_storage<bool> const& expanded,
    widget_id expander_id)
{
    ctx_ = &ctx;

    tree_node_data* data;
    if (get_data(ctx, &data))
    {
        if (flags & INITIALLY_EXPANDED)
            data->expanded = true;
        else
            data->expanded = false;
    }

    accessor_mux<indirect_accessor<bool>,inout_accessor<bool> > state =
        resolve_storage(expanded, &data->expanded);

    grid_.begin(ctx, layout_spec);
    row_.begin(grid_);
    do_children_ = state.is_gettable() ? state.get() : false;
    expander_result_ =
        do_node_expander(ctx, state, default_layout, expander_id);
    label_region_.begin(ctx, layout(GROW));
}

void tree_node::end_header()
{
    label_region_.end();
}

bool tree_node::do_children()
{
    ui_context& ctx = *ctx_;
    end_header();
    row_.end();
    alia_if(do_children_)
    {
        row_.begin(grid_, layout(GROW));
        do_spacer(ctx);
        column_.begin(ctx, layout(GROW));
        return true;
    }
    alia_end
    return false;
}

void tree_node::end()
{
    column_.end();
    row_.end();
    grid_.end();
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

// DROP DOWNS

struct drop_down_button_renderer : dispatch_interface
{
    virtual layout_vector default_size(ui_context& ctx) const = 0;

    virtual void draw(
        ui_context& ctx, renderer_data_ptr& data_ptr, layout_box const& region,
        widget_state state) const = 0;
};

struct drop_down_button_data
{
    layout_leaf layout_node;
    themed_rendering_data<drop_down_button_renderer> rendering;
    button_input_state input;
};

struct default_drop_down_button_renderer : drop_down_button_renderer
{
    layout_vector default_size(ui_context& ctx) const
    {
        return resolve_layout_size(get_layout_traversal(ctx),
            size(1.f, 1.f, EM));
    }
    void draw(
        ui_context& ctx, renderer_data_ptr& data_ptr, layout_box const& region,
        widget_state state) const
    {
        caching_renderer_data* data;
        cast_data_ptr(&data, data_ptr);

        layout_vector const& padding_size = get_padding_size(ctx);
        layout_box padded_region = add_border(region, padding_size);
        layout_vector const& unpadded_size = region.size;

        caching_renderer cache(ctx, *data,
            combine_ids(ref(*ctx.style.id), make_id(state)),
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
            draw_rect(renderer.canvas(), paint, unpadded_size);

            renderer.canvas().translate(
                SkScalarDiv(
                    layout_scalar_as_skia_scalar(unpadded_size[0]),
                    SkIntToScalar(2)),
                SkScalarDiv(
                    layout_scalar_as_skia_scalar(unpadded_size[1]),
                    SkIntToScalar(2)));
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

static bool
do_drop_down_button(
    ui_context& ctx,
    layout const& layout_spec,
    widget_id id,
    drop_down_button_data& data)
{
    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        static default_drop_down_button_renderer default_renderer;
        refresh_themed_rendering_data(ctx, data.rendering, &default_renderer);
        drop_down_button_renderer const* renderer = data.rendering.renderer;
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
            data.layout_node.assignment().region,
            get_button_state(ctx, id, data.input));
        break;
      }

     case REGION_CATEGORY:
        do_box_region(ctx, id, data.layout_node.assignment().region);
        break;

     case INPUT_CATEGORY:
        add_to_focus_order(ctx, id);
        if (detect_click(ctx, id, LEFT_BUTTON) ||
            detect_keyboard_click(ctx, data.input.key, id, KEY_SPACE))
        {
            return true;
        }
        break;
    }

    return false;
}

struct ddl_data
{
    bool popup_open;

    // When the list is open, it may maintain a separate internal selection.
    // The internal selection can be copied into the actual control state
    // when the list is closed.
    optional<int> internal_selection;

    drop_down_button_data button;

    focus_rect_data focus_rendering;

    ddl_data() : popup_open(false) {}
};

// ddl_list_query_event is used to query the drop down list to determine how
// many items there are and which is selected.
struct ddl_list_query_event : ui_event
{
    ddl_list_query_event(widget_id target)
      : ui_event(NO_CATEGORY, CUSTOM_EVENT)
      , target(target)
      , total_items(0)
    {}
    widget_id target;
    // query results
    int total_items;
    optional<int> selected_index;
};

static optional<int> get_ddl_selected_index(ui_context& ctx, widget_id ddl_id)
{
    ddl_list_query_event event(ddl_id);
    issue_targeted_event(*ctx.system, event,
        make_routable_widget_id(ctx, ddl_id));
    return event.selected_index;
}

static int get_ddl_item_count(ui_context& ctx, widget_id ddl_id)
{
    ddl_list_query_event event(ddl_id);
    issue_targeted_event(*ctx.system, event,
        make_routable_widget_id(ctx, ddl_id));
    return event.total_items;
}

// ddl_select_index_event is used to select the item at the given index.
struct ddl_select_index_event : ui_event
{
    ddl_select_index_event(widget_id target, int index)
      : ui_event(NO_CATEGORY, CUSTOM_EVENT)
      , target(target)
      , index(index)
    {}
    widget_id target;
    int index;
};

static void select_ddl_item_at_index(ui_context& ctx, widget_id ddl_id,
    int index)
{
    ddl_select_index_event event(ddl_id, index);
    issue_targeted_event(*ctx.system, event,
        make_routable_widget_id(ctx, ddl_id));
}

static int clamp_ddl_index(ui_context& ctx, widget_id ddl_id, int index)
{
    int item_count = get_ddl_item_count(ctx, ddl_id);
    return clamp(index, 0, item_count > 0 ? item_count - 1 : 0);
}

static bool process_ddl_movement_keys(
    ui_context& ctx, widget_id ddl_id,
    optional<int>& selected_index, key_event_info const& info)
{
    if (info.mods == 0)
    {
        switch (info.code)
        {
         case KEY_UP:
            if (selected_index)
            {
                selected_index =
                    clamp_ddl_index(ctx, ddl_id, get(selected_index) - 1);
            }
            return true;
         case KEY_DOWN:
            selected_index = selected_index ?
                clamp_ddl_index(ctx, ddl_id, get(selected_index) + 1) : 0;
            return true;
         case KEY_PAGEUP:
            if (selected_index)
            {
                selected_index =
                    clamp_ddl_index(ctx, ddl_id, get(selected_index) - 10);
            }
            return true;
         case KEY_PAGEDOWN:
            selected_index = selected_index ?
                clamp_ddl_index(ctx, ddl_id, get(selected_index) + 10) : 0;
            return true;
         case KEY_HOME:
            selected_index = 0;
            return true;
         case KEY_END:
          {
            int item_count = get_ddl_item_count(ctx, ddl_id);
            selected_index = item_count > 0 ? item_count - 1 : 0;
            return true;
          }
        }
    }
    return false;
}

static void open_ddl(ui_context& ctx, ddl_data& data, widget_id id)
{
    data.internal_selection = get_ddl_selected_index(ctx, id);
    data.popup_open = true;
}

untyped_ui_value const*
untyped_drop_down_list::begin(ui_context& ctx, layout const& layout_spec,
    ui_flag_set flags)
{
    ctx_ = &ctx;

    do_list_ = make_selection_visible_ = false;
    list_index_ = 0;

    untyped_ui_value const* result = 0;

    id_ = get_widget_id(ctx);

    get_cached_data(ctx, &data_);
    ddl_data& data = *data_;

    widget_state state = get_button_state(ctx, id_, data.button.input);

    container_.begin(ctx, text("control"),
        add_default_padding(
            add_default_alignment(layout_spec, LEFT, BASELINE_Y), PADDED),
        HORIZONTAL | NO_INTERNAL_PADDING, id_, state);

    switch (ctx.event->category)
    {
     case RENDER_CATEGORY:
      {
        if ((state & WIDGET_FOCUSED))
        {
            draw_focus_rect(ctx, data.focus_rendering,
                container_.outer_region());
	}
        break;
      }

     case INPUT_CATEGORY:
      {
        key_event_info info;
        if (detect_key_press(ctx, &info, id_))
        {
            if (!data.popup_open)
            {
                // If this is a list of commands, don't select them without the
                // list being open.
                if (!(flags & COMMAND_LIST))
                {
                    optional<int> selection = get_ddl_selected_index(ctx, id_);
                    if (process_ddl_movement_keys(ctx, id_, selection, info))
                    {
                        acknowledge_input_event(ctx);
                        if (selection)
                            select_ddl_item_at_index(ctx, id_, get(selection));
                    }
                }
            }
            else
            {
                if (process_ddl_movement_keys(ctx, id_,
                        data.internal_selection, info))
                {
                    acknowledge_input_event(ctx);
                    make_selection_visible_ = true;
                }
            }
            if (info.mods == 0)
            {
                switch (info.code)
                {
                 case KEY_ENTER:
                 case KEY_NUMPAD_ENTER:
                    if (!data.popup_open)
                    {
                        open_ddl(ctx, *data_, id_);
                        make_selection_visible_ = true;
                        acknowledge_input_event(ctx);
                        break;
                    }
                 case KEY_SPACE:
                    if (data.popup_open)
                    {
                        if (data.internal_selection)
                        {
                            select_ddl_item_at_index(ctx, id_,
                                get(data.internal_selection));
                        }
                        data.popup_open = false;
                        acknowledge_input_event(ctx);
                    }
                    break;
                 case KEY_ESCAPE:
                    if (data.popup_open)
                    {
                        acknowledge_input_event(ctx);
                        data.popup_open = false;
                        acknowledge_input_event(ctx);
                    }
                    break;
                }
            }
        }
        break;
      }

     case NO_CATEGORY:
        switch (ctx.event->type)
        {
         case SET_VALUE_EVENT:
          {
            set_value_event& e = get_event<set_value_event>(ctx);
            if (e.target == id_)
            {
                result = e.value.get();
                data.popup_open = false;
            }
            break;
          }

         case CUSTOM_EVENT:
          {
            {
                ddl_list_query_event* query =
                    dynamic_cast<ddl_list_query_event*>(ctx.event);
                if (query && query->target == id_)
                {
                    do_list_ = true;
                }
            }
            {
                ddl_select_index_event* event =
                    dynamic_cast<ddl_select_index_event*>(ctx.event);
                if (event && event->target == id_)
                {
                    do_list_ = true;
                }
            }
            break;
          }
        }
        break;
    }

    if (data.popup_open)
        do_list_ = true;

    contents_.begin(ctx, BASELINE_Y | GROW_X | UNPADDED);

    return result;
}

bool untyped_drop_down_list::do_list()
{
    ui_context& ctx = *ctx_;
    ddl_data& data = *data_;

    contents_.end();

    if (do_drop_down_button(ctx, CENTER | UNPADDED, id_, data.button))
    {
        open_ddl(ctx, *data_, id_);
        make_selection_visible_ = true;
    }

    alia_if (do_list_)
    {
        layout_vector lower = container_.inner_region().corner;
        layout_vector upper = lower + container_.inner_region().size;
        popup_.begin(ctx, id_,
            make_vector(lower[0], upper[1]),
            make_vector(upper[0], lower[1]));
        if (popup_.user_closed())
            data.popup_open = false;

        list_border_.begin(ctx, GROW | PADDED);
        list_panel_.begin(ctx, text("control"), 2, GROW | UNPADDED);
    }
    alia_end

    return do_list_;
}

void untyped_drop_down_list::end()
{
    if (ctx_)
    {
        if (do_list_)
        {
            list_panel_.end();
            list_border_.end();
            popup_.end();
        }

        container_.end();

        ctx_ = 0;
    }
}

bool untyped_ddl_item::begin(untyped_drop_down_list& list, bool is_selected)
{
    list_ = &list;
    int index = list.list_index_++;
    ddl_data& data = *list.data_;

    bool is_internally_selected = data.internal_selection &&
        get(data.internal_selection) == index;

    ui_context& ctx = *list.ctx_;

    widget_id id = get_widget_id(ctx);
    panel_.begin(ctx, text("item"), UNPADDED, NO_CLICK_DETECTION,
        id, get_widget_state(ctx, id, true, false, is_internally_selected));

    if (list.make_selection_visible_ && is_internally_selected)
        make_widget_visible(ctx, id);

    switch (ctx.event->category)
    {
     case INPUT_CATEGORY:
        if (detect_click(ctx, id, LEFT_BUTTON))
            return true;
        break;

     case NO_CATEGORY:
        switch (ctx.event->type)
        {
         case CUSTOM_EVENT:
            {
                ddl_list_query_event* query =
                    dynamic_cast<ddl_list_query_event*>(ctx.event);
                if (query && query->target == list.id_)
                {
                    if (is_selected)
                        query->selected_index = index;
                    ++query->total_items;
                }
            }
            {
                ddl_select_index_event* event =
                    dynamic_cast<ddl_select_index_event*>(ctx.event);
                if (event && event->target == list.id_ &&
                    event->index == index)
                {
                    return true;
                }
            }
            break;
        }
        break;
    }

    return false;
}
void untyped_ddl_item::end()
{
    if (list_)
    {
        panel_.end();
        list_ = 0;
    }
}
void untyped_ddl_item::select(untyped_ui_value* value)
{
    set_value_event e(list_->id_, value);
    issue_targeted_event(*list_->ctx_->system, e,
        make_routable_widget_id(*list_->ctx_, list_->id_));
    end_pass(*list_->ctx_);
}

struct draggable_separator_data
{
    keyed_data<float> width;
    layout_leaf layout_node;
    caching_renderer_data rendering;
    int drag_start_delta;
};

bool do_draggable_separator(ui_context& ctx, accessor<int> const& width,
    layout const& layout_spec, ui_flag_set flags, widget_id id)
{
    ALIA_GET_CACHED_DATA(draggable_separator_data)

    get_widget_id_if_needed(ctx, id);

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

     case REGION_CATEGORY:
      {
        layout_box region = data.layout_node.assignment().region;
        int axis = (flags & HORIZONTAL) ? 0 : 1;
        int const drag_axis = 1 - axis;
        // Add a couple of pixels to make it easier to click on.
        region.corner[drag_axis] -= 1;
        region.size[drag_axis] += 2;
        do_box_region(ctx, id, region, flags & HORIZONTAL ?
            UP_DOWN_ARROW_CURSOR : LEFT_RIGHT_ARROW_CURSOR);
        break;
      }

     case INPUT_CATEGORY:
      {
        int axis = (flags & HORIZONTAL) ? 0 : 1;
        int const drag_axis = 1 - axis;

        if (detect_mouse_press(ctx, id, LEFT_BUTTON))
        {
            int position = get_integer_mouse_position(ctx)[drag_axis];
            int current_width = width.is_gettable() ? get(width) : 0;
            data.drag_start_delta = (flags & FLIPPED) ?
                current_width + position : position - current_width;
        }
        if (detect_drag(ctx, id, LEFT_BUTTON))
        {
            int position = get_integer_mouse_position(ctx)[drag_axis];
            set(width, (flags & FLIPPED) ?
                data.drag_start_delta - position :
                position - data.drag_start_delta);
            return true;
        }
        break;
      }
    }
    return false;
}

void resizable_content::begin(
    ui_context& ctx, accessor<int> const& size, ui_flag_set flags)
{
    ctx_ = &ctx;
    id_ = get_widget_id(ctx);
    flags_ = flags;
    size_ = get(size);

    if (flags & PREPEND)
        do_draggable_separator(ctx, size, UNPADDED, flags | FLIPPED);

    if (flags & HORIZONTAL)
        layout_.begin(ctx, 1, alia::height(float(size_), PIXELS));
    else
        layout_.begin(ctx, 0, alia::width(float(size_), PIXELS));

    // It's possible that the content will be too big for the requested size
    // and the layout engine will force the container to a larger size.
    // If this happens, we have to record that as the real size.
    if (detect_event(ctx, MOUSE_HIT_TEST_EVENT))
    {
        unsigned drag_axis = (flags & HORIZONTAL) ? 1 : 0;
        int new_size = layout_.region().size[drag_axis];
        if (new_size != size_)
            set(size, new_size);
    }

    handle_set_value_events(ctx, id_, size);
}
void resizable_content::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;
        layout_.end();
        if (!(flags_ & PREPEND) && !ctx.pass_aborted)
        {
            if (do_draggable_separator(ctx, inout(&size_), UNPADDED, flags_))
                issue_set_value_event(ctx, id_, size_);
        }
        ctx_ = 0;
    }
}

void popup::begin(ui_context& ctx, widget_id id,
    layout_vector const& lower_bound, layout_vector const& upper_bound)
{
    ctx_ = &ctx;
    id_ = id;

    // Update the context's layer Z so that the popup is above other content.
    set_layer_z(ctx, ctx.layer_z + 1);

    vector<2,int> absolute_lower = vector<2,int>(
        transform(get_transformation(ctx),
            vector<2,double>(lower_bound) + make_vector<double>(0.5, 0.5)));
    vector<2,int> absolute_upper = vector<2,int>(
        transform(get_transformation(ctx),
            vector<2,double>(upper_bound) + make_vector<double>(0.5, 0.5)));

    layout_vector surface_size = layout_vector(ctx.surface->size());
    layout_vector maximum_size;
    for (unsigned i = 0; i != 2; ++i)
    {
        maximum_size[i] =
            (std::max)(absolute_upper[i], surface_size[i] - absolute_lower[i]);
    }

    layout_.begin(ctx, maximum_size);

    if (!is_refresh_pass(ctx))
    {
        vector<2,int> position;
        for (unsigned i = 0; i != 2; ++i)
        {
            if (absolute_lower[i] + layout_.size()[i] <= surface_size[i] ||
                surface_size[i] - absolute_lower[i] > absolute_upper[i])
            {
                position[i] = lower_bound[i];
            }
            else
            {
                position[i] = upper_bound[i] - layout_.size()[i];
            }
        }
        transform_.begin(*get_layout_traversal(ctx).geometry);
        transform_.set(translation_matrix(vector<2,double>(position)));
    }

    // Intercept mouse clicks and wheel movement to other parts of the surface.
    background_id_ = get_widget_id(ctx);
    handle_mouse_hit(ctx, background_id_, HIT_TEST_MOUSE | HIT_TEST_WHEEL);
}
bool popup::user_closed()
{
    ui_context& ctx = *ctx_;
    return
        detect_mouse_press(ctx, background_id_, LEFT_BUTTON) || 
        detect_mouse_press(ctx, background_id_, MIDDLE_BUTTON) || 
        detect_mouse_press(ctx, background_id_, RIGHT_BUTTON) ||
        detect_focus_loss(ctx, id_);
}
void popup::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;

        transform_.end();
	layout_.end();

        // Restore the context's layer Z.
	set_layer_z(ctx, ctx.layer_z - 1);

	ctx_ = 0;
    }
}

}
