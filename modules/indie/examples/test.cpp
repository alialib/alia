#include <alia/indie.hpp>

#include <color/color.hpp>

#include <cmath>

using namespace alia;

struct box_node : indie::widget
{
    virtual void
    render(SkCanvas& canvas)
    {
        SkPaint paint;
        paint.setColor(color_);
        auto const& region = this->layout_.assignment().region;
        SkRect rect;
        rect.fLeft = SkScalar(region.corner[0]);
        rect.fTop = SkScalar(region.corner[1]);
        rect.fRight = SkScalar(region.corner[0] + region.size[0]);
        rect.fBottom = SkScalar(region.corner[1] + region.size[1]);
        canvas.drawRect(rect, paint);
    }

    SkColor color_;
    alia::layout_leaf layout_;
};

void
do_box(indie::context ctx, SkColor color)
{
    if (is_refresh_event(ctx))
    {
        auto& node = get_cached_data<box_node>(ctx);
        node.color_ = color;
        add_widget(get<indie::render_traversal_tag>(ctx), &node);
        node.layout_.refresh_layout(
            get<indie::layout_traversal_tag>(ctx),
            alia::layout(TOP | LEFT | PADDED),
            alia::leaf_layout_requirements(
                alia::make_layout_vector(100, 100), 0, 0));
        add_layout_node(get<indie::layout_traversal_tag>(ctx), &node.layout_);
    }
}

namespace alia { namespace indie {

layout_traversal&
get_layout_traversal(context ctx)
{
    return get<indie::layout_traversal_tag>(ctx);
}

}} // namespace alia::indie

struct simple_widget_container : indie::widget_container
{
    void
    render(SkCanvas& canvas)
    {
        indie::render_children(canvas, *this);
    }
};

void
my_ui(indie::context ctx)
{
    static simple_widget_container container;
    indie::scoped_widget_container container_scope;
    if (is_refresh_event(ctx))
    {
        container_scope.begin(
            get<indie::render_traversal_tag>(ctx), &container);
    }
    row_layout row(ctx);
    do_box(ctx, SK_ColorMAGENTA);

    color::yiq<std::uint8_t> y1 = ::color::constant::blue_t{};
    color::yiq<std::uint8_t> y2 = ::color::constant::red_t{};
    color::yiq<std::uint8_t> yr = color::operation::mix(
        y1,
        std::min(
            1.0,
            std::fabs(std::sin(get_raw_animation_tick_count(ctx) / 1000.0))),
        y2);
    color::rgb<std::uint8_t> r(yr);

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
