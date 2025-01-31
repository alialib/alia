#include <alia/ui/library/separator.hpp>

#include <alia/ui/events.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/system/object.hpp>
#include <alia/ui/utilities/rendering.hpp>
#include <alia/ui/utilities/skia.hpp>

#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>
#include <include/core/SkRect.h>

namespace alia {

struct separator_data
{
    layout_leaf layout_node;
};

void
do_separator(ui_context ctx, layout const& layout_spec)
{
    auto& data = get_cached_data<separator_data>(ctx);

    switch (get_event_category(ctx))
    {
        case REFRESH_CATEGORY: {
            data.layout_node.refresh_layout(
                get_layout_traversal(ctx),
                layout_spec,
                leaf_layout_requirements(make_layout_vector(2, 2), 0, 0),
                FILL | PADDED);
            add_layout_node(get_layout_traversal(ctx), &data.layout_node);
            break;
        }

        case RENDER_CATEGORY: {
            auto& event = cast_event<render_event>(ctx);

            SkCanvas& canvas = *event.canvas;

            auto const& region = data.layout_node.assignment().region;

            SkRect rect;
            rect.fLeft = SkScalar(region.corner[0]);
            rect.fTop = SkScalar(region.corner[1]);
            rect.fRight = SkScalar(region.corner[0] + region.size[0]);
            rect.fBottom = SkScalar(region.corner[1] + region.size[1]);

            if (event.canvas->quickReject(rect))
                break;

            {
                SkPaint paint;
                paint.setAntiAlias(true);
                paint.setColor(
                    as_skcolor(get_system(ctx).theme.structural.base.main));
                paint.setStyle(SkPaint::kFill_Style);
                // paint.setStrokeCap(SkPaint::kRound_Cap);
                // paint.setStrokeWidth(16);
                canvas.drawPath(SkPath::Rect(rect), paint);
            }

            break;
        }
    }
}

} // namespace alia
