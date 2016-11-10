#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

// SEPARATOR

struct separator_metrics
{
    layout_vector size;
    absolute_size padding;
};

struct separator_data
{
    keyed_data<separator_metrics> metrics;
    layout_leaf layout_node;
    caching_renderer_data rendering;
};

void do_separator(ui_context& ctx, layout const& layout_spec)
{
    ALIA_GET_CACHED_DATA(separator_data)

    if (is_refresh_pass(ctx))
    {
        refresh_keyed_data(data.metrics, *ctx.style.id);
        if (!is_valid(data.metrics))
        {
            separator_metrics metrics;
            style_path_storage storage;
            style_search_path const* path =
                add_substyle_to_path(&storage, ctx.style.path, 0, "separator");
            absolute_length padding =
                get_property(path, "padding", UNINHERITED_PROPERTY,
                    absolute_length(0, PIXELS));
            metrics.padding = make_vector(padding, padding);
            absolute_length width =
                get_property(path, "width", UNINHERITED_PROPERTY,
                    absolute_length(1, PIXELS));
            metrics.size = as_layout_size(make_vector(
                resolve_absolute_length(get_layout_traversal(ctx), 0, width),
                resolve_absolute_length(get_layout_traversal(ctx), 1, width)));
            set(data.metrics, metrics);
        }
    }

    do_spacer(ctx, layout(get(data.metrics).padding, UNPADDED));

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec,
            leaf_layout_requirements(get(data.metrics).size, 0, 0),
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
            SkScalar stroke_width =
                layout_scalar_as_skia_scalar(get(data.metrics).size[0]);
            SkScalar half_stroke_width = stroke_width / 2;
            paint.setStrokeWidth(stroke_width);
            paint.setStrokeCap(SkPaint::kSquare_Cap);
            style_path_storage storage;
            style_search_path const* path =
                add_substyle_to_path(&storage, ctx.style.path, ctx.style.path, "separator");
            set_color(paint, get_color_property(path, "color"));
            renderer.canvas().drawLine(
                half_stroke_width, half_stroke_width,
                layout_scalar_as_skia_scalar(region.size[0]) -
                    half_stroke_width,
                layout_scalar_as_skia_scalar(region.size[1]) -
                    half_stroke_width,
                paint);
            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
        break;
      }
    }

    do_spacer(ctx, layout(get(data.metrics).padding, UNPADDED));
}

// COLOR

struct color_metrics
{
    layout_vector size;
    layout_scalar descent;
};

struct color_display_data
{
    keyed_data<color_metrics> metrics;
    layout_leaf layout_node;
    caching_renderer_data rendering;
};

void do_color(ui_context& ctx, accessor<rgba8> const& color,
    layout const& layout_spec)
{
    ALIA_GET_CACHED_DATA(color_display_data)

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        refresh_keyed_data(data.metrics, *ctx.style.id);
        if (!is_valid(data.metrics))
        {
            color_metrics metrics;
            style_path_storage storage;
            style_search_path const* path =
                add_substyle_to_path(&storage, ctx.style.path, 0,
                    "color-display");
            metrics.size =
                as_layout_size(
                    resolve_absolute_size(get_layout_traversal(ctx),
                        get_property(path, "size", UNINHERITED_PROPERTY,
                            make_vector(
                                absolute_length(1.4f, EM),
                                absolute_length(1.4f, EM)))));
            metrics.descent =
                as_layout_size(
                    resolve_absolute_length(get_layout_traversal(ctx), 0,
                        get_property(path, "descent", UNINHERITED_PROPERTY,
                            absolute_length(0, PIXELS))));
            set(data.metrics, metrics);
        }
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(get(data.metrics).size,
                get(data.metrics).size[1] - get(data.metrics).descent,
                get(data.metrics).descent));
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
            set_color(paint, is_gettable(color) ? get(color) :
                rgba8(0x00, 0x00, 0x00, 0x00));
            SkRect rect;
            rect.fLeft = 0;
            rect.fRight = layout_scalar_as_skia_scalar(region.size[0]);
            rect.fTop = 0;
            rect.fBottom = layout_scalar_as_skia_scalar(region.size[1]);
            style_path_storage storage;
            style_search_path const* path =
                add_substyle_to_path(&storage, ctx.style.path, 0,
                    "color-display");
            resolved_box_corner_sizes border_radii =
                resolve_box_corner_sizes(get_layout_traversal(ctx),
                    get_border_radius_property(path, relative_length(0.3f)),
                    vector<2,float>(region.size));
            draw_rect(renderer.canvas(), paint,
                skia_box(
                    make_vector(SkIntToScalar(0), SkIntToScalar(0)),
                    make_vector(
                        layout_scalar_as_skia_scalar(region.size[0]),
                        layout_scalar_as_skia_scalar(region.size[1]))),
                border_radii);
            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
        break;
      }
    }
}

void do_color(ui_context& ctx, accessor<rgb8> const& color,
    layout const& layout_spec)
{
    do_color(ctx,
        in(is_gettable(color) ? rgba8(get(color)) :
            rgba8(0x00, 0x00, 0x00, 0x00)),
        layout_spec);
}

// BULLETED LIST

void do_bullet(ui_context& ctx, layout const& layout_spec)
{
    ALIA_GET_CACHED_DATA(simple_display_data)

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        layout_scalar size = as_layout_size(
            resolve_absolute_length(get_layout_traversal(ctx), 1,
                absolute_length(1, EX)));
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

    // Add an empty row (with no height) to force the content column to grab
    // any extra space in the grid.
    {
        grid_row r(grid_);
        do_spacer(ctx, layout(size(0, 0, PIXELS), UNPADDED));
        do_spacer(ctx, layout(size(0, 0, PIXELS), UNPADDED | GROW));
    }
}
void bulleted_list::end()
{
    if (ctx_)
    {
        grid_.end();
        ctx_ = 0;
    }
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

// PROGRESS BAR
void do_progress_bar(ui_context& ctx, accessor<double> const& progress,
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

            style_path_storage storage;
            style_search_path const* path =
                add_substyle_to_path(&storage, ctx.style.path, 0,
                    "progress-bar");

            rgba8 outline_color = get_color_property(path, "outline-color");
            rgba8 background_color =
                get_color_property(path, "background");
            rgba8 bar_color = get_color_property(path, "bar-color");

            SkScalar trim = 0;
                //SkScalarDiv(
                //    layout_scalar_as_skia_scalar(region.size[1]),
                //    SkDoubleToScalar(12.));

            // The following is kinda iffy with regards to treating SkScalars
            // abstractly, but I think it works in any case.

            skia_box full_box(
                make_vector<SkScalar>(0, 0),
                make_vector<SkScalar>(
                    layout_scalar_as_skia_scalar(region.size[0]),
                    layout_scalar_as_skia_scalar(region.size[1])));

            set_color(paint, outline_color);
            draw_rect(sr.canvas(), paint, full_box);

            set_color(paint, background_color);
            draw_rect(sr.canvas(), paint, add_border(full_box, -trim));

            skia_box bar_box = add_border(full_box, -trim * 2);
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

}
