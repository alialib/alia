#include <alia/core/flow/data_graph.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/indie.hpp>
#include <alia/indie/system/api.hpp>
#include <alia/indie/system/input_constants.hpp>
#include <alia/indie/utilities/hit_testing.hpp>
#include <alia/indie/utilities/keyboard.hpp>
#include <alia/indie/utilities/mouse.hpp>
#include <alia/indie/widget.hpp>

#include <color/color.hpp>

#include <cmath>

#include <include/core/SkColor.h>
#include <include/core/SkPictureRecorder.h>

using namespace alia;

struct box_node : indie::leaf_widget
{
    void
    render(SkCanvas& canvas) override
    {
        auto const& region = this->assignment().region;

        double blend_factor = 0;

        if (indie::is_click_in_progress(*sys_, this, indie::mouse_button::LEFT)
            || is_pressed(keyboard_click_state_))
        {
            blend_factor = 0.4;
        }
        else if (is_click_possible(*sys_, this))
        {
            blend_factor = 0.2;
        }

        ::color::rgb<std::uint8_t> c;
        if (state_)
        {
            c = ::color::rgb<std::uint8_t>({0x00, 0x00, 0xff});
        }
        else
        {
            c = ::color::rgb<std::uint8_t>(
                {SkColorGetR(color_),
                 SkColorGetG(color_),
                 SkColorGetB(color_)});
        }
        if (blend_factor != 0)
        {
            ::color::yiq<std::uint8_t> color;
            color = c;
            ::color::yiq<std::uint8_t> white = ::color::constant::white_t{};
            ::color::yiq<std::uint8_t> mix
                = ::color::operation::mix(color, blend_factor, white);
            c = mix;
        }

        SkPaint paint;
        paint.setColor(SkColorSetARGB(
            0xff,
            ::color::get::red(c),
            ::color::get::green(c),
            ::color::get::blue(c)));
        SkRect rect;
        rect.fLeft = SkScalar(region.corner[0]);
        rect.fTop = SkScalar(region.corner[1]);
        rect.fRight = SkScalar(region.corner[0] + region.size[0]);
        rect.fBottom = SkScalar(region.corner[1] + region.size[1]);
        canvas.drawRect(rect, paint);

        if (indie::widget_has_focus(*sys_, this))
        {
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(4);
            paint.setColor(SK_ColorBLACK);
            canvas.drawRect(rect, paint);
        }
        // canvas.save();
        // canvas.translate(
        //     SkScalar(region.corner[0]), SkScalar(region.corner[1]));
        // canvas.drawPicture(picture_.get());
        // canvas.restore();
    }

    void
    hit_test(indie::hit_test_base& test, vector<2, double> const& point)
        const override
    {
        if (is_inside(this->assignment().region, vector<2, float>(point)))
        {
            switch (test.type)
            {
                case indie::hit_test_type::MOUSE: {
                    static_cast<indie::mouse_hit_test&>(test).result
                        = indie::mouse_hit_test_result{
                            externalize(this),
                            indie::mouse_cursor::POINTER,
                            this->assignment().region,
                            ""};
                    break;
                }
                case indie::hit_test_type::WHEEL: {
                    static_cast<indie::wheel_hit_test&>(test).result
                        = externalize(this);
                    break;
                }
            }
        }
    }

    void
    process_input(indie::event_context ctx) override
    {
        indie::add_to_focus_order(ctx, this);
        if (detect_click(ctx, this, indie::mouse_button::LEFT))
        {
            state_ = !state_;
            // advance_focus(get_system(ctx));
        }
        // if (detect_key_press(ctx, this, indie::key_code::SPACE))
        // {
        //     state_ = !state_;
        //     // advance_focus(get_system(ctx));
        // }
        if (detect_keyboard_click(ctx, keyboard_click_state_, this))
        {
            state_ = !state_;
        }
    }

    // external_component_id
    // identity() const
    // {
    //     return id_;
    // }

    indie::system* sys_;
    // external_component_id id_;
    bool state_ = false;
    SkColor color_ = SK_ColorWHITE;
    indie::keyboard_click_state keyboard_click_state_;
    // sk_sp<SkPicture> picture_;
};

void
do_box(indie::context ctx, SkColor color)
{
    std::shared_ptr<box_node>* node_ptr;
    if (get_cached_data(ctx, &node_ptr))
    {
        *node_ptr = std::make_shared<box_node>();
        (*node_ptr)->sys_ = &get_system(ctx);
        (*node_ptr)->color_ = color;
    }

    auto& node = **node_ptr;

    // auto id = get_component_id(ctx);

    if (is_refresh_event(ctx))
    {
        add_widget(get<indie::traversal_tag>(ctx).widgets, &node);
        node.refresh_layout(
            get<indie::traversal_tag>(ctx).layout,
            alia::layout(TOP | LEFT | PADDED),
            alia::leaf_layout_requirements(
                alia::make_layout_vector(100, 100), 0, 0));
        add_layout_node(get<indie::traversal_tag>(ctx).layout, &node);

        // node.id_ = externalize(id);

        // if (color != node.color_)
        // {
        //     SkPictureRecorder recorder;
        //     SkRect bounds;
        //     bounds.fLeft = 0;
        //     bounds.fTop = 0;
        //     bounds.fRight = 100;
        //     bounds.fBottom = 100;
        //     SkCanvas* canvas = recorder.beginRecording(bounds);

        //     {
        //         SkPaint paint;
        //         paint.setColor(color);
        //         SkRect rect;
        //         rect.fLeft = 0;
        //         rect.fTop = 0;
        //         rect.fRight = 100;
        //         rect.fBottom = 100;
        //         canvas->drawRect(rect, paint);
        //     }

        //     node.picture_ = recorder.finishRecordingAsPicture();
        // }
    }
}

namespace alia {

ALIA_DECLARE_LAYOUT_LOGIC(column_layout_logic)

namespace indie {

layout_traversal&
get_layout_traversal(context ctx)
{
    return get<indie::traversal_tag>(ctx).layout;
}

template<class LayoutContainer>
struct layout_container_widget : widget_container, LayoutContainer
{
    void
    render(SkCanvas& canvas) override
    {
        canvas.save();
        auto const& offset = get_container_offset(*this);
        canvas.translate(offset[0], offset[1]);
        indie::render_children(canvas, *this);
        canvas.restore();
    }

    void
    hit_test(
        hit_test_base& test, vector<2, double> const& point) const override
    {
        auto local_point
            = point - vector<2, double>(get_container_offset(*this));
        auto region = get_container_region(*this);
        if (is_inside(region, vector<2, float>(local_point)))
        {
            for (widget* node = this->widget_container::children; node;
                 node = node->next)
            {
                node->hit_test(test, local_point);
            }
        }
    }

    void
    process_input(event_context) override
    {
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
    hit_test(
        hit_test_base& test, vector<2, double> const& point) const override
    {
        for (widget* node = this->widget_container::children; node;
             node = node->next)
        {
            node->hit_test(test, point);
        }
    }

    void
    process_input(event_context) override
    {
    }
};

// get_simple_layout_container is a utility function for retrieving a
// simple_layout_container with a specific type of logic from a UI context's
// data graph and refreshing it.
template<class Logic>
struct layout_widget_container_storage
{
    layout_container_widget<simple_layout_container> container;
    Logic logic;
};
template<class Logic>
void
get_layout_widget_container(
    layout_traversal& traversal,
    data_traversal& data,
    layout_container_widget<simple_layout_container>** container,
    Logic** logic,
    layout const& layout_spec)
{
    layout_widget_container_storage<Logic>* storage;
    if (get_cached_data(data, &storage))
        storage->container.logic = &storage->logic;

    *container = &storage->container;

    if (is_refresh_pass(traversal))
    {
        if (update_layout_cacher(
                traversal, (*container)->cacher, layout_spec, FILL | UNPADDED))
        {
            // Since this container isn't active yet, it didn't get marked as
            // needing recalculation, so we need to do that manually here.
            (*container)->last_content_change = traversal.refresh_counter;
        }
    }

    *logic = &storage->logic;
}

struct scoped_column
{
    scoped_column() : container_(nullptr)
    {
    }

    scoped_column(context ctx, layout const& layout_spec = default_layout)
    {
        begin(ctx, layout_spec);
    }

    ~scoped_column()
    {
        end();
    }

    void
    begin(context ctx, layout const& layout_spec = default_layout)
    {
        column_layout_logic* logic;
        get_layout_widget_container(
            get_layout_traversal(ctx),
            get_data_traversal(ctx),
            &container_,
            &logic,
            layout_spec);
        slc_.begin(get_layout_traversal(ctx), container_);
        begin_layout_transform(
            transform_, get_layout_traversal(ctx), container_->cacher);
        widget_scope_.begin(get_widget_traversal(ctx), container_);
    }

    void
    end()
    {
        if (container_)
        {
            widget_scope_.end();
            transform_.end();
            slc_.end();
            container_ = 0;
        }
    }

    layout_box
    region() const
    {
        return get_container_region(*container_);
    }

    layout_box
    padded_region() const
    {
        return get_padded_container_region(*container_);
    }

    layout_vector
    offset() const
    {
        return get_container_offset(*container_);
    }

 private:
    layout_container_widget<simple_layout_container>* container_;
    scoped_layout_container slc_;
    scoped_transformation transform_;
    scoped_widget_container widget_scope_;
};

} // namespace indie
} // namespace alia

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

    flow_layout flow(ctx, GROW);

    for (int i = 0; i != 100; ++i)
    {
        {
            indie::scoped_column col(ctx);

            do_box(ctx, SK_ColorMAGENTA);

            // color::yiq<std::uint8_t> y1 = ::color::constant::blue_t{};
            // color::yiq<std::uint8_t> y2 = ::color::constant::red_t{};
            // color::yiq<std::uint8_t> yr = color::operation::mix(
            //     y1,
            //     std::max(
            //         0.0,
            //         std::min(
            //             1.0,
            //             std::fabs(std::sin(
            //                 get_raw_animation_tick_count(ctx) / 1000.0)))),
            //     y2);
            // color::rgb<std::uint8_t> r(yr);
            // color::rgb<std::uint8_t> r(y1);

            do_box(ctx, SK_ColorLTGRAY);
        }

        {
            indie::scoped_column col(ctx);

            static SkColor clicky_color = SK_ColorRED;
            // event_handler<indie::mouse_button_event>(
            //     ctx, [&](auto, auto&) { clicky_color = SK_ColorBLUE; });
            do_box(ctx, clicky_color);

            do_box(ctx, SK_ColorDKGRAY);

            do_box(ctx, SK_ColorGRAY);
        }
    }
}