#include <alia/ui/library/bullets.hpp>

#include <alia/ui/color.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/layout/spacer.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/utilities.hpp>
#include <alia/ui/utilities/skia.hpp>

#include <include/core/SkBlurTypes.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>

namespace alia {

// TODO: Move elsewhere.
void
draw_round_rect(SkCanvas& canvas, SkPaint& paint, layout_box const& region)
{
    SkScalar radius = layout_scalar_as_skia_scalar(
                          (std::min)(region.size[0], region.size[1]))
                      / SkIntToScalar(4);
    canvas.drawRoundRect(as_skrect(region), radius, radius, paint);
}

struct bullet_data
{
    layout_leaf layout_node;
};

void
do_bullet(ui_context ctx, layout const& layout_spec)
{
    auto& data = get_cached_data<bullet_data>(ctx);

    switch (get_event_category(ctx))
    {
        case REFRESH_CATEGORY: {
            layout_scalar size = as_layout_size(resolve_absolute_length(
                get_layout_traversal(ctx), 1, absolute_length(20, PIXELS)));
            data.layout_node.refresh_layout(
                get_layout_traversal(ctx),
                layout_spec,
                leaf_layout_requirements(
                    make_layout_vector(size, size), size, 0),
                CENTER_X | BASELINE_Y | PADDED);
            add_layout_node(get_layout_traversal(ctx), &data.layout_node);
            break;
        }

        case RENDER_CATEGORY: {
            auto& event = cast_event<render_event>(ctx);
            SkCanvas& canvas = *event.canvas;
            layout_box const& region = data.layout_node.assignment().region;
            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setStyle(SkPaint::kFill_Style);
            // TOOD: Get color from style.
            paint.setColor(SkColorSetARGB(0xa0, 0xa0, 0xa0, 0xff));
            draw_round_rect(canvas, paint, region);
            break;
        }
    }
}

void
bulleted_list::begin(ui_context ctx, layout const& layout_spec)
{
    ctx_.reset(ctx);

    grid_.begin(ctx, layout_spec);

    // Add an empty row (with no height) to force the content column to grab
    // any extra space in the grid.
    {
        grid_row r(grid_);
        do_spacer(ctx, layout(size(0, 0, PIXELS), UNPADDED));
        do_spacer(ctx, layout(size(0, 0, PIXELS), UNPADDED | GROW));
    }
}
void
bulleted_list::end()
{
    if (ctx_)
    {
        grid_.end();
        ctx_.reset();
    }
}

void
bulleted_item::begin(bulleted_list& list, layout const& layout_spec)
{
    row_.begin(list.grid_, layout_spec);
    do_bullet(*list.ctx_);
}
void
bulleted_item::end()
{
    row_.end();
}

} // namespace alia
