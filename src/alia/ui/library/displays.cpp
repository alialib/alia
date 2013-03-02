#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

// SEPARATOR

struct separator_data
{
    keyed_data<layout_vector> size;
    keyed_data<absolute_size> padding;
    layout_leaf layout_node;
    caching_renderer_data rendering;
};

void do_separator(ui_context& ctx, layout const& layout_spec)
{
    ALIA_GET_CACHED_DATA(separator_data)

    if (is_refresh_pass(ctx))
    {
        refresh_keyed_data(data.size, *ctx.style.id);
        if (!is_valid(data.size))
        {
            absolute_length spec =
                get_property(ctx, "separator-width", INHERITED_PROPERTY,
                    absolute_length(1, PIXELS));
            set(data.size, as_layout_size(make_vector(
                resolve_absolute_length(get_layout_traversal(ctx), 0, spec),
                resolve_absolute_length(get_layout_traversal(ctx), 1, spec))));
        }
        refresh_keyed_data(data.padding, *ctx.style.id);
        if (!is_valid(data.padding))
        {
            absolute_length spec =
                get_property(ctx, "separator-padding", INHERITED_PROPERTY,
                    absolute_length(0, PIXELS));
            set(data.padding, make_vector(spec, spec));
        }
    }

    do_spacer(ctx, layout(get(data.padding), UNPADDED));

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec,
            leaf_layout_requirements(get(data.size), 0, 0),
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
            set_color(paint, get_color_property(ctx, "separator-color"));
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

    do_spacer(ctx, layout(get(data.padding), UNPADDED));
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
            set_color(paint, is_gettable(color) ? get(color) :
                rgba8(0x00, 0x00, 0x00, 0x00));
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

void do_color(ui_context& ctx, getter<rgb8> const& color,
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

            style_path_storage storage;
            style_search_path const* path =
                add_substyle_to_path(&storage, ctx.style.path, 0,
                    "progress-bar");

            rgba8 outline_color = get_color_property(path, "outline-color");
            rgba8 background_color =
                get_color_property(path, "background");
            rgba8 bar_color = get_color_property(path, "bar-color");

            SkScalar trim =
                SkScalarDiv(
                    layout_scalar_as_skia_scalar(region.size[1]),
                    SkDoubleToScalar(12.));

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
