#include <alia/indie.hpp>

#include <color/color.hpp>

#include <cmath>

#include <include/core/SkPictureRecorder.h>

using namespace alia;

struct box_node : indie::leaf_widget
{
    void
    render(SkCanvas& canvas) override
    {
        auto const& region = this->assignment().region;

        // SkPaint paint;
        // paint.setColor(color_);
        // SkRect rect;
        // rect.fLeft = SkScalar(region.corner[0]);
        // rect.fTop = SkScalar(region.corner[1]);
        // rect.fRight = SkScalar(region.corner[0] + region.size[0]);
        // rect.fBottom = SkScalar(region.corner[1] + region.size[1]);
        // canvas.drawRect(rect, paint);

        canvas.save();
        canvas.translate(
            SkScalar(region.corner[0]), SkScalar(region.corner[1]));
        canvas.drawPicture(picture_.get());

        canvas.restore();
    }

    void
    handle_region_event(indie::region_event& event) override
    {
        switch (event.type)
        {
            case indie::region_event_type::MOUSE_HIT_TEST: {
                auto& hit_test
                    = static_cast<indie::mouse_hit_test_event&>(event);
                if (is_inside(
                        this->assignment().region,
                        vector<2, float>(hit_test.parameters.mouse_position)))
                {
                    hit_test.response = indie::mouse_hit_response{
                        null_component_id,
                        indie::mouse_cursor::POINTER,
                        this->assignment().region,
                        ""};
                }
                break;
            }
        }
    }

    SkColor color_ = SK_ColorWHITE;
    sk_sp<SkPicture> picture_;
};

void
do_box(indie::context ctx, SkColor color)
{
    auto& node = get_cached_data<box_node>(ctx);
    if (is_refresh_event(ctx))
    {
        add_widget(get<indie::traversal_tag>(ctx).widgets, &node);
        node.refresh_layout(
            get<indie::traversal_tag>(ctx).layout,
            alia::layout(TOP | LEFT | PADDED),
            alia::leaf_layout_requirements(
                alia::make_layout_vector(100, 100), 0, 0));
        add_layout_node(get<indie::traversal_tag>(ctx).layout, &node);

        if (color != node.color_)
        {
            SkPictureRecorder recorder;
            SkRect bounds;
            bounds.fLeft = 0;
            bounds.fTop = 0;
            bounds.fRight = 100;
            bounds.fBottom = 100;
            SkCanvas* canvas = recorder.beginRecording(bounds);

            {
                SkPaint paint;
                paint.setColor(color);
                SkRect rect;
                rect.fLeft = 0;
                rect.fTop = 0;
                rect.fRight = 100;
                rect.fBottom = 100;
                canvas->drawRect(rect, paint);
            }

            node.picture_ = recorder.finishRecordingAsPicture();
            node.color_ = color;
        }
    }
}

namespace alia { namespace indie {

layout_traversal&
get_layout_traversal(context ctx)
{
    return get<indie::traversal_tag>(ctx).layout;
}

bool
region_event_is_relevant(
    region_event const& event, box<2, float> const& region)
{
    switch (event.type)
    {
        case region_event_type::MOUSE_HIT_TEST:
            return is_inside(
                region,
                vector<2, float>(
                    static_cast<mouse_hit_test_event const&>(event)
                        .parameters.mouse_position));
        // case region_event_type::WHEEL_HIT_TEST:
        //     return is_inside(
        //         region,
        //         static_cast<wheel_hit_test_event const&>(event)
        //             .parameters.position);
        // case region_event_type::WIDGET_VISIBILITY:
        //     return overlapping(region,
        //     static_cast<widget_visibility_event
        //     const&>(event).parameters.region);
        default:
            return false;
    }
}

template<class LayoutContainer>
struct layout_container_widget : widget_container, LayoutContainer
{
    void
    render(SkCanvas& canvas) override
    {
        indie::render_children(canvas, *this);
    }

    void
    handle_region_event(region_event& event) override
    {
        auto region = this->LayoutContainer::region();
        if (region_event_is_relevant(event, region))
        {
            for (widget* node = this->widget_container::children; node;
                 node = node->next)
                node->handle_region_event(event);
        }
    }
};

struct simple_container_widget : widget_container
{
    void
    render(SkCanvas& canvas) override
    {
        indie::render_children(canvas, *this);
    }

    void
    handle_region_event(region_event& event) override
    {
        for (widget* node = this->widget_container::children; node;
             node = node->next)
        {
            node->handle_region_event(event);
        }
    }
};

}} // namespace alia::indie

void
my_ui(indie::context ctx)
{
    // static indie::layout_container_widget<column_layout> container;
    static indie::simple_container_widget container;
    indie::scoped_widget_container container_scope;
    if (is_refresh_event(ctx))
    {
        container_scope.begin(
            get<indie::traversal_tag>(ctx).widgets, &container);
    }
    row_layout row(ctx);
    do_box(ctx, SK_ColorMAGENTA);

    color::yiq<std::uint8_t> y1 = ::color::constant::blue_t{};
    color::yiq<std::uint8_t> y2 = ::color::constant::red_t{};
    color::yiq<std::uint8_t> yr = color::operation::mix(
        y1,
        std::max(
            0.0,
            std::min(
                1.0,
                std::fabs(
                    std::sin(get_raw_animation_tick_count(ctx) / 1000.0)))),
        y2);
    color::rgb<std::uint8_t> r(yr);
    // color::rgb<std::uint8_t> r(y1);

    do_box(
        ctx,
        SkColorSetARGB(
            0xff,
            ::color::get::red(r),
            ::color::get::green(r),
            ::color::get::blue(r)));

    static SkColor clicky_color = SK_ColorRED;
    event_handler<indie::mouse_click_event>(
        ctx, [&](auto, auto&) { clicky_color = SK_ColorBLUE; });
    do_box(ctx, clicky_color);
}
