#include <alia/ui_library.hpp>
#include <alia/ui_utilities.hpp>
#include <alia/skia.hpp>
#include <alia/ui_system.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <alia/native/font_provider.hpp>
#include <alia/ascii_text_renderer.hpp>
#include <alia/cached_ascii_text.hpp>
#include <utility>
#include <cctype>

namespace alia {

template<class T>
void set_new_value(accessor<T> const& accessor, control_result<T>& result,
    T const& new_value)
{
    set(accessor, new_value);
    result.new_value = new_value;
    result.changed = true;
}

struct simple_display_data
{
    layout_leaf layout_node;
    caching_renderer_data rendering;
};

struct boolean_widget_renderer : dispatch_interface
{
    virtual layout_vector default_size(ui_context& ctx) const = 0;

    virtual void draw(
        ui_context& ctx, renderer_data_ptr& data_ptr, layout_box const& region,
        getter<bool> const& value, widget_state state) const = 0;
};

// FOCUS

static void setup_focus_drawing(ui_context& ctx, SkPaint& paint)
{
    paint.setStrokeWidth(SkScalar(ctx.layout.style_info->padding_size[0]) *
        SkScalar(0.7));
    paint.setStyle(SkPaint::kStroke_Style);
    set_color(paint, get_color_property(ctx, "focus_color"));
}

static void draw_round_focus_rect(ui_context& ctx, SkCanvas& canvas,
    vector<2,int> const& size)
{
    SkPaint paint;
    paint.setFlags(SkPaint::kAntiAlias_Flag);
    setup_focus_drawing(ctx, paint);
    draw_round_rect(canvas, paint, size);
}

static void draw_focus_rect(ui_context& ctx, SkCanvas& canvas,
    vector<2,int> const& size)
{
    SkPaint paint;
    paint.setFlags(SkPaint::kAntiAlias_Flag);
    setup_focus_drawing(ctx, paint);
    paint.setStrokeJoin(SkPaint::kRound_Join);
    draw_rect(canvas, paint, size);
}

typedef caching_renderer_data focus_rect_data;

static void draw_focus_rect(ui_context& ctx, focus_rect_data& data,
    box<2,int> const& content_region)
{
    int padding = ctx.layout.style_info->padding_size[0];
    box<2,int> rect = add_border(content_region, padding / 2);
    box<2,int> padded_region = add_border(rect, padding);
    caching_renderer cache(ctx, data, *ctx.style.id, padded_region);
    if (cache.needs_rendering())
    {
        skia_renderer renderer(ctx, cache.image(), padded_region.size);
        renderer.canvas().translate(SkScalar(padding), SkScalar(padding));
        draw_focus_rect(ctx, renderer.canvas(), rect.size);
        renderer.cache();
        cache.mark_valid();
    }
    cache.draw();
}

// SEPARATOR

struct separator_data
{
    keyed_data<float> width;
    layout_leaf layout_node;
    caching_renderer_data rendering;
};

void do_separator(ui_context& ctx, layout const& layout_spec)
{
    separator_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        refresh_keyed_data(data->width, *ctx.style.id);
        if (!data->width.is_valid)
            set(data->width, get_float_property(ctx, "separator_width", 2));
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec,
            leaf_layout_requirements(
                make_layout_vector(
                    as_layout_size(data->width.value),
                    as_layout_size(data->width.value)),
                0, 0),
            FILL | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        if (!is_rendering_active(ctx))
            break;
        layout_box const& region = data->layout_node.assignment().region;
        caching_renderer cache(ctx, data->rendering, *ctx.style.id, region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), region.size);
            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);
            paint.setStrokeWidth(2);
            paint.setStrokeCap(SkPaint::kRound_Cap);
            set_color(paint, get_color_property(ctx, "separator_color"));
            renderer.canvas().drawLine(
                SkScalar(1), SkScalar(1),
                SkScalar(region.size[0] - 1), SkScalar(region.size[1] - 1),
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
    simple_display_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx),
            add_default_size(layout_spec, size(1.4f, 1.4f, EM)),
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        if (!is_rendering_active(ctx))
            break;
        layout_box const& region = data->layout_node.assignment().region;
        caching_renderer cache(ctx, data->rendering, color.id(), region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), region.size);
            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);
            paint.setStyle(SkPaint::kFill_Style);
            set_color(paint, get(color));
            SkRect rect;
            rect.fLeft = 0;
            rect.fRight = SkScalar(region.size[0]);
            rect.fTop = 0;
            rect.fBottom = SkScalar(region.size[1]);
            SkScalar radius =
                SkScalar((std::min)(region.size[0], region.size[1])) / 3;
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
    simple_display_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        layout_scalar size =
            resolve_layout_height(get_layout_traversal(ctx), 1, EX);
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec,
            leaf_layout_requirements(
                make_layout_vector(size, size),
                size, 0),
            CENTER_X | BASELINE_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        if (!is_rendering_active(ctx))
            break;
        layout_box const& region = data->layout_node.assignment().region;
        caching_renderer cache(ctx, data->rendering, *ctx.style.id, region);
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

        layout_vector const& padding_size =
            ctx.layout.style_info->padding_size;
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

            renderer.canvas().translate(SkScalar(padding_size[0]),
                SkScalar(padding_size[1]));

            paint.setStyle(SkPaint::kFill_Style);
            set_color(paint, bg_color);
            draw_round_rect(renderer.canvas(), paint, unpadded_size);

            if (value.is_gettable() && get(value))
            {
                set_color(paint, fg_color);
                paint.setStrokeCap(SkPaint::kRound_Cap);
                paint.setStrokeJoin(SkPaint::kRound_Join);
                SkScalar dx = SkScalar(unpadded_size[0]) / SkScalar(10.);
                SkScalar dy = SkScalar(unpadded_size[1]) / SkScalar(10.);
                paint.setStrokeWidth(dx * SkScalar(1.6));
                renderer.canvas().drawLine(
                    dx * 3, dy * 3, dx * 7, dy * 7, paint);
                renderer.canvas().drawLine(
                    dx * 3, dy * 7, dx * 7, dy * 3, paint);
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
    get_widget_id_if_needed(ctx, id);

    check_box_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        static default_check_box_renderer default_renderer;
        refresh_themed_rendering_data(ctx, data->rendering, &default_renderer);
        check_box_renderer const* renderer = data->rendering.renderer;
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(renderer->default_size(ctx), 0, 0),
            LEFT | CENTER_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        //refresh_focus(ctx, id);
        break;
      }

     case RENDER_CATEGORY:
      {
        if (!is_rendering_active(ctx))
            break;
        data->rendering.renderer->draw(ctx, data->rendering.data,
            data->layout_node.assignment().region, value,
            get_button_state(ctx, id, data->input));
        break;
      }

     case REGION_CATEGORY:
        do_box_region(ctx, id, data->layout_node.assignment().region);
        break;

     case INPUT_CATEGORY:
        if (do_button_input(ctx, id, data->input))
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

        layout_vector const& padding_size =
            ctx.layout.style_info->padding_size;
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
                SkScalar(padding_size[0]), SkScalar(padding_size[1]));

            double scale = (unpadded_size[0] + unpadded_size[1]) / 20.;
            layout_vector center = unpadded_size / 2;

            if (state & WIDGET_FOCUSED)
            {
                set_color(paint, get_color_property(ctx, "focus_color"));
                paint.setStyle(SkPaint::kFill_Style);
                renderer.canvas().drawCircle(
                    SkScalar(center[0]), SkScalar(center[1]),
                    SkScalar(6.4 * scale), paint);
            }

            if (bg_color.a)
            {
                set_color(paint, bg_color);
                paint.setStyle(SkPaint::kFill_Style);
                renderer.canvas().drawCircle(
                    SkScalar(center[0]), SkScalar(center[1]),
                    SkScalar(5 * scale), paint);
            }

            if (value.is_gettable() && get(value))
            {
                set_color(paint, fg_color);
                paint.setStyle(SkPaint::kFill_Style);
                renderer.canvas().drawCircle(
                    SkScalar(center[0]), SkScalar(center[1]),
                    SkScalar(2.5 * scale), paint);
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
    get_widget_id_if_needed(ctx, id);

    radio_button_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        static default_radio_button_renderer default_renderer;
        refresh_themed_rendering_data(ctx, data->rendering, &default_renderer);
        radio_button_renderer const* renderer = data->rendering.renderer;
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(renderer->default_size(ctx), 0, 0),
            LEFT | CENTER_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        //refresh_focus(ctx, id);
        break;
      }

     case RENDER_CATEGORY:
      {
        if (!is_rendering_active(ctx))
            break;
        data->rendering.renderer->draw(ctx, data->rendering.data,
            data->layout_node.assignment().region, value,
            get_button_state(ctx, id, data->input));
        break;
      }

     case REGION_CATEGORY:
        do_box_region(ctx, id, data->layout_node.assignment().region);
        break;

     case INPUT_CATEGORY:
        if (do_button_input(ctx, id, data->input))
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

        layout_vector const& padding_size =
            ctx.layout.style_info->padding_size;
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
                SkScalar(padding_size[0]),
                SkScalar(padding_size[1]));

            if ((state & WIDGET_PRIMARY_STATE_MASK) != WIDGET_NORMAL)
            {
                paint.setStyle(SkPaint::kFill_Style);
                set_color(paint, bg_color);
                draw_round_rect(renderer.canvas(), paint, unpadded_size);
            }

            if (state & WIDGET_FOCUSED)
                draw_round_focus_rect(ctx, renderer.canvas(), unpadded_size);

            renderer.canvas().translate(
                SkScalar(unpadded_size[0] / 2),
                SkScalar(unpadded_size[1] / 2));

            if (value.is_gettable() && get(value))
                renderer.canvas().rotate(90);

            {
                set_color(paint, fg_color);
                paint.setStyle(SkPaint::kFill_Style);
                SkScalar a = SkScalar(unpadded_size[0]) / SkScalar(1.8);
                SkPath path;
                path.incReserve(4);
                SkPoint p0;
                p0.fX = a * SkScalar(-0.34);
                p0.fY = a * SkScalar(-0.5);
                path.moveTo(p0);
                SkPoint p1;
                p1.fX = p0.fX;
                p1.fY = a * SkScalar(0.5);
                path.lineTo(p1);
                SkPoint p2;
                p2.fX = p0.fX + a * SkScalar(0.866);
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
    get_widget_id_if_needed(ctx, id);

    node_expander_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        static default_node_expander_renderer default_renderer;
        refresh_themed_rendering_data(ctx, data->rendering, &default_renderer);
        node_expander_renderer const* renderer = data->rendering.renderer;
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(renderer->default_size(ctx), 0, 0),
            LEFT | CENTER_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        //refresh_focus(ctx, id);
        break;
      }

     case RENDER_CATEGORY:
      {
        if (!is_rendering_active(ctx))
            break;
        data->rendering.renderer->draw(ctx, data->rendering.data,
            data->layout_node.assignment().region, value,
            get_button_state(ctx, id, data->input));
        break;
      }

     case REGION_CATEGORY:
        do_box_region(ctx, id, data->layout_node.assignment().region);
        break;

     case INPUT_CATEGORY:
        if (do_button_input(ctx, id, data->input))
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
    simple_display_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx),
            // Note that this is effectively a default minimum width, since
            // the true default behavior is to fill the allotted width.
            add_default_size(layout_spec, size(10.f, 1.4f, EM)),
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0),
            FILL_X | TOP | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        if (!is_rendering_active(ctx))
            break;
        layout_box const& region = data->layout_node.assignment().region;
        caching_renderer cache(ctx, data->rendering, progress.id(), region);
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

            SkScalar trim = SkScalar(region.size[1] / 12.);
            box<2,SkScalar> full_box(
                make_vector<SkScalar>(0, 0),
                make_vector<SkScalar>(
                    SkScalar(region.size[0]),
                    SkScalar(region.size[1])));

            set_color(paint, outline_color);
            draw_rect(sr.canvas(), paint, full_box);

            set_color(paint, bg_color);
            draw_rect(sr.canvas(), paint, add_border(full_box, -trim));

            box<2,SkScalar> bar_box = add_border(full_box, -trim * 2);
            bar_box.size[0] = SkScalar(bar_box.size[0] *
                (progress.is_gettable() ? get(progress) : 0));
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

// TEXT

ascii_font_image const*
get_ascii_font_image(font const& font, rgba8 text_color, rgba8 bg_color)
{
    // TODO: GC font images that aren't in active use.
    typedef std::map<std::pair<alia::font,std::pair<rgba8,rgba8> >,
        alia__shared_ptr<ascii_font_image> > image_map_type;
    static image_map_type cached_font_images;
    ascii_font_image* image;
    image_map_type::iterator i = cached_font_images.find(
        std::make_pair(font, std::make_pair(text_color, bg_color)));
    if (i == cached_font_images.end())
    {
        image = new ascii_font_image;
        native::create_ascii_font_image(image, font, text_color, bg_color);
        cached_font_images[
            std::make_pair(font, std::make_pair(text_color, bg_color))].
            reset(image);
    }
    else
        image = i->second.get();
    return image;
}

static ascii_font_image const*
get_font_image_for_active_style(ui_context& ctx)
{
    return get_ascii_font_image(ctx.style.properties->font,
        ctx.style.properties->text_color, ctx.style.properties->bg_color);
}

struct keyed_cached_image
{
    cached_image_ptr img;
    owned_id key;
};

struct text_display_data;

struct text_layout_node : layout_node
{
    // implementation of normal layout interface
    layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx);
    layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_scalar assigned_width);
    void set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment);

    // implementation of wrapping layout interface
    layout_requirements get_minimal_horizontal_requirements(
        layout_calculation_context& ctx);
    void calculate_wrapping(
        layout_calculation_context& ctx,
        layout_scalar assigned_width,
        wrapping_state& state);
    void assign_wrapped_regions(
        layout_calculation_context& ctx,
        layout_scalar assigned_width,
        wrapping_assignment_state& state);

    text_display_data* data;
};

enum text_image_state
{
    INVALID_IMAGE,
    UNWRAPPED_IMAGE,
    WRAPPED_IMAGE
};

struct text_display_data
{
    alia::font font;
    rgba8 text_color, bg_color;

    ascii_font_image const* font_image;

    owned_id text_id;
    bool text_valid;
    string text;

    layout layout_spec;
    resolved_layout_spec resolved_spec;
    text_layout_node layout_node;

    owned_id style_id;

    image<rgba8> text_image;
    text_image_state image_state;
    cached_image_ptr cached_image;
    bool image_cached;
    vector<2,int> image_position;

    int wrapped_rows;
};

layout_requirements text_layout_node::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    // If this gets called, we know that we're not wrapping, so just compute
    // the image of the text unwrapped and use that from here on out.

    if (data->image_state != UNWRAPPED_IMAGE)
    {
        ascii_text_renderer renderer(*data->font_image);
        create_image(data->text_image,
            make_vector<int>(
                renderer.measure_text(as_utf8_string(data->text)),
                data->font_image->metrics.ascent +
                data->font_image->metrics.descent));
        renderer.render_text(data->text_image.view, as_utf8_string(data->text),
            data->text_color, data->bg_color);
        data->image_cached = false;
        data->image_state = UNWRAPPED_IMAGE;
    }

    layout_requirements requirements;
    resolve_requirements(
        requirements, data->resolved_spec, 0,
        calculated_layout_requirements(data->text_image.view.size[0], 0, 0));
    return requirements;
}
layout_requirements text_layout_node::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_scalar assigned_width)
{
    layout_requirements requirements;
    resolve_requirements(
        requirements, data->resolved_spec, 1,
        calculated_layout_requirements(
            0,
            as_layout_size(data->font_image->metrics.ascent),
            as_layout_size(data->font_image->metrics.descent)));
    return requirements;
}
void text_layout_node::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    assert(data->image_state == UNWRAPPED_IMAGE);
    layout_requirements horizontal_requirements, vertical_requirements;
    resolve_requirements(
        horizontal_requirements, data->resolved_spec, 0,
        calculated_layout_requirements(data->text_image.view.size[0], 0, 0));
    resolve_requirements(
        vertical_requirements, data->resolved_spec, 1,
        calculated_layout_requirements(
            as_layout_size(data->font_image->metrics.ascent +
                data->font_image->metrics.descent),
            as_layout_size(data->font_image->metrics.ascent),
            as_layout_size(data->font_image->metrics.descent)));
    relative_layout_assignment relative_assignment;
    resolve_relative_assignment(relative_assignment, data->resolved_spec,
        assignment, horizontal_requirements, vertical_requirements);
    data->image_position = relative_assignment.region.corner;
}

layout_requirements text_layout_node::get_minimal_horizontal_requirements(
    layout_calculation_context& ctx)
{
    return layout_requirements(
        data->font_image->metrics.average_width * 10, 0, 0, 0);
}
void text_layout_node::calculate_wrapping(
    layout_calculation_context& ctx,
    layout_scalar assigned_width,
    wrapping_state& state)
{
    int padding_width = (std::max)(
        data->font_image->metrics.overhang * 2,
        data->font_image->advance[' ']);
    int usable_width = assigned_width - padding_width;
    data->wrapped_rows = 0;
    utf8_string text = as_utf8_string(data->text);
    if (!is_empty(text))
    {
        layout_requirements y_requirements(
            0,
            as_layout_size(data->font_image->metrics.ascent),
            as_layout_size(data->font_image->metrics.descent),
            0);
        ascii_text_renderer renderer(*data->font_image);
        utf8_ptr p = text.begin;
        while (1)
        {
            fold_in_requirements(state.active_row.requirements,
                y_requirements);
            utf8_ptr line_end =
                renderer.break_text(
                    utf8_string(p, text.end),
                    usable_width - state.accumulated_width,
                    state.accumulated_width == 0);
            state.accumulated_width +=
                renderer.compute_advance(utf8_string(p, line_end)) +
                padding_width;
            ++data->wrapped_rows;
            if (line_end == text.end)
                break;
            p = line_end;
            wrap_row(state);
        }
    }
}
void text_layout_node::assign_wrapped_regions(
    layout_calculation_context& ctx,
    layout_scalar assigned_width,
    wrapping_assignment_state& state)
{
    utf8_string text = as_utf8_string(data->text);

    int padding_width = (std::max)(
        data->font_image->metrics.overhang * 2,
        data->font_image->advance[' ']);
    int usable_width = assigned_width - padding_width;

    int total_height = 0;
    std::vector<wrapped_row>::const_iterator row_i = state.active_row;
    for (int i = 0; i != data->wrapped_rows; ++i)
    {
        total_height += row_i->requirements.minimum_size;
        ++row_i;
    }

    ascii_text_renderer renderer(*data->font_image);
    create_image(data->text_image,
        make_vector<int>(assigned_width, total_height));
    alia_foreach_pixel(data->text_image.view, rgba8, i, i = rgba8(0, 0, 0, 0));
    data->image_cached = false;
    data->image_state = UNWRAPPED_IMAGE;

    data->image_position = make_vector<int>(0, state.active_row->y);

    if (!is_empty(text))
    {
        ascii_text_renderer renderer(*data->font_image);
        utf8_ptr p = text.begin;
        int y = 0;
        while (1)
        {
            utf8_ptr line_end =
                renderer.break_text(
                    utf8_string(p, text.end),
                    usable_width - state.x,
                    state.x == 0);
            if (line_end != p)
            {
                renderer.render_text(
                    subimage(data->text_image.view, box<2,int>(
                        make_vector<int>(state.x,
                            y + state.active_row->requirements.minimum_ascent -
                                data->font_image->metrics.ascent),
                        make_vector<int>(assigned_width - state.x,
                            data->font_image->metrics.ascent +
                            data->font_image->metrics.descent))),
                    utf8_string(p, line_end),
                    data->text_color, data->bg_color);
                state.x += renderer.compute_advance(utf8_string(p, line_end)) +
                    padding_width;
            }
            if (line_end == text.end)
                break;
            p = line_end;
            y += state.active_row->requirements.minimum_size;
            wrap_row(state);
        }
    }
}

template<class T>
bool float_from_string(T* value, string const& str, string* message)
{
    try
    {
        *value = boost::lexical_cast<T>(str);
        return true;
    }
    catch (boost::bad_lexical_cast&)
    {
        *message = "This input expects a number.";
        return false;
    }
}

#define ALIA_FLOAT_CONVERSIONS(T) \
    static inline bool from_string( \
        T* value, string const& str, string* message) \
    { return float_from_string(value, str, message); } \
    string to_string(T value) \
    { return str(boost::format("%s") % value); }

ALIA_FLOAT_CONVERSIONS(float)
ALIA_FLOAT_CONVERSIONS(double)

template<class T>
bool integer_from_string(T* value, string const& str, string* message)
{
    try
    {
        long long n = boost::lexical_cast<long long>(str);
        *value = boost::numeric_cast<T>(n);
        return true;
    }
    catch (boost::bad_lexical_cast&)
    {
        *message = "This input expects an integer.";
        return false;
    }
    catch (boost::bad_numeric_cast&)
    {
        *message = "integer out of range";
        return false;
    }
}

#define ALIA_INTEGER_CONVERSIONS(T) \
    bool from_string(T* value, string const& str, string* message) \
    { return integer_from_string(value, str, message); } \
    string to_string(T value) \
    { return boost::lexical_cast<string>(value); }

//ALIA_INTEGER_CONVERSIONS(int64)
//ALIA_INTEGER_CONVERSIONS(uint64)
//ALIA_INTEGER_CONVERSIONS(int32)
//ALIA_INTEGER_CONVERSIONS(uint32)
//ALIA_INTEGER_CONVERSIONS(int16)
//ALIA_INTEGER_CONVERSIONS(uint16)
//ALIA_INTEGER_CONVERSIONS(int8)
//ALIA_INTEGER_CONVERSIONS(uint8_t)
ALIA_INTEGER_CONVERSIONS(int)
ALIA_INTEGER_CONVERSIONS(unsigned)

void do_text(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec)
{
    text_display_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        if (!data->text_id.matches(text.id()) ||
            !data->text_valid && text.is_gettable() ||
            !data->style_id.matches(*ctx.style.id) ||
            data->layout_spec != layout_spec)
        {
            record_layout_change(get_layout_traversal(ctx));
            data->image_state = INVALID_IMAGE;
            data->layout_node.data = data;
            data->font = ctx.style.properties->font;
            data->text_color = ctx.style.properties->text_color;
            data->bg_color = ctx.style.properties->bg_color;
            data->font_image = get_ascii_font_image(
                data->font, data->text_color, data->bg_color);
            resolve_layout_spec(get_layout_traversal(ctx), data->resolved_spec,
                layout_spec, LEFT | BASELINE_Y);
            data->layout_spec = layout_spec;
            data->text_valid = text.is_gettable();
            data->text = data->text_valid ? get(text) : "";
            data->text_id.store(text.id());
            data->style_id.store(*ctx.style.id);
            data->image_cached = false;
        }
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        break;

     case RENDER_CATEGORY:
        if (!is_rendering_active(ctx))
            break;
        if (data->text_image.view.pixels)
        {
            if (!data->image_cached || !is_valid(data->cached_image))
            {
                ctx.surface->cache_image(data->cached_image,
                    make_interface(data->text_image.view));
                data->image_cached = true;
            }
            data->cached_image->draw(vector<2,double>(data->image_position));
        }
        break;
    }
}

void do_number(ui_context& ctx, char const* format,
    getter<double> const& number, layout const& layout_spec)
{
    cached_string_conversion* cache;
    if (get_data(ctx, &cache) || cache->id.matches(number.id()))
    {
        cache->text = str(boost::format(format) % get(number));
        cache->id.store(number.id());
    }
    do_text(ctx, cached_string_conversion_accessor(cache), layout_spec);
}

void do_paragraph(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec)
{
    flow_layout f(ctx, add_default_padding(layout_spec, PADDED));
    do_text(ctx, text);
}

struct standalone_text_data
{
    owned_id key;

    layout_leaf layout_node;
    leaf_layout_requirements layout_requirements;

    cached_image_ptr cached_image;
};

static void refresh_standalone_text(
    ui_context& ctx,
    standalone_text_data& data,
    getter<string> const& text,
    layout const& layout_spec)
{
    if (!data.key.matches(combine_ids(ref(text.id()), ref(*ctx.style.id))))
    {
        ascii_font_image const* font_image =
            get_font_image_for_active_style(ctx);
        ascii_text_renderer renderer(*font_image);

        vector<2,int> image_size = make_vector<int>(
            renderer.measure_text(as_utf8_string(get(text))),
            as_layout_size(font_image->metrics.ascent +
                font_image->metrics.descent));

        data.layout_requirements =
            leaf_layout_requirements(
                make_layout_vector(image_size[0], image_size[1]),
                as_layout_size(font_image->metrics.ascent),
                as_layout_size(font_image->metrics.descent));

        data.cached_image.reset();

        data.layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec, data.layout_requirements,
            LEFT | BASELINE_Y);

        data.key.store(combine_ids(ref(text.id()), ref(*ctx.style.id)));
    }
    add_layout_node(get_layout_traversal(ctx), &data.layout_node);
}

static box<2,int> get_region(standalone_text_data& data)
{
    return data.layout_node.assignment().region;
}

static void render_standalone_text(
    ui_context& ctx,
    standalone_text_data& data,
    getter<string> const& text)
{
    if (!is_valid(data.cached_image))
    {
        ascii_font_image const* font_image =
            get_font_image_for_active_style(ctx);
        ascii_text_renderer renderer(*font_image);
        image<rgba8> text_image;
        create_image(text_image, data.layout_requirements.size);
        renderer.render_text(
            text_image.view, as_utf8_string(get(text)),
            ctx.style.properties->text_color,
            ctx.style.properties->bg_color);
        ctx.surface->cache_image(data.cached_image,
            make_interface(text_image.view));
    }
    data.cached_image->draw(vector<2,double>(get_region(data).corner));
}

void do_label(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec)
{
    standalone_text_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        refresh_standalone_text(ctx, *data, text, layout_spec);
        break;
     case RENDER_CATEGORY:
        if (!is_rendering_active(ctx))
            break;
        render_standalone_text(ctx, *data, text);
        break;
    }
}

// LINK

struct link_data
{
    button_input_state input;
    standalone_text_data standalone_text;
    focus_rect_data focus_rect;
};

bool do_link(
    ui_context& ctx,
    getter<string> const& text,
    layout const& layout_spec,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);

    link_data* data;
    // Technically, the key_state field is state, but it only needs to persist
    // while the user is directly interacting with the link, so it's fine to
    // just call everything cached data.
    get_cached_data(ctx, &data);

    widget_state state = get_button_state(ctx, id, data->input);
    scoped_substyle substyle(ctx, const_text("link"), state);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        refresh_standalone_text(ctx, data->standalone_text, text, layout_spec);
        //refresh_focus(ctx, id);
        break;

     case REGION_CATEGORY:
        do_box_region(ctx, id, get_region(data->standalone_text), HAND_CURSOR);
        break;

     case RENDER_CATEGORY:
        if (!is_rendering_active(ctx))
            break;
        render_standalone_text(ctx, data->standalone_text, text);
        if (state & WIDGET_FOCUSED)
        {
            // TODO: This should be a little less hackish, but that might
            // require more information about the font metrics.
            box<2,int> const& ar = get_region(data->standalone_text);
            box<2,int> r;
            r.corner[0] = ar.corner[0] - 1;
            r.corner[1] = ar.corner[1];
            r.size[0] = ar.size[0] + 3;
            r.size[1] = ar.size[1] + 1;
            draw_focus_rect(ctx, data->focus_rect, r);
        }
        break;

     case INPUT_CATEGORY:
        return do_button_input(ctx, id, data->input);
    }

    return false;
}

// BORDERED BOX

void bordered_box::begin(
    ui_context& ctx, layout const& layout_spec, ui_flag_set flags)
{
    box_.begin(ctx, (flags & HORIZONTAL) ? 0 : 1, layout_spec);

    if (is_rendering(ctx))
    {
        rgba8 const& color = ctx.style.properties->border_color;
        layout_box box_with_border =
            add_border(box_.region(), ctx.layout.style_info->padding_size);
        layout_vector poly[4];
        make_polygon(poly, box_with_border);
        ctx.surface->draw_filled_polygon(color, poly, 4);
    }

    //const_cast<layout_vector&>(ctx.layout.style_info->padding_size) =
    //    padding_size;
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
    ui_context& ctx, column_layout& outer, widget_id id, ui_flag_set flags)
{
    panel_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        //refresh_focus(ctx, id);
        break;

     case RENDER_CATEGORY:
      {
        if (!is_rendering_active(ctx))
            break;
        if (flags & ROUNDED)
        {
            layout_box const& rect = outer.region();
            caching_renderer cache(ctx, data->rendering, *ctx.style.id, rect);
            if (cache.needs_rendering())
            {
                int padding = ctx.layout.style_info->padding_size[0];
                skia_renderer renderer(ctx, cache.image(), rect.size);
                SkPaint paint;
                paint.setFlags(SkPaint::kAntiAlias_Flag);
                set_color(paint, ctx.style.properties->bg_color);
                SkRect sr;
                sr.fLeft = 0;
                sr.fRight = SkScalar(rect.size[0]);
                sr.fTop = 0;
                sr.fBottom = SkScalar(rect.size[1]);
                SkScalar radius = SkScalar(padding) * 2;
                renderer.canvas().drawRoundRect(sr, radius, radius, paint);
                renderer.cache();
                cache.mark_valid();
            }
            cache.draw();
        }
        else
        {
            layout_vector poly[4];
            make_polygon(poly, outer.region());
            ctx.surface->draw_filled_polygon(
                ctx.style.properties->bg_color, poly, 4);
        }
        break;
      }

     case REGION_CATEGORY:
        // So the panel will block mouse events on things behind it.
        do_box_region(ctx, id, outer.region());
        break;

     case INPUT_CATEGORY:
        // So the panel will steal the focus if clicked on.
        if (ctx.event->type == MOUSE_PRESS_EVENT && is_region_hot(ctx, id))
            set_focus(ctx, id);
        break;
    }
}

void panel::begin(
    ui_context& ctx, getter<string> const& style,
    layout const& layout_spec, ui_flag_set flags, widget_id id,
    widget_state state)
{
    ctx_ = &ctx;
    get_widget_id_if_needed(ctx, id);
    outer_.begin(ctx, add_default_padding(layout_spec, PADDED));
    substyle_.begin(ctx, style, state);
    begin_panel(ctx, outer_, id, flags);
    inner_.begin(ctx, (flags & HORIZONTAL) ? 0 : 1, GROW | PADDED);
}
void panel::end()
{
    inner_.end();
    substyle_.end();
    outer_.end();
}

layout_box panel::outer_region() const
{
    // outer_.region() returns its region in its own frame of reference, which
    // isn't valid for panel users, so this is used instead.
    return add_border(inner_.region(), ctx_->layout.style_info->padding_size);
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
        layout_box rect = add_border(p.inner_region(),
            ctx.layout.style_info->padding_size);
        if (flags & ROUNDED)
        {
            caching_renderer cache(ctx, rendering, *ctx.style.id,
                add_border(rect, ctx.layout.style_info->padding_size));
            if (cache.needs_rendering())
            {
                int padding = ctx.layout.style_info->padding_size[0];
                skia_renderer renderer(ctx, cache.image(),
                    rect.size + ctx.layout.style_info->padding_size * 2);
                SkPaint paint;
                paint.setFlags(SkPaint::kAntiAlias_Flag);
                setup_focus_drawing(ctx, paint);
                renderer.canvas().translate(
                    SkScalar(ctx.layout.style_info->padding_size[0]),
                    SkScalar(ctx.layout.style_info->padding_size[1]));
                SkRect sr;
                sr.fLeft = 0;
                sr.fRight = SkScalar(rect.size[0]);
                sr.fTop = 0;
                sr.fBottom = SkScalar(rect.size[1]);
                SkScalar radius = SkScalar(padding) * 2;
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
    get_widget_id_if_needed(ctx, id);

    clickable_panel_data* data;
    get_data(ctx, &data);

    widget_state state = get_button_state(ctx, id, data->input);
    panel_.begin(ctx, const_text("clickable_panel"), layout_spec, flags, id,
        state);
    if ((flags & DISABLED) == 0)
    {
        clicked_ = do_button_input(ctx, id, data->input);
        if (is_rendering(ctx) && (state & WIDGET_FOCUSED))
            draw_panel_focus_border(ctx, panel_, flags, data->rendering);
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
    begin_panel(ctx, outer_, id, flags);
    region_.begin(ctx, GROW | UNPADDED, scrollable_axes, id);
    inner_.begin(ctx, GROW | PADDED);
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
    end_header();
    row_.end();
    alia_if_(*ctx_, do_children_)
    {
        row_.begin(grid_, layout(GROW));
        do_spacer(*ctx_);
        column_.begin(*ctx_, layout(GROW));
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
    getter<string> const& text,
    layout const& layout_spec,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    button_data* data;
    get_data(ctx, &data);
    widget_state state = get_button_state(ctx, id, data->input);
    panel p(ctx, const_text("button"),
        add_default_alignment(layout_spec, LEFT, TOP), NO_FLAGS, id, state);
    do_text(ctx, text, CENTER);
    if (is_rendering(ctx) && (state & WIDGET_FOCUSED))
    {
        draw_focus_rect(ctx, data->focus_rect, add_border(p.inner_region(),
            ctx.layout.style_info->padding_size));
    }
    return do_button_input(ctx, id, data->input);
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

        layout_vector const& padding_size =
            ctx.layout.style_info->padding_size;
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
                SkScalar(padding_size[0]),
                SkScalar(padding_size[1]));

            if ((state & WIDGET_PRIMARY_STATE_MASK) != WIDGET_NORMAL)
            {
                paint.setStyle(SkPaint::kFill_Style);
                set_color(paint, bg_color);
                draw_round_rect(renderer.canvas(), paint, unpadded_size);
            }

            if (state & WIDGET_FOCUSED)
                draw_round_focus_rect(ctx, renderer.canvas(), unpadded_size);

            renderer.canvas().translate(
                SkScalar(unpadded_size[0] / 2),
                SkScalar(unpadded_size[1] / 2));

            set_color(paint, fg_color);

            switch (icon)
            {
             case REMOVE_ICON:
              {
                SkScalar a = SkScalar(unpadded_size[0]) / SkScalar(5);
                paint.setStrokeWidth(a);
                paint.setStrokeCap(SkPaint::kRound_Cap);
                paint.setStyle(SkPaint::kFill_Style);
                renderer.canvas().drawLine(
                    SkScalar(-a), SkScalar(-a),
                    SkScalar(a), SkScalar(a),
                    paint);
                renderer.canvas().drawLine(
                    SkScalar(-a), SkScalar(a),
                    SkScalar(a), SkScalar(-a),
                    paint);
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

    icon_button_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        static default_icon_button_renderer default_renderer;
        refresh_themed_rendering_data(ctx, data->rendering, &default_renderer);
        icon_button_renderer const* renderer = data->rendering.renderer;
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(renderer->default_size(ctx), 0, 0),
            LEFT | CENTER_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        //refresh_focus(ctx, id);
        break;
      }

     case RENDER_CATEGORY:
      {
        if (!is_rendering_active(ctx))
            break;
        data->rendering.renderer->draw(ctx, data->rendering.data,
            data->layout_node.assignment().region, icon,
            get_button_state(ctx, id, data->input));
        break;
      }

     case REGION_CATEGORY:
        do_box_region(ctx, id, data->layout_node.assignment().region);
        break;

     case INPUT_CATEGORY:
        if (do_button_input(ctx, id, data->input))
            return true;
        break;
    }

    return false;
}

// SLIDER

struct slider_renderer : dispatch_interface
{
    virtual layout_scalar default_width(ui_context& ctx) const = 0;
    virtual layout_scalar left_border(ui_context& ctx) const = 0;
    virtual layout_scalar right_border(ui_context& ctx) const = 0;
    virtual layout_scalar height(ui_context& ctx) const = 0;
    virtual layout_box thumb_region(ui_context& ctx) const = 0;
    virtual box<1,layout_scalar> track_region(ui_context& ctx) const = 0;

    virtual void draw_track(
        ui_context& ctx, renderer_data_ptr& data_ptr, unsigned axis,
        layout_vector const& track_position, layout_scalar track_width) const
        = 0;

    virtual void draw_thumb(
        ui_context& ctx, renderer_data_ptr& data_ptr, unsigned axis,
        layout_vector const& thumb_position, widget_state state) const = 0;
};

struct default_slider_renderer : slider_renderer
{
    struct data_type
    {
        caching_renderer_data track, thumb;
    };

    layout_scalar default_width(ui_context& ctx) const
    {
        return resolve_layout_width(get_layout_traversal(ctx), 20, EM);
    }
    layout_scalar left_border(ui_context& ctx) const
    {
        return resolve_layout_width(get_layout_traversal(ctx), 0.5f, EM);
    }
    layout_scalar right_border(ui_context& ctx) const
    {
        return resolve_layout_width(get_layout_traversal(ctx), 0.5f, EM);
    }
    layout_scalar height(ui_context& ctx) const
    {
        return resolve_layout_width(get_layout_traversal(ctx), 1.3f, EM);
    }
    layout_box thumb_region(ui_context& ctx) const
    {
        layout_scalar x =
            resolve_layout_width(get_layout_traversal(ctx), 1.3f, EM);
        return layout_box(make_layout_vector(
            round_to_layout_scalar(x * -0.3), 0),
            make_layout_vector(round_to_layout_scalar(x * 0.6), x));
    }
    box<1,layout_scalar> track_region(ui_context& ctx) const
    {
        layout_scalar x =
            resolve_layout_width(get_layout_traversal(ctx), 1.3f, EM);
        box<1,layout_scalar> b;
        b.corner[0] = as_layout_size(x * 0.5);
        b.size[0] = x / 6;
        return b;
    }

    void draw_track(
        ui_context& ctx, renderer_data_ptr& data_ptr, unsigned axis,
        layout_vector const& track_position, layout_scalar track_width) const
    {
        data_type* data;
        cast_data_ptr(&data, data_ptr);

        layout_vector track_size;
        track_size[axis] = track_width;
        track_size[1 - axis] = track_region(ctx).size[0];

        layout_box track_box(track_position, track_size);

        caching_renderer cache(ctx, data->track,
            combine_ids(ref(*ctx.style.id), make_id(0)),
            track_box);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), track_box.size);

            style_tree const* slider_style =
                find_substyle(ctx.style.path, "slider");

            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);

            rgba8 color = get_color_property(slider_style, "track_color");

            renderer.canvas().drawColor(
                SkColorSetARGB(color.a, color.r, color.g, color.b));

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }

    void draw_thumb(
        ui_context& ctx, renderer_data_ptr& data_ptr, unsigned axis,
        layout_vector const& thumb_position, widget_state state) const
    {
        data_type* data;
        cast_data_ptr(&data, data_ptr);

        layout_box thumb_region = this->thumb_region(ctx);
        thumb_region.corner += thumb_position;

        caching_renderer cache(ctx, data->track,
            combine_ids(ref(*ctx.style.id), make_id(state)),
            thumb_region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), thumb_region.size);

            style_tree const* slider_style =
                find_substyle(ctx.style.path, "slider", state);

            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);

            rgba8 color = get_color_property(slider_style, "thumb_color");

            renderer.canvas().drawColor(
                SkColorSetARGB(color.a, color.r, color.g, color.b));

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }
};

struct slider_data
{
    slider_data() : dragging(false) {}
    themed_rendering_data<slider_renderer> rendering;
    layout_leaf layout_node;
    widget_data track_id, thumb_id;
    bool dragging;
    int dragging_offset;
    double dragging_value;
    focus_rect_data focus_rendering;
};

template<typename T>
static T clamp(T x, T min, T max)
{
    assert(min <= max);
    return (std::min)((std::max)(x, min), max);
}

static double clamp(double x, double min, double max, double step)
{
    assert(min <= max);
    double clamped = clamp(x, min, max);
    return std::floor((clamped - min) / step + 0.5) * step;
}

static double get_values_per_pixel(ui_context& ctx, slider_data& data,
    unsigned axis, double minimum, double maximum)
{
    layout_box const& assigned_region = data.layout_node.assignment().region;
    slider_renderer const* renderer = data.rendering.renderer;
    return double(maximum - minimum) /
        (assigned_region.size[axis] - renderer->left_border(ctx) -
        renderer->right_border(ctx) - 1);
}

static layout_vector get_track_position(ui_context& ctx, slider_data& data,
    unsigned axis)
{
    slider_renderer const* renderer = data.rendering.renderer;
    layout_box const& assigned_region = data.layout_node.assignment().region;
    layout_vector track_position;
    track_position[axis] = assigned_region.corner[axis] +
        renderer->left_border(ctx);
    track_position[1 - axis] = assigned_region.corner[1 - axis] +
        renderer->track_region(ctx).corner[0];
    return track_position;
}

static layout_scalar get_track_width(ui_context& ctx, slider_data& data,
    unsigned axis)
{
    slider_renderer const* renderer = data.rendering.renderer;
    layout_box const& assigned_region = data.layout_node.assignment().region;
    return assigned_region.size[axis] -
        renderer->left_border(ctx) - renderer->right_border(ctx);
}

static layout_vector get_thumb_position(ui_context& ctx, slider_data& data,
    unsigned axis, double minimum, double maximum, getter<double> const& value)
{
    slider_renderer const* renderer = data.rendering.renderer;
    layout_box const& assigned_region = data.layout_node.assignment().region;
    layout_vector thumb_position;
    if (data.dragging &&
        (!value.is_gettable() || value.get() == data.dragging_value))
    {
        thumb_position[axis] = get_integer_mouse_position(ctx)[axis] -
            data.dragging_offset;
        thumb_position[1 - axis] = assigned_region.corner[1 - axis];

        int const maximum_position = get_high_corner(assigned_region)[axis] -
            renderer->right_border(ctx) - 1;
        int const minimum_position = assigned_region.corner[axis] +
            renderer->left_border(ctx);

        thumb_position[axis] = clamp(thumb_position[axis],
            minimum_position, maximum_position);
    }
    else
    {
        thumb_position = assigned_region.corner;
        thumb_position[axis] +=
            round_to_layout_scalar((get(value) - minimum) /
                get_values_per_pixel(ctx, data, axis, minimum, maximum)) +
            renderer->left_border(ctx);
    }
    return thumb_position;
}

static layout_box get_thumb_region(ui_context& ctx, slider_data& data,
    unsigned axis, double minimum, double maximum, getter<double> const& value)
{
    slider_renderer const* renderer = data.rendering.renderer;
    layout_vector thumb_position =
        get_thumb_position(ctx, data, axis, minimum, maximum, value);
    layout_box thumb_region;
    thumb_region.corner[axis] =
        renderer->thumb_region(ctx).corner[0] + thumb_position[axis];
    thumb_region.corner[1 - axis] =
        renderer->thumb_region(ctx).corner[1] + thumb_position[1 - axis];
    thumb_region.size[axis] = renderer->thumb_region(ctx).size[0];
    thumb_region.size[1 - axis] = renderer->thumb_region(ctx).size[1];
    return thumb_region;
}

slider_result
do_slider(ui_context& ctx, accessor<double> const& value,
    double minimum, double maximum, double step,
    layout const& layout_spec, ui_flag_set flags)
{
    slider_result result;
    result.changed = false;

    slider_data* data;
    get_cached_data(ctx, &data);

    unsigned axis = (flags & VERTICAL) ? 1 : 0;

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        //refresh_widget_id(ctx, &data->track_id);
        //refresh_widget_id(ctx, &data->thumb_id);
        static default_slider_renderer default_renderer;
        refresh_themed_rendering_data(ctx, data->rendering, &default_renderer);
        slider_renderer const* renderer = data->rendering.renderer;
        vector<2,int> default_size;
        default_size[axis] = renderer->default_width(ctx);
        default_size[1 - axis] = renderer->height(ctx);
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(default_size, 0, 0),
            LEFT | CENTER_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        //refresh_focus(ctx, thumb_id);
        break;
      }

     case RENDER_CATEGORY:
      {
        if (!is_rendering_active(ctx))
            break;
        slider_renderer const* renderer = data->rendering.renderer;
        renderer->draw_track(ctx, data->rendering.data, axis,
            get_track_position(ctx, *data, axis),
            get_track_width(ctx, *data, axis));
        widget_state thumb_state = get_widget_state(ctx, &data->thumb_id);
        if (value.is_gettable())
        {
            renderer->draw_thumb(ctx, data->rendering.data, axis,
                get_thumb_position(ctx, *data, axis, minimum, maximum, value),
                thumb_state);
        }
        if (thumb_state & WIDGET_FOCUSED)
        {
            draw_focus_rect(ctx, data->focus_rendering,
                data->layout_node.assignment().region);
        }
        break;
      }

     case REGION_CATEGORY:
      {
        if (!value.is_gettable())
            break;

        slider_renderer const* renderer = data->rendering.renderer;

        layout_vector track_size;
        track_size[axis] = get_track_width(ctx, *data, axis);
        track_size[1 - axis] = renderer->track_region(ctx).size[0];
        do_box_region(ctx, &data->track_id,
            add_border(
                layout_box(get_track_position(ctx, *data, axis), track_size),
                make_layout_vector(2, 2)));

        do_box_region(ctx, &data->thumb_id,
            get_thumb_region(ctx, *data, axis, minimum, maximum, value));

        break;
      }

     case INPUT_CATEGORY:
      {
        if (!value.is_gettable())
            break;

        if (detect_mouse_press(ctx, &data->track_id, LEFT_BUTTON) ||
            detect_drag(ctx, &data->track_id, LEFT_BUTTON))
        {
            slider_renderer const* renderer = data->rendering.renderer;

            double new_value =
                (get_integer_mouse_position(ctx)[axis] -
                    data->layout_node.assignment().region.corner[axis] -
                    renderer->left_border(ctx)) *
                get_values_per_pixel(ctx, *data, axis, minimum, maximum) +
                minimum;

            set_new_value(value, result,
                clamp(new_value, minimum, maximum, step));
        }

        if (detect_drag(ctx, &data->thumb_id, LEFT_BUTTON))
        {
            if (!data->dragging)
            {
                layout_vector thumb_position =
                    get_thumb_position(ctx, *data, axis, minimum, maximum,
                        value);
                data->dragging_offset =
                    get_integer_mouse_position(ctx)[axis] -
                    thumb_position[axis];
                data->dragging = true;
            }

            slider_renderer const* renderer = data->rendering.renderer;

            double new_value =
                (get_integer_mouse_position(ctx)[axis] -
                    data->dragging_offset -
                    data->layout_node.assignment().region.corner[axis] -
                    renderer->left_border(ctx)) *
                get_values_per_pixel(ctx, *data, axis, minimum, maximum) +
                minimum;

            set_new_value(value, result,
                clamp(new_value, minimum, maximum, step));

            data->dragging_value = get(value);
        }

        if (detect_drag_release(ctx, &data->thumb_id, LEFT_BUTTON))
            data->dragging = false;

        add_to_focus_order(ctx, &data->thumb_id);

        key_event_info info;
        if (detect_key_press(ctx, &info, &data->thumb_id) && info.mods == 0)
        {
            double increment = (maximum - minimum) / 10;
            switch (info.code)
            {
             case KEY_LEFT:
                if (axis == 0)
                {
                    set_new_value(value, result,
                        clamp(get(value) - increment, minimum, maximum, step));
                    acknowledge_input_event(ctx);
                }
                break;
             case KEY_DOWN:
                if (axis == 1)
                {
                    set_new_value(value, result,
                        clamp(get(value) - increment, minimum, maximum, step));
                    acknowledge_input_event(ctx);
                }
                break;
             case KEY_RIGHT:
                if (axis == 0)
                {
                    set_new_value(value, result,
                        clamp(get(value) - increment, minimum, maximum, step));
                    acknowledge_input_event(ctx);
                }
                break;
             case KEY_UP:
                if (axis == 1)
                {
                    set_new_value(value, result,
                        clamp(get(value) + increment, minimum, maximum, step));
                    acknowledge_input_event(ctx);
                }
                break;
             case KEY_HOME:
                set_new_value(value, result, minimum);
                acknowledge_input_event(ctx);
                break;
             case KEY_END:
                set_new_value(value, result, maximum);
                acknowledge_input_event(ctx);
                break;
            }
        }

        break;
      }
    }

    return result;
}

// COLOR CONTROL

static void do_color_block(
    ui_context& ctx, drop_down_list<rgba8>& ddl, uint8_t red)
{
    column_layout c(ctx);
    for (int i = 0; i != 4; ++i)
    {
        row_layout r(ctx, default_layout);
        for (int j = 0; j != 4; ++j)
        {
            rgba8 color(
                red, uint8_t(i * 255 / 3), uint8_t(j * 255 / 3), 0xff);
            ddl_item<rgba8> item(ddl, color);
            do_color(ctx, in(color));
        }
    }
}

color_control_result
do_color_control(ui_context& ctx, accessor<rgba8> const& value,
    layout const& layout_spec)
{
    drop_down_list<rgba8> ddl(ctx, value);
    do_color(ctx, value);
    alia_if (ddl.do_list())
    {
        ui_context& ctx = ddl.list_context();
        column_layout c(ctx);
        {
            row_layout r(ctx, default_layout);
            do_color_block(ctx, ddl, uint8_t(0 * 255 / 3));
            do_spacer(ctx, width(1, CHARS));
            do_color_block(ctx, ddl, uint8_t(1 * 255 / 3));
        }
        do_spacer(ctx, height(1, CHARS));
        {
            row_layout r(ctx, default_layout);
            do_color_block(ctx, ddl, uint8_t(2 * 255 / 3));
            do_spacer(ctx, width(1, CHARS));
            do_color_block(ctx, ddl, uint8_t(3 * 255 / 3));
        }
    }
    alia_end

    color_control_result result;
    result.changed = ddl.changed();
    if (result.changed)
        result.new_value = get(value);
    return result;
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
            size(1.2f, 1.2f, EM));
    }
    void draw(
        ui_context& ctx, renderer_data_ptr& data_ptr, layout_box const& region,
        widget_state state) const
    {
        caching_renderer_data* data;
        cast_data_ptr(&data, data_ptr);

        layout_vector const& padding_size =
            ctx.layout.style_info->padding_size;
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
                SkScalar(padding_size[0]),
                SkScalar(padding_size[1]));

            paint.setStyle(SkPaint::kFill_Style);
            set_color(paint, bg_color);
            draw_rect(renderer.canvas(), paint, unpadded_size);

            renderer.canvas().translate(
                SkScalar(unpadded_size[0] / 2),
                SkScalar(unpadded_size[1] / 2));
            renderer.canvas().rotate(90);

            {
                set_color(paint, fg_color);
                paint.setStyle(SkPaint::kFill_Style);
                SkScalar a = SkScalar(unpadded_size[0]) / SkScalar(1.8);
                SkPath path;
                path.incReserve(4);
                SkPoint p0;
                p0.fX = a * SkScalar(-0.34);
                p0.fY = a * SkScalar(-0.5);
                path.moveTo(p0);
                SkPoint p1;
                p1.fX = p0.fX;
                p1.fY = a * SkScalar(0.5);
                path.lineTo(p1);
                SkPoint p2;
                p2.fX = p0.fX + a * SkScalar(0.866);
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

static bool detect_popup_click(
    ui_context& ctx, popup_ptr& popup, bool* open_at_click,
    widget_id id, mouse_button button)
{
    if (detect_mouse_press(ctx, id, button))
    {
        *open_at_click = is_open(popup);
        // Refuse to be the active ID, since this click will effectively be
        // ignored.
        if (*open_at_click)
            ctx.system->input.active_id = null_widget_id;
    }
    return detect_mouse_release(ctx, id, button) && is_region_active(ctx, id);
}

static bool
do_drop_down_button(
    ui_context& ctx,
    layout const& layout_spec,
    widget_id id,
    popup_ptr& popup,
    bool* open_at_click)
{
    drop_down_button_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        static default_drop_down_button_renderer default_renderer;
        refresh_themed_rendering_data(ctx, data->rendering, &default_renderer);
        drop_down_button_renderer const* renderer = data->rendering.renderer;
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(renderer->default_size(ctx), 0, 0),
            LEFT | CENTER_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        //refresh_focus(ctx, id);
        break;
      }

     case RENDER_CATEGORY:
      {
        if (!is_rendering_active(ctx))
            break;
        data->rendering.renderer->draw(ctx, data->rendering.data,
            data->layout_node.assignment().region,
            get_button_state(ctx, id, data->input));
        break;
      }

     case REGION_CATEGORY:
        do_box_region(ctx, id, data->layout_node.assignment().region);
        break;

     case INPUT_CATEGORY:
        add_to_focus_order(ctx, id);
        if (detect_popup_click(ctx, popup, open_at_click, id, LEFT_BUTTON))
        {
            if (!*open_at_click)
                return true;
            else
                popup.reset();
        }
        if (detect_keyboard_click(ctx, data->input.key, id, KEY_SPACE))
            return true;
        break;
    }

    return false;
}

struct ddl_data
{
    popup_ptr popup;
    bool open_at_click;

    // When the list is open, it may maintain a separate internal selection.
    // The internal selection can be copied into the actual control state
    // when the list is closed.
    optional<int> internal_selection;

    button_input_state input;
    focus_rect_data focus_rendering;
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

//struct proxy_controller : ui_controller
//{
//    proxy_controller(
//        ui_system& parent_system, routable_widget_id parent_id)
//      : parent_system(parent_system), parent_id(parent_id) {}
//    ui_system& parent_system;
//    routable_widget_id parent_id;
//
//    void do_ui(ui_context& ctx)
//    {
//        wrapped_event e(parent_id.id, ctx);
//        issue_targeted_event(parent_system, e, parent_id);
//    }
//};

static bool process_ddl_key_press(ui_context& ctx, widget_id ddl_id,
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

untyped_ui_value const*
untyped_drop_down_list::begin(ui_context& ctx, layout const& layout_spec,
    ui_flag_set flags)
{
    //ctx_ = &ctx;

    //do_list_ = make_selection_visible_ = false;
    //list_index_ = 0;

    untyped_ui_value const* result = 0;

    //id_ = get_widget_id(ctx);

    //get_data(ctx, &data_);

    //widget_state state = get_button_state(ctx, id_, data_->input);

    //container_.begin(ctx, add_default_padding(add_default_alignment(
    //    layout_spec, LEFT, BASELINE_Y), PADDED));

    //substyle_.begin(ctx, const_text("control"), state);

    //switch (ctx.event->category)
    //{
    // case RENDER_CATEGORY:
    //  {
    //    if (!is_rendering_active(ctx))
    //        break;
    //    layout_vector poly[4];
    //    make_polygon(poly, container_.region());
    //    ctx.surface->draw_filled_polygon(
    //        ctx.style.properties->bg_color, poly, 4);

    //    if ((state & WIDGET_FOCUSED))
    //        draw_focus_rect(ctx, data_->focus_rendering, container_.region());

    //    break;
    //  }

    // case REGION_CATEGORY:
    //    do_box_region(ctx, id_, container_.region());
    //    break;

    // case INPUT_CATEGORY:
    //  {
    //    key_event_info info;
    //    if (detect_key_press(ctx, &info))
    //    {
    //        // If this is a list of commands, don't select them without the
    //        // list being open.
    //        if (!(flags & COMMAND_LIST))
    //        {
    //            optional<int> selection = get_ddl_selected_index(ctx, id_);
    //            if (process_ddl_key_press(ctx, id_, selection, info))
    //            {
    //                acknowledge_input_event(ctx);
    //                if (selection)
    //                    select_ddl_item_at_index(ctx, id_, get(selection));
    //            }
    //        }
    //    }
    //    break;
    //  }

    // case NO_CATEGORY:
    //    switch (ctx.event->type)
    //    {
    //     case SET_VALUE_EVENT:
    //      {
    //        set_value_event& e = get_event<set_value_event>(ctx);
    //        if (e.target == id_)
    //        {
    //            result = e.value.get();
    //            if (data_->popup)
    //                data_->popup->close();
    //        }
    //        break;
    //      }

    //     case CUSTOM_EVENT:
    //      {
    //        {
    //            ddl_list_query_event* query =
    //                dynamic_cast<ddl_list_query_event*>(ctx.event);
    //            if (query && query->target == id_)
    //            {
    //                list_ctx_ = ctx_;
    //                do_list_ = true;
    //            }
    //        }
    //        {
    //            ddl_select_index_event* event =
    //                dynamic_cast<ddl_select_index_event*>(ctx.event);
    //            if (event && event->target == id_)
    //            {
    //                list_ctx_ = ctx_;
    //                do_list_ = true;
    //            }
    //        }
    //        break;
    //      }
    //    }
    //    break;
    //}

    //contents_.begin(ctx, BASELINE_Y | GROW_X | UNPADDED);

    //do_list_ = is_active_overlay(ctx, overlay_id);

    //alia_if (do_list_)
    //{
    //    list_border_.begin(*list_ctx_, GROW | PADDED);
    //    list_panel_.begin(*list_ctx_, const_text(""), 2,
    //        GROW | UNPADDED);

    //    key_event_info info;
    //    if (detect_key_press(*list_ctx_, &info))
    //    {
    //        if (process_ddl_key_press(ctx, id_,
    //                data_->internal_selection, info))
    //        {
    //            acknowledge_input_event(*list_ctx_);
    //            make_selection_visible_ = true;
    //        }
    //        if (info.mods == 0 &&
    //           (info.code == KEY_ENTER ||
    //            info.code == KEY_NUMPAD_ENTER ||
    //            info.code == KEY_SPACE))
    //        {
    //            if (data_->internal_selection)
    //            {
    //                select_ddl_item_at_index(ctx, id_,
    //                    get(data_->internal_selection));
    //            }
    //            acknowledge_input_event(*list_ctx_);
    //            data_->popup->close();
    //        }
    //        if (info.mods == 0 && info.code == KEY_ESCAPE)
    //        {
    //            acknowledge_input_event(*list_ctx_);
    //            data_->popup->close();
    //        }
    //    }
    //}
    //alia_end

    return result;
}
void untyped_drop_down_list::end()
{
    //if (ctx_)
    //{
    //    list_panel_.end();
    //    list_border_.end();

    //    contents_.end();

    //    if (!ctx_->pass_aborted)
    //    {
    //        if (do_drop_down_button(*ctx_, CENTER, id_,
    //                data_->popup, &data_->open_at_click))
    //        {
    //            data_->internal_selection =
    //                get_ddl_selected_index(*ctx_, id_);
    //            vector<2,int> absolute_position = vector<2,int>(
    //                transform(ctx_->geometry.transformation_matrix,
    //                    vector<2,double>(container_.region().corner)) +
    //                make_vector<double>(0.5, 0.5));
    //            //data_->popup.reset(ctx_->surface->open_popup(
    //            //    new proxy_controller(*ctx_->system,
    //            //        make_routable_widget_id(*ctx_, id_)),
    //            //    absolute_position +
    //            //        make_vector<int>(0, container_.region().size[1]),
    //            //    absolute_position +
    //            //        make_vector<int>(container_.region().size[0], 0),
    //            //    make_vector<int>(container_.region().size[0], 0)));
    //        }
    //    }

    //    container_.end();

    //    ctx_ = 0;
    //}
}

bool untyped_ddl_item::begin(untyped_drop_down_list& list, bool is_selected)
{
    //list_ = &list;
    //int index = list.list_index_++;
    //ddl_data* data = list.data_;

    //bool is_internally_selected = data->internal_selection &&
    //    get(data->internal_selection) == index;

    //ui_context& ctx = *list.list_ctx_;

    //widget_id id = get_widget_id(ctx);
    //panel_.begin(ctx, const_text("item"), UNPADDED, NO_FLAGS, id,
    //    get_widget_state(ctx, id, true, false, is_internally_selected));

    //if (list.make_selection_visible_ && is_internally_selected)
    //    make_widget_visible(*ctx.system, make_routable_widget_id(ctx, id));

    //switch (ctx.event->category)
    //{
    // case INPUT_CATEGORY:
    //    if (detect_click(ctx, id, LEFT_BUTTON))
    //        return true;
    //    break;

    // case NO_CATEGORY:
    //    switch (ctx.event->type)
    //    {
    //     case CUSTOM_EVENT:
    //        {
    //            ddl_list_query_event* query =
    //                dynamic_cast<ddl_list_query_event*>(ctx.event);
    //            if (query && query->target == list.id_)
    //            {
    //                if (is_selected)
    //                    query->selected_index = index;
    //                ++query->total_items;
    //            }
    //        }
    //        {
    //            ddl_select_index_event* event =
    //                dynamic_cast<ddl_select_index_event*>(ctx.event);
    //            if (event && event->target == list.id_ &&
    //                event->index == index)
    //            {
    //                return true;
    //            }
    //        }
    //        break;

    //     case INITIAL_VISIBILITY_EVENT:
    //        if (is_internally_selected)
    //        {
    //            make_widget_visible(*ctx.system,
    //                make_routable_widget_id(ctx, id));
    //        }
    //        break;
    //    }
    //    break;
    //}

    return false;
}
void untyped_ddl_item::end()
{
    //panel_.end();
}
void untyped_ddl_item::select(untyped_ui_value* value)
{
    //set_value_event e(list_->id_, value);
    //issue_targeted_event(*list_->ctx_->system, e,
    //    make_routable_widget_id(*list_->ctx_, list_->id_));
}

// TEXT CONTROL

struct text_control_data;

struct text_control_layout_node : layout_node
{
    // implementation of normal layout interface
    layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx);
    layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_scalar assigned_width);
    void set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment);

    text_control_data* data;
};

struct text_control_data
{
    text_control_data()
      : cursor_on(false)
      , cursor_position(0)
      , editing(false)
      , first_selected(0)
      , n_selected(0)
      , true_cursor_x(-1)
      , need_layout(false)
      , force_cursor_visible(false)
      , text_edited(false)
    {}

    // flags passed in by user
    ui_flag_set flags;

    // layout
    text_control_layout_node layout_node;
    resolved_layout_spec resolved_spec;
    layout_box assigned_region;

    // is the cursor on?
    bool cursor_on;
    // the cursor is before the character at the given index
    cached_text::offset cursor_position;

    // in editing mode?
    bool editing;

    // the range of characters that's selected
    cached_text::offset first_selected;
    unsigned n_selected;

    bool safe_to_drag;
    // when dragging, this is the character index at which the drag started
    cached_text::offset drag_start_index;

    // When moving the cursor vertically, the horizontal position within the
    // new line is determined by the cursor's original horizontal position on
    // the line where the vertical motion started, so we have to remember that.
    int true_cursor_x;

    bool need_layout;

    // Sometimes it's not possible to immediately force the cursor to be
    // visible because the text has changed and we don't know where the cursor
    // will actually be, so we need to remember to do it after the layout has
    // been recalculated.
    bool force_cursor_visible;

    // the text that is currently in the text box
    string text;

    // the ID of the external value associated with the text
    owned_id external_id;

    // the ID of the text style active for this control
    owned_id style_id;

    // true if text is different than external value
    bool text_edited;

    ascii_font_image const* font_image;
    ascii_font_image const* highlighted_font_image;

    cached_text_ptr renderer;

    focus_rect_data focus_rendering;
};

string get_display_text(text_control_data& tc)
{
    if (tc.flags & PASSWORD)
        return string(tc.text.length(), '*');
    else
        return tc.text;
}

layout_requirements text_control_layout_node::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    // TODO: Set some sort of true minimum width?
    layout_requirements requirements;
    resolve_requirements(
        requirements, data->resolved_spec, 0,
        calculated_layout_requirements(0, 0, 0));
    return requirements;
}
layout_requirements text_control_layout_node::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_scalar assigned_width)
{
    // TODO: This is rather inefficient, but it's good enough for now.
    cached_text_ptr* renderer;
    get_data(ctx, &renderer);
    if (!*renderer ||
        assigned_width - 1 != (*renderer)->get_size()[0])
    {
        renderer->reset(new cached_ascii_text(
            *data->font_image, *data->highlighted_font_image,
            get_display_text(*data), assigned_width - 1));
    }

    layout_requirements requirements;
    resolve_requirements(
        requirements, data->resolved_spec, 1,
        calculated_layout_requirements(
            as_layout_size((*renderer)->get_size()[1]),
            as_layout_size(data->font_image->metrics.ascent),
            as_layout_size((*renderer)->get_size()[1] -
                as_layout_size(data->font_image->metrics.ascent))));
    return requirements;
}
void text_control_layout_node::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    // TODO: Actually resolve position?
    data->assigned_region = assignment.region;
    data->assigned_region.corner[1] +=
        assignment.baseline_y - data->font_image->metrics.ascent;
}

struct text_control
{
 public:
    text_control(
        ui_context& ctx,
        text_control_data& data,
        accessor<string> const& value,
        layout const& layout_spec,
        ui_flag_set flags,
        widget_id id,
        int max_chars)
      : ctx(ctx), data(data), value(value),
        flags(flags), layout_spec(layout_spec), id(id), max_chars(max_chars)
    {}

    void do_pass()
    {
        result.event = TEXT_CONTROL_NO_EVENT;
        result.changed = false;
        cursor_id = get_widget_id(ctx);

        panel_.begin(ctx, const_text("control"),
            add_default_alignment(layout_spec, LEFT, BASELINE_Y), HORIZONTAL);
        switch (ctx.event->category)
        {
         case REFRESH_CATEGORY:
            do_refresh();
            break;
         case RENDER_CATEGORY:
            if (!is_rendering_active(ctx))
                break;
            render();
            break;
         case REGION_CATEGORY:
            update_renderer();
            do_box_region(ctx, cursor_id, get_cursor_region(), IBEAM_CURSOR);
            do_box_region(ctx, id, get_full_region(), IBEAM_CURSOR);
            break;
         case INPUT_CATEGORY:
            do_input();
            break;
        }
    }

    text_control_result<string> result;

 private:
    bool is_password() const { return (flags & PASSWORD) != 0; }
    bool is_read_only() const { return (flags & DISABLED) != 0; }
    bool is_disabled() const { return (flags & DISABLED) != 0; }
    bool is_single_line() const { return (flags & SINGLE_LINE) != 0; }
    bool is_multiline() const { return (flags & MULTILINE) != 0; }

    box<2,int> get_full_region()
    {
        return panel_.outer_region();
    }

    box<2,int> get_cursor_region()
    {
        return box<2,int>(
            get_character_boundary_location(data.cursor_position),
            make_vector<int>(1, data.font_image->metrics.height));
    }

    void reset_to_external_value()
    {
        data.text = value.is_gettable() ? get(value) : "";
        data.cursor_position = cached_text::offset(data.text.length());
        on_text_change();
        data.text_edited = false;
        if ((flags & ALWAYS_EDITING) == 0)
            exit_edit_mode();
    }

    void do_refresh()
    {
        if (!data.external_id.matches(value.id()))
        {
            // The value changed through some external program logic,
            // so update the displayed text to reflect it.
            // This also aborts any edits that may have been taking
            // place.
            reset_to_external_value();
            data.external_id.store(value.id());
        }

        if (!data.style_id.matches(*ctx.style.id))
        {
            data.need_layout = true;

            data.font_image = get_ascii_font_image(
                ctx.style.properties->font,
                ctx.style.properties->text_color,
                ctx.style.properties->bg_color);
            data.highlighted_font_image = get_ascii_font_image(
                ctx.style.properties->font,
                ctx.style.properties->selected_text_color,
                ctx.style.properties->selected_bg_color);

            data.style_id.store(*ctx.style.id);
        }

        if (flags != data.flags)
        {
            data.need_layout = true;
            data.flags = flags;
        }

        if (data.need_layout)
        {
            resolve_layout_spec(get_layout_traversal(ctx),
                data.resolved_spec, UNPADDED, BASELINE_Y | GROW_X);
            record_layout_change(get_layout_traversal(ctx));
            data.layout_node.data = &data;
            data.renderer.reset();
            data.need_layout = false;
        }

        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
    }

    void update_renderer()
    {
        if (!data.renderer)
        {
            data.renderer.reset(new cached_ascii_text(
                *data.font_image, *data.highlighted_font_image,
                get_display_text(data), data.assigned_region.size[0] - 1));
        }
    }

    void render()
    {
        update_renderer();

        if (id_has_focus(ctx, id))
            draw_focus_rect(ctx, data.focus_rendering, get_full_region());

        if (data.n_selected != 0)
        {
            data.renderer->draw_with_selection(
                *ctx.surface, vector<2,double>(data.assigned_region.corner),
                data.first_selected,
                data.first_selected + data.n_selected);
        }
        else
        {
            data.renderer->draw(
                *ctx.surface, vector<2,double>(data.assigned_region.corner));
        }

        if (data.cursor_on && data.editing)
        {
            vector<2,int> cursor_p = get_character_boundary_location(
                data.cursor_position);
            bool cursor_selected =
                (data.n_selected != 0 &&
                data.cursor_position >= data.first_selected &&
                data.cursor_position < cached_text::offset(
                    data.first_selected + data.n_selected));
            data.renderer->draw_cursor(
                *ctx.surface, cursor_selected, vector<2,double>(cursor_p));
        }
    }

    void do_input()
    {
        update_renderer();

        if (detect_double_click(ctx, id, LEFT_BUTTON))
        {
            string display_text = data.renderer->get_text();
            int i = get_character_at_pixel(get_integer_mouse_position(ctx));
            if (i >= 0 && i < int(display_text.length()))
            {
                int left, right;
                if (std::isspace(display_text[i]))
                {
                    left = i;
                    while (left > 0 &&
                        std::isspace(display_text[left - 1]))
                    {
                        --left;
                    }
                    right = i + 1;
                    while (right < int(display_text.length()) &&
                        std::isspace(display_text[right]))
                    {
                        ++right;
                    }
                }
                else if (std::isalnum(display_text[i]))
                {
                    left = i;
                    while (left > 0 &&
                        std::isalnum(display_text[left - 1]))
                    {
                        --left;
                    }
                    right = i + 1;
                    while (right < int(display_text.length()) &&
                        std::isalnum(display_text[right]))
                    {
                        ++right;
                    }
                }
                else
                {
                    left = i;
                    right = i + 1;
                }
                set_selection(left, right);
                data.cursor_position = right;
                data.true_cursor_x = -1;
                reset_cursor_blink();
            }
        }
        else if (detect_mouse_press(ctx, id, LEFT_BUTTON))
        {
            // This determines if the click is just an initial "move the focus
            // to this control and select its text" click or a an actual click
            // to move the cursor and/or drag.
            // If the control already has focus, then all clicks are the latter
            // type. Similarly if the control is read-only. It's less clear
            // what to do for multiline controls (and what constitutes a
            // "multiline" control), so this may have to be revisited.
            if (is_read_only() || data.renderer->get_line_count() > 1 ||
                id_has_focus(ctx, id))
            {
                int i = get_character_boundary_at_pixel(
                    get_integer_mouse_position(ctx));
                data.drag_start_index = i;
                move_cursor(i);
                reset_cursor_blink();
                data.safe_to_drag = true;
                if (!is_read_only())
                    data.editing = true;
            }
            else
                data.safe_to_drag = false;
        }
        else if (detect_drag(ctx, id, LEFT_BUTTON) && data.safe_to_drag)
        {
            do_drag();
            start_timer(ctx, id, drag_delay);
        }

        if (is_timer_done(ctx, id) && is_region_active(ctx, id) &&
            is_mouse_button_pressed(ctx, LEFT_BUTTON))
        {
            do_drag();
            restart_timer(ctx, id, drag_delay);
        }

        //if (detect_click(ctx, id, RIGHT_BUTTON))
        //{
        //    right_click_menu menu(*this);
        //    ctx.surface->show_popup_menu(&menu);
        //}

        do_key_input();

        {
            if (detect_focus_gain(ctx, id))
            {
                if (!is_read_only())
                    data.editing = true;
                reset_cursor_blink();
                ensure_cursor_visible();
                if (!is_read_only() && data.renderer->get_line_count() < 2)
                    select_all();
            }
            else if (detect_focus_loss(ctx, id))
            {
                if (data.text_edited)
                {
                    value.set(data.text);
                    result.new_value = data.text;
                    result.event = TEXT_CONTROL_FOCUS_LOST;
                    result.changed = true;
                }
                exit_edit_mode();
            }
        }

        if (id_has_focus(ctx, id) && is_timer_done(ctx, cursor_id))
        {
            data.cursor_on = !data.cursor_on;
            restart_timer(ctx, cursor_id, cursor_blink_delay);
        }
    }

    void do_drag()
    {
        int i = get_character_boundary_at_pixel(
            get_integer_mouse_position(ctx));
        if (i < 0)
            i = 0;
        else if (i > int(data.renderer->get_text().length()))
            i = int(data.renderer->get_text().length()) - 1;
        set_selection(data.drag_start_index, i);
        data.cursor_position = i;
        data.true_cursor_x = -1;
        ensure_cursor_visible();
        reset_cursor_blink();
    }

    void do_key_input()
    {
        if (!is_read_only())
            add_to_focus_order(ctx, id);

        utf8_string text;
        if (detect_text_input(ctx, &text, id))
        {
            // TODO: real unicode
            for (utf8_ptr p = text.begin; p != text.end; ++p)
            {
                if (std::isprint(*p))
                {
                    if (data.editing)
                    {
                        insert_text(string(1, *p));
                        on_edit();
                    }
                    acknowledge_key();
                }
            }
        }
        key_event_info info;
        if (detect_key_press(ctx, &info, id))
        {
            switch (info.mods.code)
            {
             case 0:
                switch (info.code)
                {
                 case KEY_HOME:
                    move_cursor(get_home_position());
                    acknowledge_key();
                    break;

                 case KEY_END:
                    move_cursor(get_end_position());
                    acknowledge_key();
                    break;

                 case KEY_ENTER:
                 case KEY_NUMPAD_ENTER:
                    if (data.editing)
                    {
                        if (is_multiline())
                        {
                            insert_text("\n");
                            on_edit();
                        }
                        else
                        {
                            if (data.text_edited)
                            {
                                value.set(data.text);
                                result.new_value = data.text;
                                result.changed = true;
                            }
                            if ((flags & ALWAYS_EDITING) == 0)
                                exit_edit_mode();
                            result.event = TEXT_CONTROL_ENTER_PRESSED;
                        }
                    }
                    else
                        data.editing = true;
                    acknowledge_key();
                    break;

                 case KEY_ESCAPE:
                    reset_to_external_value();
                    result.event = TEXT_CONTROL_EDIT_CANCELED;
                    acknowledge_input_event(ctx);
                    break;

                 case KEY_BACKSPACE:
                    if (data.editing)
                    {
                        if (has_selection())
                        {
                            delete_selection();
                        }
                        else if (data.cursor_position > 0)
                        {
                            data.text =
                                data.text.substr(0, data.cursor_position - 1) +
                                data.text.substr(data.cursor_position);
                            --data.cursor_position;
                        }
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case KEY_DELETE:
                    if (data.editing)
                    {
                        if (has_selection())
                        {
                            delete_selection();
                        }
                        else if (data.cursor_position <
                            int(data.text.length()))
                        {
                            data.text =
                                data.text.substr(0, data.cursor_position) +
                                data.text.substr(data.cursor_position + 1);
                        }
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case KEY_LEFT:
                    move_cursor(data.cursor_position - 1);
                    acknowledge_key();
                    break;

                 case KEY_RIGHT:
                    move_cursor(data.cursor_position + 1);
                    acknowledge_key();
                    break;

                 case KEY_UP:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        move_cursor(get_vertically_adjusted_position(-1),
                            false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_DOWN:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        move_cursor(get_vertically_adjusted_position(1),
                            false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEUP:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        move_cursor(get_vertically_adjusted_position(
                            -(data.assigned_region.size[1] /
                            data.font_image->metrics.height - 1)), false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEDOWN:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        move_cursor(get_vertically_adjusted_position(
                            data.assigned_region.size[1] /
                            data.font_image->metrics.height - 1), false);
                        acknowledge_key();
                    }
                    break;

                 default:
                    ;
                }
                break;

             case KMOD_CTRL_CODE:
                switch (info.code)
                {
                 case 'a':
                    select_all();
                    acknowledge_key();
                    break;

                 case 'c':
                 case KEY_INSERT:
                    copy();
                    acknowledge_key();
                    break;

                 case 'x':
                    if (data.editing)
                    {
                        cut();
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case 'v':
                    if (data.editing)
                    {
                        paste();
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case KEY_HOME:
                    move_cursor(0);
                    acknowledge_key();
                    break;

                 case KEY_END:
                    move_cursor(int(data.text.length()));
                    acknowledge_key();
                    break;

                 case KEY_DELETE:
                    if (data.editing)
                    {
                        delete_selection();
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case KEY_LEFT:
                    move_cursor(get_previous_word_boundary(
                        data.cursor_position));
                    acknowledge_key();
                    break;

                 case KEY_RIGHT:
                    move_cursor(get_next_word_boundary(
                        data.cursor_position));
                    acknowledge_key();
                    break;

                 default:
                    ;
                }
                break;

             case KMOD_SHIFT_CODE:
                switch (info.code)
                {
                 case KEY_HOME:
                    shift_move_cursor(get_home_position());
                    acknowledge_key();
                    break;

                 case KEY_END:
                    shift_move_cursor(get_end_position());
                    acknowledge_key();
                    break;

                 case KEY_INSERT:
                    if (data.editing)
                    {
                        paste();
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case KEY_DELETE:
                    if (data.editing)
                    {
                        cut();
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case KEY_LEFT:
                    shift_move_cursor(data.cursor_position - 1);
                    acknowledge_key();
                    break;

                 case KEY_RIGHT:
                    shift_move_cursor(data.cursor_position + 1);
                    acknowledge_key();
                    break;

                 case KEY_UP:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        shift_move_cursor(get_vertically_adjusted_position(-1),
                            false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_DOWN:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        shift_move_cursor(get_vertically_adjusted_position(1),
                            false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEUP:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        shift_move_cursor(get_vertically_adjusted_position(
                            -(data.assigned_region.size[1] /
                            data.font_image->metrics.height - 1)), false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEDOWN:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        shift_move_cursor(get_vertically_adjusted_position(
                            data.assigned_region.size[1] /
                            data.font_image->metrics.height - 1), false);
                        acknowledge_key();
                    }
                    break;

                 default:
                    ;
                }
                break;

             case KMOD_SHIFT_CODE | KMOD_CTRL_CODE:
                switch (info.code)
                {
                 case KEY_HOME:
                    shift_move_cursor(0);
                    acknowledge_key();
                    break;

                 case KEY_END:
                    shift_move_cursor(int(data.text.length()));
                    acknowledge_key();
                    break;

                 case KEY_LEFT:
                    shift_move_cursor(get_previous_word_boundary(
                        data.cursor_position));
                    acknowledge_key();
                    break;

                 case KEY_RIGHT:
                    shift_move_cursor(get_next_word_boundary(
                        data.cursor_position));
                    acknowledge_key();
                    break;

                 default:
                    ;
                }
                break;
            }
        }
    }

    // Call this after any key press.
    void acknowledge_key()
    {
        reset_cursor_blink();
        acknowledge_input_event(ctx);
        ensure_cursor_visible();
    }

    void ensure_cursor_visible()
    {
        if (data.need_layout)
        {
            data.force_cursor_visible = true;
            return;
        }
        make_widget_visible(ctx, cursor_id);
        data.force_cursor_visible = false;
    }

    // Reset the cursor blink so that it's visible.
    void reset_cursor_blink()
    {
        data.cursor_on = true;
        start_timer(ctx, cursor_id, cursor_blink_delay);
    }

    void on_text_change()
    {
        data.true_cursor_x = -1;
        data.need_layout = true;
    }

    void on_edit()
    {
        on_text_change();
        data.text_edited = true;
    }

    void exit_edit_mode()
    {
        data.editing = false;
        data.n_selected = 0;
        data.cursor_on = false;
    }

    // Get the number of the line that contains the given character index.
    unsigned get_line_number(cached_text::offset char_i)
    {
        return data.renderer->get_line_number(char_i);
    }

    string sanitize(string const& text)
    {
        string r;
        r.reserve(text.length());
        for (unsigned i = 0; i < text.length(); ++i)
        {
            char c = text[i];
            if (std::isprint(unsigned char(c)) || c == '\n')
                r.push_back(c);
        }
        return r;
    }

    // Insert text at the current cursor position.
    void insert_text(string const& text)
    {
        string sanitized = sanitize(text);
        if (max_chars < 0 ||
            int(data.text.length() + sanitized.length() - data.n_selected)
            <= max_chars)
        {
            delete_selection();
            data.text = data.text.substr(0, data.cursor_position) +
                sanitized + data.text.substr(data.cursor_position);
            data.cursor_position += cached_text::offset(sanitized.length());
        }
    }

    // Move the cursor to the given position.
    void move_cursor(int new_position, bool reset_x = true)
    {
        data.cursor_position = clamp(new_position, 0,
            int(data.text.length()));
        data.n_selected = 0;

        if (reset_x)
            data.true_cursor_x = -1;
    }

    // Move the cursor, manipulating the selection in the process.
    void shift_move_cursor(int new_position, bool reset_x = true)
    {
        new_position = clamp(new_position, 0, int(data.text.length()));

        int selection_end = data.first_selected + data.n_selected;

        if (has_selection() && data.cursor_position == data.first_selected)
            set_selection(new_position, selection_end);
        else if (has_selection() && data.cursor_position == selection_end)
            set_selection(data.first_selected, new_position);
        else
            set_selection(data.cursor_position, new_position);

        data.cursor_position = new_position;

        if (reset_x)
            data.true_cursor_x = -1;
    }

    // Set the current selection.
    void set_selection(int from, int to)
    {
        if (from > to)
            std::swap(from, to);
        data.first_selected = from;
        data.n_selected = to - from;
    }

    // Select all text.
    void select_all()
    {
        data.first_selected = 0;
        data.cursor_position = data.n_selected = int(data.text.length());
    }

    // Is there currently any text selected?
    bool has_selection() const
    {
        return data.n_selected != 0;
    }

    // Delete the current selection.
    void delete_selection()
    {
        if (has_selection())
        {
            data.text = data.text.substr(0, data.first_selected) +
                data.text.substr(data.first_selected + data.n_selected);
            data.cursor_position = data.first_selected;
            data.n_selected = 0;
        }
    }

    // Copy the current selection to the clipboard.
    void copy()
    {
        if (has_selection())
        {
            ctx.surface->set_clipboard_text(
                data.text.substr(data.first_selected, data.n_selected));
        }
    }

    // Cut the current selection.
    void cut()
    {
        copy();
        delete_selection();
    }

    // Paste the current clipboard contents into the control.
    void paste()
    {
        insert_text(ctx.surface->get_clipboard_text());
    }

    // Get the position that the home key should go to.
    int get_home_position()
    {
        return get_line_begin(get_line_number(data.cursor_position));
    }

    // Get the position that the end key should go to.
    int get_end_position()
    {
        return get_line_end(get_line_number(data.cursor_position));
    }

    // Get the character index that corresponds to the cursor position shifted
    // down by delta lines (a negative delta shifts up).
    int get_vertically_adjusted_position(int delta)
    {
        unsigned line_n = get_line_number(data.cursor_position);
        if (data.true_cursor_x < 0)
        {
            data.true_cursor_x = data.renderer->get_character_position(
                data.cursor_position)[0];
        }
        line_n = unsigned(clamp(int(line_n) + delta, 0,
            int(data.renderer->get_line_count()) - 1));
        return data.renderer->get_character_boundary_at_point(make_vector<int>(
            data.true_cursor_x,
            data.renderer->get_character_position(get_line_begin(line_n))[1]));
    }

    // Get the index of the character that contains the given pixel.
    // Will return invalid character indices if the pixel is not actually
    // inside a character.
    int get_character_at_pixel(vector<2,int> const& p)
    {
        return data.renderer->get_character_at_point(
            vector<2,int>(p - data.assigned_region.corner));
    }

    int get_line_begin(int line_n)
    {
        return data.renderer->get_line_begin(line_n);
    }

    int get_line_end(int line_n)
    {
        return data.renderer->get_line_end(line_n);
    }

    // Get the index of the character that begins closest to the given pixel.
    int get_character_boundary_at_pixel(vector<2,int> const& p)
    {
        return data.renderer->get_character_boundary_at_point(
            vector<2,int>(p - data.assigned_region.corner));
    }

    // Get the screen location of the character boundary immediately before the
    // given character index.
    vector<2,int> get_character_boundary_location(cached_text::offset char_i)
    {
        return data.renderer->get_character_position(char_i) +
            vector<2,int>(data.assigned_region.corner);
    }

    // Get the index of the word boundary immediately before the given
    // character index.
    int get_previous_word_boundary(int char_i)
    {
        int n = char_i;
        while (n > 0 && !std::isalnum(data.renderer->get_text()[n - 1]))
            --n;
        while (n > 0 && std::isalnum(data.renderer->get_text()[n - 1]))
            --n;
        return n;
    }

    // Get the index of the word boundary immediately after the given
    // character index.
    int get_next_word_boundary(int char_i)
    {
        int n = char_i;
        while (n < int(data.renderer->get_text().length()) &&
            std::isalnum(data.renderer->get_text()[n]))
        {
            ++n;
        }
        while (n < int(data.renderer->get_text().length()) &&
            !std::isalnum(data.renderer->get_text()[n]))
        {
            ++n;
        }
        return n;
    }

    ui_context& ctx;
    text_control_data& data;
    accessor<string> const& value;
    ui_flag_set flags;
    layout const& layout_spec;
    widget_id id, cursor_id;
    int max_chars;
    static int const cursor_blink_delay = 500;
    static int const drag_delay = 40;
    panel panel_;
};

text_control_result<string>
do_text_control(
    ui_context& ctx,
    accessor<string> const& value,
    layout const& layout_spec,
    ui_flag_set flags,
    widget_id id,
    int max_chars)
{
    if (!id) id = get_widget_id(ctx);
    text_control_data* data;
    get_data(ctx, &data);
    text_control tc(ctx, *data, value, layout_spec, flags, id, max_chars);
    tc.do_pass();
    return tc.result;
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
    draggable_separator_data* data;
    get_cached_data(ctx, &data);

    get_widget_id_if_needed(ctx, id);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        refresh_keyed_data(data->width, *ctx.style.id);
        if (!data->width.is_valid)
            set(data->width, get_float_property(ctx, "separator_width", 2));
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec,
            leaf_layout_requirements(
                make_layout_vector(
                    as_layout_size(data->width.value),
                    as_layout_size(data->width.value)),
                0, 0),
            FILL | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        if (!is_rendering_active(ctx))
            break;
        layout_box const& region = data->layout_node.assignment().region;
        caching_renderer cache(ctx, data->rendering, *ctx.style.id, region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), region.size);
            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);
            paint.setStrokeWidth(2);
            paint.setStrokeCap(SkPaint::kRound_Cap);
            set_color(paint, get_color_property(ctx, "separator_color"));
            renderer.canvas().drawLine(
                SkScalar(1), SkScalar(1),
                SkScalar(region.size[0] - 1), SkScalar(region.size[1] - 1),
                paint);
            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
        break;
      }

     case REGION_CATEGORY:
      {
        layout_box region = data->layout_node.assignment().region;
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
            data->drag_start_delta = (flags & FLIPPED) ?
                current_width + position : position - current_width;
        }
        if (detect_drag(ctx, id, LEFT_BUTTON))
        {
            int position = get_integer_mouse_position(ctx)[drag_axis];
            set(width, (flags & FLIPPED) ?
                data->drag_start_delta - position :
                position - data->drag_start_delta);
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
    if (detect_event(ctx, HIT_TEST_EVENT))
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
        layout_.end();
        if (!(flags_ & PREPEND) && !ctx_->pass_aborted)
        {
            if (do_draggable_separator(*ctx_, inout(&size_), UNPADDED, flags_))
                issue_set_value_event(*ctx_, id_, size_);
        }
        ctx_ = 0;
    }
}

struct layout_dependent_text_data
{
    layout_leaf layout_node;
    cached_image_ptr cached_image;
};

void do_layout_dependent_text(ui_context& ctx, int n,
    layout const& layout_spec)
{
    layout_dependent_text_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec,
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        if (!is_rendering_active(ctx))
            break;
        layout_box const& region = data->layout_node.assignment().region;
        ascii_font_image const* font_image =
            get_ascii_font_image(ctx.style.properties->font,
                rgba8(0xff, 0xff, 0xff, 0xff),
                rgba8(0x00, 0x00, 0x00, 0x00));
        char text[64];
        sprintf(text, "%i", n);
        draw_ascii_text(*ctx.surface, font_image, data->cached_image,
            vector<2,double>(region.corner), text,
            ctx.style.properties->text_color);
        break;
      }
    }
}

}
