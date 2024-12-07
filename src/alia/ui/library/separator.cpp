// #include <alia/ui/library/separator.hpp>

// #include <alia/ui/events.hpp>
// #include <alia/ui/layout/utilities.hpp>

// namespace alia {

// struct separator_data
// {
//     layout_leaf layout_node;
// };

// void
// do_separator(ui_context ctx, layout const& layout_spec)
// {
//     auto& data = get_cached_data<separator_data>(ctx);

//     switch (get_event_category(ctx))
//     {
//         case REFRESH_CATEGORY: {
//             data.layout_node.refresh_layout(
//                 get_layout_traversal(ctx),
//                 layout_spec,
//                 leaf_layout_requirements(make_layout_vector(2, 2), 0, 0),
//                 FILL | PADDED);
//             add_layout_node(get_layout_traversal(ctx), &data.layout_node);
//             break;
//         }

//         case RENDER_CATEGORY: {
//             layout_box const& region = data.layout_node.assignment().region;
//             caching_renderer cache(ctx, data.rendering, *ctx.style.id,
//             region); if (cache.needs_rendering())
//             {
//                 skia_renderer renderer(ctx, cache.image(), region.size);
//                 SkPaint paint;
//                 paint.setFlags(SkPaint::kAntiAlias_Flag);
//                 SkScalar stroke_width
//                     =
//                     layout_scalar_as_skia_scalar(get(data.metrics).size[0]);
//                 SkScalar half_stroke_width = stroke_width / 2;
//                 paint.setStrokeWidth(stroke_width);
//                 paint.setStrokeCap(SkPaint::kSquare_Cap);
//                 style_path_storage storage;
//                 style_search_path const* path = add_substyle_to_path(
//                     &storage, ctx.style.path, ctx.style.path, "separator");
//                 set_color(paint, get_color_property(path, "color"));
//                 renderer.canvas().drawLine(
//                     half_stroke_width,
//                     half_stroke_width,
//                     layout_scalar_as_skia_scalar(region.size[0])
//                         - half_stroke_width,
//                     layout_scalar_as_skia_scalar(region.size[1])
//                         - half_stroke_width,
//                     paint);
//                 renderer.cache();
//                 cache.mark_valid();
//             }
//             cache.draw();
//             break;
//         }
//     }

//     do_spacer(ctx, layout(get(data.metrics).padding, UNPADDED));
// }

// } // namespace alia
