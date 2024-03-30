#ifndef ALIA_UI_TEXT_WIDGETS_HPP
#define ALIA_UI_TEXT_WIDGETS_HPP

#include <alia/ui/layout/containers/utilities.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/text/fonts.hpp>
#include <alia/ui/text/shaping.hpp>

namespace alia {

struct text_node : layout_leaf
{
    layout_requirements
    get_horizontal_requirements() override
    {
        if (!shape_)
        {
            shape_ = Shape(
                text_.c_str(),
                text_.size(),
                *font_,
                std::numeric_limits<SkScalar>::max());
        }
        return cache_horizontal_layout_requirements(
            cacher, this->last_content_change, [&] {
                return calculated_layout_requirements{shape_->width, 0, 0};
            });
    }
    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width) override
    {
        if (!shape_)
        {
            shape_ = Shape(
                text_.c_str(),
                text_.size(),
                *font_,
                std::numeric_limits<SkScalar>::max());
        }
        return cache_vertical_layout_requirements(
            cacher, this->last_content_change, assigned_width, [&] {
                return calculated_layout_requirements{
                    layout_scalar(shape_->verticalAdvance),
                    0 /* TODO: ascent */,
                    0 /* TODO: descent */};
            });
    }
    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override
    {
        if (!shape_)
        {
            shape_ = Shape(
                text_.c_str(),
                text_.size(),
                *font_,
                std::numeric_limits<SkScalar>::max());
        }
        update_relative_assignment(
            *this,
            cacher,
            this->last_content_change,
            assignment,
            [&](auto const&) {});
    }

    void
    render(render_event& event) override
    {
        SkCanvas& canvas = *event.canvas;

        auto const& region = cacher.relative_assignment.region;

        SkRect bounds;
        bounds.fLeft = SkScalar(region.corner[0] + event.current_offset[0]);
        bounds.fTop = SkScalar(region.corner[1] + event.current_offset[1]);
        bounds.fRight = bounds.fLeft + SkScalar(region.size[0]);
        bounds.fBottom = bounds.fTop + SkScalar(region.size[1]);
        if (canvas.quickReject(bounds))
            return;

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(color_);

        if (!shape_)
        {
            shape_ = Shape(
                text_.c_str(),
                text_.size(),
                *font_,
                std::numeric_limits<SkScalar>::max());
        }
        canvas.drawTextBlob(
            shape_->blob.get(), bounds.fLeft, bounds.fTop, paint);
    }

    void
    hit_test(hit_test_base&, vector<2, double> const&) const override
    {
    }

    void
    process_input(ui_event_context) override
    {
    }

    matrix<3, 3, double>
    transformation() const override
    {
        return parent->transformation();
    }

    layout_box
    bounding_box() const override
    {
        return this->assignment().region;
    }

    void
    reveal_region(region_reveal_request const& request) override
    {
        parent->reveal_region(request);
    }

    ui_system* sys_;

    SkFont* font_;

    captured_id text_id_;
    std::string text_;

    SkColor color_;

    layout_cacher cacher;

    std::optional<ShapeResult> shape_;

    counter_type last_content_change = 0;
};

struct wrapped_text_node : widget
{
    void
    render(render_event& event) override
    {
        SkCanvas& canvas = *event.canvas;

        auto const& region = cacher.relative_assignment.region;

        SkRect bounds;
        bounds.fLeft = SkScalar(region.corner[0] + event.current_offset[0]);
        bounds.fTop = SkScalar(region.corner[1] + event.current_offset[1]);
        bounds.fRight = bounds.fLeft + SkScalar(region.size[0]);
        bounds.fBottom = bounds.fTop + SkScalar(region.size[1]);
        if (canvas.quickReject(bounds))
            return;

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SK_ColorBLACK);

        if (shape_width_ != region.size[0])
        {
            shape_
                = Shape(text_.c_str(), text_.size(), *font_, region.size[0]);
            shape_width_ = region.size[0];
        }
        canvas.drawTextBlob(
            shape_.blob.get(), bounds.fLeft, bounds.fTop, paint);
    }

    void
    hit_test(hit_test_base&, vector<2, double> const&) const override
    {
    }

    void
    process_input(ui_event_context) override
    {
    }

    matrix<3, 3, double>
    transformation() const override
    {
        return parent->transformation();
    }

    layout_requirements
    get_horizontal_requirements() override
    {
        return cache_horizontal_layout_requirements(
            cacher, this->last_content_change, [&] {
                // TODO: What should the actual minimum width be here?
                return calculated_layout_requirements{12, 0, 0};
            });
    }
    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width) override
    {
        return cache_vertical_layout_requirements(
            cacher, this->last_content_change, assigned_width, [&] {
                if (shape_width_ != assigned_width)
                {
                    // std::cout << "(gvr) " << shape_width_ << " -> "
                    //           << assigned_width << std::endl;
                    shape_ = Shape(
                        text_.c_str(), text_.size(), *font_, assigned_width);
                    shape_width_ = assigned_width;
                }

                return calculated_layout_requirements{
                    layout_scalar(shape_.verticalAdvance),
                    0 /* TODO: ascent */,
                    0 /* TODO: descent */};
            });
    }
    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override
    {
        update_relative_assignment(
            *this,
            cacher,
            this->last_content_change,
            assignment,
            [&](auto const&) {
                // if (shape_width_ != resolved_assignment.region.size[0])
                // {
                //     std::cout << "(sra) " << shape_width_ << " -> "
                //               << resolved_assignment.region.size[0]
                //               << std::endl;
                //     shape_ = Shape(
                //         text_.c_str(),
                //         text_.size(),
                //         *the_font,
                //         resolved_assignment.region.size[0]);
                //     shape_width_ = resolved_assignment.region.size[0];
                // }
            });
    }

    layout_box
    bounding_box() const override
    {
        return cacher.relative_assignment.region;
    }

    void
    reveal_region(region_reveal_request const& request) override
    {
        parent->reveal_region(request);
    }

    ui_system* sys_;

    captured_id text_id_;
    std::string text_;

    layout_cacher cacher;

    double shape_width_ = 0;
    ShapeResult shape_;

    SkFont* font_;

    counter_type last_content_change = 0;
};

} // namespace alia

#endif
