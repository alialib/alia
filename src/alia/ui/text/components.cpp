#include <alia/ui/text/components.hpp>

#include <alia/ui/events.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/text/fonts.hpp>
#include <alia/ui/text/shaping.hpp>
#include <alia/ui/utilities/rendering.hpp>

#ifdef _WIN32
#pragma warning(push, 0)
#endif

#include <include/core/SkCanvas.h>

#ifdef _WIN32
#pragma warning(pop)
#endif

namespace alia {

// TODO: Move elsewhere.
inline SkColor
as_skia_color(rgba8 color)
{
    return SkColorSetARGB(color.a, color.r, color.g, color.b);
}

inline SkColor
as_skia_color(rgb8 color)
{
    return SkColorSetARGB(0xff, color.r, color.g, color.b);
}

struct text_data
{
    layout_leaf layout_node;
    captured_id text_id;
    SkFont* font;
    std::optional<ShapeResult> shape;
};

void
do_text(
    ui_context ctx,
    readable<text_style> style,
    readable<std::string> text,
    layout const& layout_spec)
{
    text_data* data_ptr;
    if (get_cached_data(ctx, &data_ptr))
    {
        // TODO: This isn't necessarily readable right now.
        // TODO: Need to track changes in this.
        data_ptr->font = &get_font(
            read_signal(style).font_name, read_signal(style).font_size);
    }
    auto& data = *data_ptr;

    alia_untracked_switch(get_event_category(ctx))
    {
        case REFRESH_CATEGORY:
            refresh_signal_view(
                data.text_id,
                text,
                [&](std::string const& new_value) {
                    data.shape = Shape(
                        new_value.c_str(),
                        new_value.size(),
                        *data.font,
                        std::numeric_limits<SkScalar>::max());
                },
                [&] {
                    data.shape = Shape(
                        "",
                        0,
                        *data.font,
                        std::numeric_limits<SkScalar>::max());
                });

            data.layout_node.refresh_layout(
                get_layout_traversal(ctx),
                layout_spec,
                leaf_layout_requirements(
                    data.shape
                        ? make_layout_vector(
                              layout_scalar(data.shape->width),
                              layout_scalar(data.shape->verticalAdvance))
                        : make_layout_vector(0, 0),
                    0, // TODO: ascent/descent
                    0),
                LEFT | BASELINE_Y | PADDED);

            add_layout_node(
                get<ui_traversal_tag>(ctx).layout, &data.layout_node);

            break;

        case REGION_CATEGORY:
            break;

        case INPUT_CATEGORY:
            break;

        case RENDER_CATEGORY: {
            auto& event = cast_event<render_event>(ctx);

            SkCanvas& canvas = *event.canvas;

            auto const& region = data.layout_node.assignment().region;

            SkRect bounds;
            bounds.fLeft = SkScalar(region.corner[0]);
            bounds.fTop = SkScalar(region.corner[1]);
            bounds.fRight = SkScalar(region.corner[0] + region.size[0]);
            bounds.fBottom = SkScalar(region.corner[1] + region.size[1]);

            if (canvas.quickReject(bounds))
                break;

            SkPaint paint;
            paint.setAntiAlias(true);
            // Note that this isn't necessarily readable yet.
            auto color = as_skia_color(read_signal(style).color);
            paint.setColor(color);

            canvas.drawTextBlob(
                data.shape->blob.get(), bounds.fLeft, bounds.fTop, paint);
        }
    }
    alia_end
}

struct wrapped_text_node : layout_node
{
    layout_requirements
    get_horizontal_requirements() override
    {
        return cache_horizontal_layout_requirements(cacher, [&] {
            // TODO: What should the actual minimum width be here?
            return calculated_layout_requirements{12, 0, 0};
        });
    }
    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width) override
    {
        return cache_vertical_layout_requirements(cacher, assigned_width, [&] {
            if (this->shape_width != assigned_width)
            {
                this->shape = Shape(
                    this->text.c_str(),
                    this->text.size(),
                    *this->font,
                    assigned_width);
                this->shape_width = assigned_width;
            }

            return calculated_layout_requirements{
                layout_scalar(this->shape.verticalAdvance),
                0 /* TODO: ascent */,
                0 /* TODO: descent */};
        });
    }
    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override
    {
        update_relative_assignment(
            *this, this->cacher, assignment, [&](auto const&) {});
    }

    captured_id text_id;
    std::string text;

    layout_cacher cacher;

    double shape_width = 0;
    ShapeResult shape;

    SkFont* font;
};

void
do_wrapped_text(
    ui_context ctx,
    readable<text_style> style,
    readable<std::string> text,
    layout const& layout_spec)
{
    wrapped_text_node* node_ptr;
    if (get_cached_data(ctx, &node_ptr))
    {
        // TODO: This isn't necessarily readable right now.
        node_ptr->text = read_signal(text);
        // TODO: This also isn't necessarily readable right now.
        // TODO: Need to track changes in this.
        node_ptr->font = &get_font(
            read_signal(style).font_name, read_signal(style).font_size);
    }

    auto& node = *node_ptr;

    alia_untracked_switch(get_event_category(ctx))
    {
        case REFRESH_CATEGORY:
            refresh_signal_view(
                node.text_id,
                text,
                [&](std::string const& new_value) { node.text = new_value; },
                [&] { node.text.clear(); },
                [&] {
                    record_layout_change(get_layout_traversal(ctx));
                    node.shape_width = 0;
                    node.shape = ShapeResult();
                    invalidate_cached_layout(node.cacher);
                });

            update_layout_cacher(
                get_layout_traversal(ctx),
                node.cacher,
                layout_spec,
                TOP | LEFT | PADDED);

            add_layout_node(get<ui_traversal_tag>(ctx).layout, &node);

            break;

        case REGION_CATEGORY:
            break;

        case INPUT_CATEGORY:
            break;

        case RENDER_CATEGORY: {
            auto& event = cast_event<render_event>(ctx);

            SkCanvas& canvas = *event.canvas;

            auto const& region = node.cacher.relative_assignment.region;

            SkRect bounds;
            bounds.fLeft = SkScalar(region.corner[0]);
            bounds.fTop = SkScalar(region.corner[1]);
            bounds.fRight = bounds.fLeft + SkScalar(region.size[0]);
            bounds.fBottom = bounds.fTop + SkScalar(region.size[1]);
            if (canvas.quickReject(bounds))
                return;

            SkPaint paint;
            paint.setAntiAlias(true);
            // Note that this isn't necessarily readable yet.
            auto color = as_skia_color(read_signal(style).color);
            paint.setColor(color);

            if (node.shape_width != region.size[0])
            {
                node.shape = Shape(
                    node.text.c_str(),
                    node.text.size(),
                    *node.font,
                    region.size[0]);
                node.shape_width = region.size[0];
            }
            canvas.drawTextBlob(
                node.shape.blob.get(), bounds.fLeft, bounds.fTop, paint);
        }
    }
    alia_end
}

} // namespace alia
