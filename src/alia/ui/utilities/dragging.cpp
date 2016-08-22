#include <alia/ui/utilities/dragging.hpp>
#include <alia/ui/utilities.hpp>
#include <SkDashPathEffect.h>

namespace alia {

void draw_drop_location(dataless_ui_context& ctx, layout_box const& region,
    caching_renderer_data& renderer_data, draggable_style const& style)
{
    caching_renderer cache(ctx, renderer_data, no_id, region);
    if (cache.needs_rendering())
    {
        skia_renderer renderer(ctx, cache.image(), region.size);
        box<2,float> outline_box =
            add_border(
                box<2,float>(make_vector(0.f, 0.f),
                    vector<2,float>(region.size)),
                -(style.outline_margin + style.outline_width / 2));
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(SkFloatToScalar(style.outline_width));
        SkScalar dashing[2];
        dashing[0] = dashing[1] = SkFloatToScalar(style.outline_dashing);
        paint.setPathEffect(SkDashPathEffect::Make(dashing, 2, 0));
        set_color(paint, style.outline_color);
        draw_rect(renderer.canvas(), paint,
            float_box_as_skia_box(outline_box),
            resolve_box_corner_sizes(get_layout_traversal(ctx),
                style.corner_radii, outline_box.size));
        renderer.cache();
        cache.mark_valid();
    }
    cache.draw();
}

void refresh_draggable_style(dataless_ui_context& ctx,
    keyed_data<draggable_style>& style_data)
{
    if (!is_refresh_pass(ctx))
        return;

    refresh_keyed_data(style_data, *ctx.style.id);
    if (!is_valid(style_data))
    {
        draggable_style style;
        style_path_storage storage;
        style_search_path const* path =
            add_substyle_to_path(&storage, ctx.style.path, 0, "draggable");
        style.outline_width =
            resolve_absolute_length(get_layout_traversal(ctx), 0,
                get_property(path, "outline-width", UNINHERITED_PROPERTY,
                    absolute_length(3.f, PIXELS)));
        style.outline_margin =
            resolve_absolute_length(get_layout_traversal(ctx), 0,
                get_property(path, "outline-margin", UNINHERITED_PROPERTY,
                    absolute_length(0.f, PIXELS)));
        style.outline_dashing =
            resolve_absolute_length(get_layout_traversal(ctx), 0,
                get_property(path, "outline-dashing", UNINHERITED_PROPERTY,
                    absolute_length(3.f, PIXELS)));
        style.outline_color =
            get_color_property(path, "outline-color");
        style.corner_radii = get_border_radius_property(path);
        style.fill_color =
            get_property(path, "fill-color", UNINHERITED_PROPERTY,
                rgba8(0, 0, 0, 0));
        style.fill_size =
            resolve_absolute_length(get_layout_traversal(ctx), 0,
                get_property(path, "fill-size", UNINHERITED_PROPERTY,
                    absolute_length(0.f, PIXELS)));
        set(style_data, style);
    }
}

vector<2,double>
calculate_relative_drag_delta(
    dataless_ui_context& ctx, layout_box const& region)
{
    vector<2,double> absolute_delta =
        get_mouse_position(ctx) - vector<2,double>(region.corner);
    vector<2,double> relative_delta;
    for (unsigned i = 0; i != 2; ++i)
        relative_delta[i] = absolute_delta[i] / region.size[i];
    return relative_delta;
}

vector<2,double>
evaluate_relative_drag_delta(
    dataless_ui_context& ctx, vector<2,double> const& size,
    vector<2,double> const& relative_delta)
{
    vector<2,double> absolute_delta;
    for (unsigned i = 0; i != 2; ++i)
        absolute_delta[i] = relative_delta[i] * size[i];
    return get_mouse_position(ctx) - absolute_delta;
}

}
