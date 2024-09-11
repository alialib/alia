#include <alia/core/flow/data_graph.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/core/timing/cubic_bezier.hpp>
#include <alia/ui.hpp>
#include <alia/ui/color.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/layout/grids.hpp>
#include <alia/ui/layout/simple.hpp>
#include <alia/ui/layout/spacer.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/library/checkbox.hpp>
#include <alia/ui/library/panels.hpp>
#include <alia/ui/library/radio_button.hpp>
#include <alia/ui/library/slider.hpp>
#include <alia/ui/library/switch.hpp>
#include <alia/ui/scrolling.hpp>
#include <alia/ui/system/api.hpp>
#include <alia/ui/system/input_constants.hpp>
#include <alia/ui/system/object.hpp>
#include <alia/ui/text/fonts.hpp>
#include <alia/ui/utilities/keyboard.hpp>
#include <alia/ui/utilities/mouse.hpp>
#include <alia/ui/utilities/regions.hpp>
#include <alia/ui/utilities/rendering.hpp>
#include <alia/ui/utilities/skia.hpp>

// #include <color/color.hpp>

#include <bit>

#include <cmath>

#include <include/core/SkTypeface.h>
#include <limits>

#include <optional>

#include <alia/core/flow/components.hpp>
#include <alia/core/flow/macros.hpp>
#include <alia/core/flow/top_level.hpp>
#include <alia/core/signals/core.hpp>
#include <alia/core/timing/smoothing.hpp>
#include <alia/core/timing/ticks.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/internals.hpp>
#include <alia/ui/text/components.hpp>
#include <alia/ui/utilities/animation.hpp>
#include <alia/ui/utilities/click_flares.hpp>

#ifdef _WIN32
#pragma warning(push, 0)
#endif

#include <include/core/SkBlurTypes.h>
#include <include/core/SkColor.h>
#include <include/core/SkFontTypes.h>
#include <include/core/SkMaskFilter.h>
#include <include/core/SkPath.h>
#include <include/core/SkPictureRecorder.h>

#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/TextStyle.h"
#include "modules/skparagraph/src/ParagraphBuilderImpl.h"
#include "modules/skparagraph/src/ParagraphImpl.h"

#include "include/core/SkRRect.h"
#include "include/core/SkStream.h"
#include "include/utils/SkNoDrawCanvas.h"
// #include "modules/svg/include/SkSVGDOM.h"
// #include "modules/svg/include/SkSVGNode.h"

#ifdef _WIN32
#pragma warning(pop)
#endif

using namespace alia;

struct box_data
{
    bool state_ = false;
    keyboard_click_state keyboard_click_state_;
    layout_leaf layout_node;
    sk_sp<SkPicture> picture;
    rgb8 cached_color;
    layout_box cached_rect;
};

void
do_box(
    ui_context ctx,
    SkColor color,
    action<> /*on_click*/,
    layout const& layout_spec = layout(TOP | LEFT | PADDED))
{
    box_data* data_ptr;
    get_cached_data(ctx, &data_ptr);
    auto& data = *data_ptr;
    auto id = data_ptr;

    alia_untracked_switch(get_event_category(ctx))
    {
        case REFRESH_CATEGORY:
            data.layout_node.refresh_layout(
                get_layout_traversal(ctx),
                layout_spec,
                leaf_layout_requirements(make_layout_vector(40, 40), 0, 0),
                LEFT | BASELINE_Y | PADDED);

            add_layout_node(
                get<ui_traversal_tag>(ctx).layout, &data.layout_node);

            break;

        case REGION_CATEGORY:
            do_box_region(
                ctx, id, box<2, double>(data.layout_node.assignment().region));
            break;

        case INPUT_CATEGORY:
            if (detect_click(ctx, id, mouse_button::LEFT)
                || detect_keyboard_click(ctx, data.keyboard_click_state_, id))
            {
                data.state_ = !data.state_;
            }

            break;

        case RENDER_CATEGORY: {
            auto& event = cast_event<render_event>(ctx);

            SkCanvas& canvas = *event.canvas;

            auto const& region = data.layout_node.assignment().region;

            SkRect rect;
            rect.fLeft = SkScalar(region.corner[0]);
            rect.fTop = SkScalar(region.corner[1]);
            rect.fRight = SkScalar(region.corner[0] + region.size[0]);
            rect.fBottom = SkScalar(region.corner[1] + region.size[1]);

            // if (event.canvas->quickReject(rect))
            //     break;

            double blend_factor = 0;

            if (is_click_in_progress(get_system(ctx), id, mouse_button::LEFT)
                || is_pressed(data.keyboard_click_state_))
            {
                blend_factor = 0.4;
            }
            else if (is_click_possible(get_system(ctx), id))
            {
                blend_factor = 0.2;
            }

            rgb8 c;
            if (data.state_)
            {
                c = rgb8(0x40, 0x40, 0x40);
            }
            else
            {
                c = rgb8(
                    std::uint8_t(SkColorGetR(color)),
                    std::uint8_t(SkColorGetG(color)),
                    std::uint8_t(SkColorGetB(color)));
            }
            if (blend_factor != 0)
            {
                c = interpolate(c, rgb8(0xff, 0xff, 0xff), blend_factor);
            }

            {
                SkPaint paint;
                paint.setColor(SkColorSetARGB(0xff, c.r, c.g, c.b));
                canvas.drawRect(rect, paint);
            }

            // if (!data.picture || data.cached_color != c
            //     || data.cached_rect != region)
            // {
            //     SkPictureRecorder recorder;
            //     SkCanvas* recoreder_canvas = recorder.beginRecording(rect);

            //     {
            //         SkPaint paint;
            //         paint.setColor(SkColorSetARGB(0xff, c.r, c.g, c.b));
            //         recoreder_canvas->drawRect(rect, paint);
            //     }

            //     data.picture = recorder.finishRecordingAsPicture();
            //     data.cached_color = c;
            //     data.cached_rect = region;
            // }

            // data.picture->playback(&canvas);

            // --

            // auto position = smooth_value(
            //     position_,
            //     region.corner + event.current_offset,
            //     tick_counter_,
            //     {default_curve, 80});

            // if (rect.width() > 200)
            // {
            //     SkPaint blur(paint);
            //     blur.setAlpha(200);
            //     blur.setMaskFilter(
            //         SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 40, false));
            //     canvas.drawRect(rect, blur);
            // }

            if (widget_has_focus(ctx, id))
            {
                SkPaint paint;
                paint.setStyle(SkPaint::kStroke_Style);
                paint.setStrokeWidth(4);
                paint.setColor(SK_ColorBLACK);
                canvas.drawRect(rect, paint);
            }
            break;
        }
    }
    alia_end
}

template<class Selected, class Index>
struct radio_signal
    : lazy_signal<radio_signal<Selected, Index>, bool, duplex_signal>
{
    radio_signal(Selected selected, Index index)
        : selected_(std::move(selected)), index_(std::move(index))
    {
    }
    bool
    has_value() const override
    {
        return signal_has_value(selected_) && signal_has_value(index_);
    }
    bool
    move_out() const override
    {
        return read_signal(selected_) == read_signal(index_);
    }
    id_interface const&
    value_id() const override
    {
        return selected_.value_id();
    }
    bool
    ready_to_write() const override
    {
        return signal_ready_to_write(selected_) && signal_has_value(index_);
    }
    id_interface const&
    write(bool) const override
    {
        write_signal(selected_, read_signal(index_));
        return null_id;
    }

 private:
    Selected selected_;
    Index index_;
};
template<class Selected, class Index>
radio_signal<Selected, Index>
make_radio_signal(Selected selected, Index index)
{
    return radio_signal<Selected, Index>(
        std::move(selected), std::move(index));
}

// struct tree_expander_node : widget
// {
//     layout_requirements
//     get_horizontal_requirements() override
//     {
//         layout_requirements requirements;
//         resolve_requirements(
//             requirements,
//             resolved_spec_,
//             0,
//             calculated_layout_requirements{40, 0, 0});
//         return requirements;
//     }
//     layout_requirements
//     get_vertical_requirements(layout_scalar /*assigned_width*/) override
//     {
//         layout_requirements requirements;
//         resolve_requirements(
//             requirements,
//             resolved_spec_,
//             1,
//             calculated_layout_requirements{40, 0, 0});
//         return requirements;
//     }
//     void
//     set_relative_assignment(
//         relative_layout_assignment const& assignment) override
//     {
//         layout_requirements horizontal_requirements, vertical_requirements;
//         resolve_requirements(
//             horizontal_requirements,
//             resolved_spec_,
//             0,
//             calculated_layout_requirements{40, 0, 0});
//         resolve_requirements(
//             vertical_requirements,
//             resolved_spec_,
//             1,
//             calculated_layout_requirements{40, 0, 0});
//         relative_assignment_ = resolve_relative_assignment(
//             resolved_spec_,
//             assignment,
//             horizontal_requirements,
//             vertical_requirements);
//     }

//     void
//     render(render_event& event) override
//     {
//         SkCanvas& canvas = *event.canvas;

//         auto const& region = this->assignment().region;

//         uint8_t background_alpha = 0;

//         if (is_click_in_progress(
//                 *sys_, widget_id{*this, 0}, mouse_button::LEFT)
//             || is_pressed(keyboard_click_state_))
//         {
//             background_alpha = 0x30;
//         }
//         else if (is_click_possible(*sys_, widget_id{*this, 0}))
//         {
//             background_alpha = 0x18;
//         }

//         auto position = region.corner + event.current_offset;

//         if (background_alpha != 0)
//         {
//             SkPaint paint;
//             paint.setColor(SkColorSetARGB(background_alpha, 0x00, 0x00,
//             0xff)); SkRect rect; rect.fLeft = SkScalar(position[0]);
//             rect.fTop = SkScalar(position[1]);
//             rect.fRight = SkScalar(position[0] + region.size[0]);
//             rect.fBottom = SkScalar(position[1] + region.size[1]);
//             canvas.drawRect(rect, paint);
//         }

//         float angle = smooth_value(
//             angle_smoother_,
//             state_ ? 90.f : 0.f,
//             tick_counter_,
//             animated_transition{linear_curve, 200});

//         canvas.save();

//         canvas.translate(
//             position[0] + region.size[0] / SkIntToScalar(2),
//             position[1] + region.size[1] / SkIntToScalar(2));
//         canvas.rotate(angle);

//         {
//             SkPaint paint;
//             paint.setAntiAlias(true);
//             paint.setColor(SK_ColorBLACK);
//             // set_color(paint, renderer.style().fg_color);
//             paint.setStyle(SkPaint::kFill_Style);
//             SkScalar a = region.size[0] / SkDoubleToScalar(2.5);
//             SkPath path;
//             path.incReserve(4);
//             SkPoint p0;
//             p0.fX = a * SkDoubleToScalar(-0.34);
//             p0.fY = a * SkDoubleToScalar(-0.5);
//             path.moveTo(p0);
//             SkPoint p1;
//             p1.fX = p0.fX;
//             p1.fY = a * SkDoubleToScalar(0.5);
//             path.lineTo(p1);
//             SkPoint p2;
//             p2.fX = p0.fX + a * SkDoubleToScalar(0.866);
//             p2.fY = 0;
//             path.lineTo(p2);
//             path.lineTo(p0);
//             canvas.drawPath(path, paint);
//         }

//         canvas.restore();

//         // if (rect.width() > 200)
//         // {
//         //     SkPaint blur(paint);
//         //     blur.setAlpha(200);
//         //     blur.setMaskFilter(
//         //         SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 40, false));
//         //     canvas.drawRect(rect, blur);
//         // }

//         // if (widget_has_focus(*sys_, widget_id{*this, 0}))
//         // {
//         //     SkPaint paint;
//         //     paint.setStyle(SkPaint::kStroke_Style);
//         //     paint.setStrokeWidth(4);
//         //     paint.setColor(SK_ColorBLACK);
//         //     canvas.drawRect(rect, paint);
//         // }
//     }

//     void
//     hit_test(
//         hit_test_base& test, vector<2, double> const& point) const override
//     {
//         std::cout << "hit test: " << point << std::endl;
//         if (is_inside(this->assignment().region, vector<2, float>(point)))
//         {
//             std::cout << "inside!" << std::endl;
//             if (test.type == hit_test_type::MOUSE)
//             {
//                 static_cast<mouse_hit_test&>(test).result
//                     = mouse_hit_test_result{
//                         externalize(widget_id{*this, 0}),
//                         mouse_cursor::POINTER,
//                         this->assignment().region,
//                         ""};
//             }
//         }
//     }

//     void
//     process_input(dataless_ui_context ctx) override
//     {
//         add_to_focus_order(ctx, widget_id{*this, 0});
//         if (detect_click(
//                 ctx, widget_id{*this, 0}, mouse_button::LEFT))
//         {
//             state_ = !state_;
//         }
//         if (detect_keyboard_click(
//                 ctx, keyboard_click_state_, widget_id{*this, 0}))
//         {
//             state_ = !state_;
//         }
//     }

//     matrix<3, 3, double>
//     transformation() const override
//     {
//         return parent->transformation();
//     }

//     relative_layout_assignment const&
//     assignment() const
//     {
//         return relative_assignment_;
//     }

//     layout_box
//     bounding_box() const override
//     {
//         return add_border(this->assignment().region, 4.f);
//     }

//     void
//     reveal_region(region_reveal_request const& request) override
//     {
//         parent->reveal_region(request);
//     }

//     ui_system* sys_;
//     external_component_id id_;
//     bool state_ = false;
//     keyboard_click_state keyboard_click_state_;
//     value_smoother<float> angle_smoother_;
//     millisecond_count tick_counter_;
//     // the resolved spec
//     resolved_layout_spec resolved_spec_;
//     // resolved relative assignment
//     relative_layout_assignment relative_assignment_;
// };

// void
// do_tree_expander(
//     ui_context ctx, layout const& layout_spec = layout(TOP | LEFT | PADDED))
// {
//     std::shared_ptr<tree_expander_node>* node_ptr;
//     if (get_cached_data(ctx, &node_ptr))
//     {
//         *node_ptr = std::make_shared<tree_expander_node>();
//         (*node_ptr)->sys_ = &get_system(ctx);
//     }
//     auto& node = **node_ptr;

//     auto id = get_component_id(ctx);

//     if (is_refresh_event(ctx))
//     {
//         resolved_layout_spec resolved_spec;
//         resolve_layout_spec(
//             get<ui_traversal_tag>(ctx).layout,
//             resolved_spec,
//             layout_spec,
//             TOP | LEFT | PADDED);
//         detect_layout_change(
//             get<ui_traversal_tag>(ctx).layout,
//             &node.resolved_spec_,
//             resolved_spec);

//         add_layout_node(get<ui_traversal_tag>(ctx).layout, &node);

//         node.id_ = externalize(id);

//         node.tick_counter_ = get_raw_animation_tick_count(ctx);
//     }
// }

// // struct svg_image_node : widget
// // {
// //     layout_requirements
// //     get_horizontal_requirements() override
// //     {
// //         layout_requirements requirements;
// //         resolve_requirements(
// //             requirements,
// //             resolved_spec_,
// //             0,
// //             calculated_layout_requirements{40, 0, 0});
// //         return requirements;
// //     }
// //     layout_requirements
// //     get_vertical_requirements(layout_scalar /*assigned_width*/) override
// //     {
// //         layout_requirements requirements;
// //         resolve_requirements(
// //             requirements,
// //             resolved_spec_,
// //             1,
// //             calculated_layout_requirements{40, 0, 0});
// //         return requirements;
// //     }
// //     void
// //     set_relative_assignment(
// //         relative_layout_assignment const& assignment) override
// //     {
// //         layout_requirements horizontal_requirements,
// vertical_requirements;
// //         resolve_requirements(
// //             horizontal_requirements,
// //             resolved_spec_,
// //             0,
// //             calculated_layout_requirements{40, 0, 0});
// //         resolve_requirements(
// //             vertical_requirements,
// //             resolved_spec_,
// //             1,
// //             calculated_layout_requirements{40, 0, 0});
// //         relative_assignment_ = resolve_relative_assignment(
// //             resolved_spec_,
// //             assignment,
// //             horizontal_requirements,
// //             vertical_requirements);
// //     }

// //     void
// //     render(render_event& event) override
// //     {
// //         SkCanvas& canvas = *event.canvas;

// //         auto const& region = this->assignment().region;

// //         uint8_t background_alpha = 0;

// //         if (is_click_in_progress(
// //                 *sys_, widget_id{*this, 0}, mouse_button::LEFT)
// //             || is_pressed(keyboard_click_state_))
// //         {
// //             background_alpha = 0x30;
// //         }
// //         else if (is_click_possible(*sys_, widget_id{*this, 0}))
// //         {
// //             background_alpha = 0x18;
// //         }

// //         auto position = region.corner + event.current_offset;

// //         if (background_alpha != 0)
// //         {
// //             SkPaint paint;
// //             paint.setColor(SkColorSetARGB(background_alpha, 0x00, 0x00,
// //             0xff)); SkRect rect; rect.fLeft = SkScalar(position[0]);
// //             rect.fTop = SkScalar(position[1]);
// //             rect.fRight = SkScalar(position[0] + region.size[0]);
// //             rect.fBottom = SkScalar(position[1] + region.size[1]);
// //             canvas.drawRect(rect, paint);
// //         }

// //         canvas.save();

// //         const std::string svgText = R"EOF(
// // <svg xmlns="http://www.w3.org/2000/svg"
// //  width="467" height="462">
// //   <rect x="80" y="60" width="250" height="250" rx="20"
// //       style="fill:#ff0000; stroke:#000000;stroke-width:2px;" />

// //   <rect x="140" y="120" width="250" height="250" rx="40"
// //       style="fill:#0000ff; stroke:#000000; stroke-width:2px;
// //       fill-opacity:0.7;" />
// // </svg>
// // )EOF";

// //         auto str = SkMemoryStream::MakeDirect(svgText.c_str(),
// //         svgText.size()); auto svg_dom = SkSVGDOM::Builder().make(*str);
// //         svg_dom->render(&canvas);

// //         canvas.restore();

// //         // if (rect.width() > 200)
// //         // {
// //         //     SkPaint blur(paint);
// //         //     blur.setAlpha(200);
// //         //     blur.setMaskFilter(
// //         //         SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 40,
// false));
// //         //     canvas.drawRect(rect, blur);
// //         // }

// //         // if (widget_has_focus(*sys_, widget_id{*this, 0}))
// //         // {
// //         //     SkPaint paint;
// //         //     paint.setStyle(SkPaint::kStroke_Style);
// //         //     paint.setStrokeWidth(4);
// //         //     paint.setColor(SK_ColorBLACK);
// //         //     canvas.drawRect(rect, paint);
// //         // }
// //     }

// //     void
// //     hit_test(
// //         hit_test_base& test, vector<2, double> const& point) const
// override
// //     {
// //         if (is_inside(this->assignment().region, vector<2,
// float>(point)))
// //         {
// //             if (test.type == hit_test_type::MOUSE)
// //             {
// //                 static_cast<mouse_hit_test&>(test).result
// //                     = mouse_hit_test_result{
// //                         externalize(widget_id{*this, 0}),
// //                         mouse_cursor::POINTER,
// //                         this->assignment().region,
// //                         ""};
// //             }
// //         }
// //     }

// //     void
// //     process_input(dataless_ui_context ctx) override
// //     {
// //         add_to_focus_order(ctx, widget_id{*this, 0});
// //         if (detect_click(
// //                 ctx, widget_id{*this, 0}, mouse_button::LEFT))
// //         {
// //             state_ = !state_;
// //         }
// //         if (detect_keyboard_click(
// //                 ctx, keyboard_click_state_, widget_id{*this,
// 0}))
// //         {
// //             state_ = !state_;
// //         }
// //     }

// //     matrix<3, 3, double>
// //     transformation() const override
// //     {
// //         return parent->transformation();
// //     }

// //     relative_layout_assignment const&
// //     assignment() const
// //     {
// //         return relative_assignment_;
// //     }

// //     layout_box
// //     bounding_box() const override
// //     {
// //         return add_border(this->assignment().region, 4.f);
// //     }

// //     void
// //     reveal_region(region_reveal_request const& request) override
// //     {
// //         parent->reveal_region(request);
// //     }

// //     ui_system* sys_;
// //     external_component_id id_;
// //     bool state_ = false;
// //     keyboard_click_state keyboard_click_state_;
// //     value_smoother<float> angle_smoother_;
// //     millisecond_count tick_counter_;
// //     // the resolved spec
// //     resolved_layout_spec resolved_spec_;
// //     // resolved relative assignment
// //     relative_layout_assignment relative_assignment_;
// // };

// // void
// // do_svg_image(
// //     ui_context ctx, layout const& layout_spec = layout(TOP | LEFT |
// PADDED))
// // {
// //     std::shared_ptr<svg_image_node>* node_ptr;
// //     if (get_cached_data(ctx, &node_ptr))
// //     {
// //         *node_ptr = std::make_shared<svg_image_node>();
// //         (*node_ptr)->sys_ = &get_system(ctx);
// //     }
// //     auto& node = **node_ptr;

// //     auto id = get_component_id(ctx);

// //     if (is_refresh_event(ctx))
// //     {
// //         resolved_layout_spec resolved_spec;
// //         resolve_layout_spec(
// //             get<ui_traversal_tag>(ctx).layout,
// //             resolved_spec,
// //             layout_spec,
// //             TOP | LEFT | PADDED);
// //         detect_layout_change(
// //             get<ui_traversal_tag>(ctx).layout,
// //             &node.resolved_spec_,
// //             resolved_spec);

// //         add_layout_node(get<ui_traversal_tag>(ctx).layout, &node);

// //         node.id_ = externalize(id);

// //         node.tick_counter_ = get_raw_animation_tick_count(ctx);
// //     }
// // }

// // ---

// // ---

// // namespace alia {

// // template<class Value>
// // struct readable_signal_type
// // {
// //     using type = readable<Value>;
// // };

// // template<class Value>
// // struct in : readable_signal_type<Value>::type
// // {
// //     using signal_ref::signal_ref;
// // };

// // } // namespace alia

// // #define ALIA_PP_CONCAT(a, b) ALIA_PP_CONCAT1(a, b)
// // #define ALIA_PP_CONCAT1(a, b) ALIA_PP_CONCAT2(a, b)
// // #define ALIA_PP_CONCAT2(a, b) a##b

// // #define ALIA_PP_FE_2_0(F, a, b)
// // #define ALIA_PP_FE_2_1(F, a, b, x) F(a, b, x)
// // #define ALIA_PP_FE_2_2(F, a, b, x, ...) \
// //     F(a, b, x) ALIA_PP_FE_2_1(F, a, b, __VA_ARGS__)
// // #define ALIA_PP_FE_2_3(F, a, b, x, ...) \
// //     F(a, b, x) ALIA_PP_FE_2_2(F, a, b, __VA_ARGS__)
// // #define ALIA_PP_FE_2_4(F, a, b, x, ...) \
// //     F(a, b, x) ALIA_PP_FE_2_3(F, a, b, __VA_ARGS__)
// // #define ALIA_PP_FE_2_5(F, a, b, x, ...) \
// //     F(a, b, x) ALIA_PP_FE_2_4(F, a, b, __VA_ARGS__)
// // #define ALIA_PP_FE_2_6(F, a, b, x, ...) \
// //     F(a, b, x) ALIA_PP_FE_2_5(F, a, b, __VA_ARGS__)

// // #define ALIA_PP_GET_MACRO(_0, _1, _2, _3, _4, _5, _6, NAME, ...) NAME
// // #define ALIA_PP_FOR_EACH_2(F, a, b, ...) \
// //     ALIA_PP_GET_MACRO( \
// //         _0, \
// //         __VA_ARGS__, \
// //         ALIA_PP_FE_2_6, \
// //         ALIA_PP_FE_2_5, \
// //         ALIA_PP_FE_2_4, \
// //         ALIA_PP_FE_2_3, \
// //         ALIA_PP_FE_2_2, \
// //         ALIA_PP_FE_2_1, \
// //         ALIA_PP_FE_2_0) \
// //     (F, a, b, __VA_ARGS__)

// // #define ALIA_DEFINE_STRUCT_SIGNAL_FIELD(signal_type, struct_name, field_name) \
// //     auto ALIA_PP_CONCAT(ALIA_PP_CONCAT(_get_, field_name), _signal)()         \
// //     {                                                                         \
// //         return (*this)->*&struct_name::field_name;                            \
// //     }                                                                         \
// //     __declspec(property(                                                      \
// //         get = ALIA_PP_CONCAT(ALIA_PP_CONCAT(_get_, field_name), _signal)))    \
// //         alia::field_signal<                                                   \
// //             ALIA_PP_CONCAT(ALIA_PP_CONCAT(signal_type, _), struct_name),      \
// //             decltype(struct_name::field_name)>                                \
// //             field_name;

// // #define ALIA_DEFINE_STRUCT_SIGNAL_FIELDS(signal_type, struct_name, ...)       \
// //     ALIA_PP_FOR_EACH_2(                                                       \
// //         ALIA_DEFINE_STRUCT_SIGNAL_FIELD,                                      \
// //         signal_type,                                                          \
// //         struct_name,                                                          \
// //         __VA_ARGS__)

// // #define ALIA_DEFINE_CUSTOM_STRUCT_SIGNAL(                                     \
// //     signal_name, signal_type, struct_name, ...)                               \
// //     struct ALIA_PP_CONCAT(ALIA_PP_CONCAT(signal_type, _), struct_name)        \
// //         : signal_type<struct_name>                                            \
// //     {                                                                         \
// //         using signal_ref::signal_ref;                                         \
// //         ALIA_DEFINE_STRUCT_SIGNAL_FIELDS(                                     \
// //             signal_type, struct_name, __VA_ARGS__)                            \
// //     };

// // #define ALIA_DEFINE_STRUCT_SIGNAL(signal_type, struct_name, ...)              \
// //     ALIA_DEFINE_CUSTOM_STRUCT_SIGNAL(                                         \
// //         ALIA_PP_CONCAT(ALIA_PP_CONCAT(signal_type, _), struct_name),          \
// //         signal_type,                                                          \
// //         struct_name,                                                          \
// //         __VA_ARGS__)

// // #define ALIA_DEFINE_STRUCT_SIGNALS(struct_name, ...)                          \
// //     ALIA_DEFINE_STRUCT_SIGNAL(readable, struct_name, __VA_ARGS__)             \
// //     ALIA_DEFINE_STRUCT_SIGNAL(duplex, struct_name, __VA_ARGS__)

// // struct realm
// // {
// //     std::string name;
// //     std::string description;
// // };
// // ALIA_DEFINE_STRUCT_SIGNALS(realm, name, description)

// // template<>
// // struct alia::readable_signal_type<realm>
// // {
// //     using type = readable_realm;
// // };

// // in<realm>
// // get_realm_name(ui_context ctx)
// // {
// //     auto foo = value("focus");
// //     return alia::apply(
// //         ctx,
// //         [] {
// //             return realm{foo, "a realm for " + foo};
// //         },
// //         foo);
// // }

// namespace alia {

// struct scoped_flow_layout : simple_scoped_layout<flow_layout_logic>
// {
//     using simple_scoped_layout::simple_scoped_layout;

//     void
//     begin(ui_context ctx, layout const& layout_spec = default_layout)
//     {
//         // With a flow layout, we want to have the layout itself
//         // always fill the horizontal space and use the requested X
//         // alignment to position the individual rows in the flow.
//         auto adjusted_layout_spec = add_default_padding(layout_spec,
//         PADDED); layout_flag_set x_alignment = FILL_X; if
//         ((layout_spec.flags.code & 0x3) != 0)
//         {
//             x_alignment.code
//                 = adjusted_layout_spec.flags.code & X_ALIGNMENT_MASK_CODE;
//             adjusted_layout_spec.flags.code &= ~X_ALIGNMENT_MASK_CODE;
//             adjusted_layout_spec.flags.code |= FILL_X_CODE;
//         }
//         flow_layout_logic* logic;
//         get_simple_layout_container(
//             get_layout_traversal(ctx),
//             get_data_traversal(ctx),
//             &container_,
//             &logic,
//             adjusted_layout_spec);
//         logic->x_alignment = x_alignment;
//         slc_.begin(get_layout_traversal(ctx), container_);
//     }
// };

// struct nonuniform_grid_tag
// {
// };
// struct uniform_grid_tag
// {
// };

// // This structure stores the layout requirements for the columns in a grid.
// template<class Uniformity>
// struct grid_column_requirements
// {
// };
// // In nonuniform grids, each column has its own requirements.
// template<>
// struct grid_column_requirements<nonuniform_grid_tag>
// {
//     std::vector<layout_requirements> columns;
// };
// // In uniform grids, the columns all share the same requirements.
// template<>
// struct grid_column_requirements<uniform_grid_tag>
// {
//     size_t n_columns;
//     layout_requirements requirements;
// };

// // Get the number of columns.
// static size_t
// get_column_count(grid_column_requirements<nonuniform_grid_tag> const& x)
// {
//     return x.columns.size();
// }
// static size_t
// get_column_count(grid_column_requirements<uniform_grid_tag> const& x)
// {
//     return x.n_columns;
// }

// // Reset the column requirements.
// static void
// clear_requirements(grid_column_requirements<nonuniform_grid_tag>& x)
// {
//     x.columns.clear();
// }
// static void
// clear_requirements(grid_column_requirements<uniform_grid_tag>& x)
// {
//     x.n_columns = 0;
//     x.requirements = layout_requirements{0, 0, 0, 1};
// }

// // Add the requirements for a column.
// static void
// add_requirements(
//     grid_column_requirements<nonuniform_grid_tag>& x,
//     layout_requirements const& addition)
// {
//     x.columns.push_back(addition);
// }
// static void
// add_requirements(
//     grid_column_requirements<uniform_grid_tag>& x,
//     layout_requirements const& addition)
// {
//     ++x.n_columns;
//     fold_in_requirements(x.requirements, addition);
// }

// // Fold the second set of requirements into the first.
// static void
// fold_in_requirements(
//     grid_column_requirements<nonuniform_grid_tag>& x,
//     grid_column_requirements<nonuniform_grid_tag> const& y)
// {
//     size_t n_columns = get_column_count(y);
//     if (get_column_count(x) < n_columns)
//         x.columns.resize(n_columns, layout_requirements{0, 0, 0, 0});
//     for (size_t i = 0; i != n_columns; ++i)
//     {
//         layout_requirements& xi = x.columns[i];
//         layout_requirements const& yi = y.columns[i];
//         fold_in_requirements(xi, yi);
//         if (xi.growth_factor < yi.growth_factor)
//             xi.growth_factor = yi.growth_factor;
//     }
// }
// static void
// fold_in_requirements(
//     grid_column_requirements<uniform_grid_tag>& x,
//     grid_column_requirements<uniform_grid_tag> const& y)
// {
//     if (x.n_columns < y.n_columns)
//         x.n_columns = y.n_columns;
//     fold_in_requirements(x.requirements, y.requirements);
// }

// // Get the requirements for the nth column.
// inline layout_requirements const&
// get_column_requirements(
//     grid_column_requirements<nonuniform_grid_tag> const& x, size_t n)
// {
//     return x.columns[n];
// }
// inline layout_requirements const&
// get_column_requirements(
//     grid_column_requirements<uniform_grid_tag> const& x, size_t /*n*/)
// {
//     return x.requirements;
// }

// template<class Uniformity>
// struct grid_row_container;

// template<class Uniformity>
// struct cached_grid_vertical_requirements
// {
// };

// template<>
// struct cached_grid_vertical_requirements<uniform_grid_tag>
// {
//     calculated_layout_requirements requirements;
//     counter_type last_update = 0;
// };

// template<class Uniformity>
// struct grid_data
// {
//     // the container that contains the whole grid
//     widget_container* container = nullptr;

//     // list of rows in the grid
//     grid_row_container<Uniformity>* rows = nullptr;

//     // spacing between columns
//     layout_scalar column_spacing;

//     // requirements for the columns
//     grid_column_requirements<Uniformity> requirements;
//     counter_type last_content_query = 0;

//     // cached vertical requirements
//     cached_grid_vertical_requirements<Uniformity>
//     vertical_requirements_cache;

//     // cached assignments
//     std::vector<layout_scalar> assignments;
//     counter_type last_assignments_update;
// };

// template<class Uniformity>
// void
// refresh_grid(
//     layout_traversal<widget_container, widget>& traversal,
//     grid_data<Uniformity>& data)
// {
//     if (traversal.is_refresh_pass)
//     {
//         // Reset the row list.
//         data.rows = 0;
//     }
// }

// template<class Uniformity>
// void
// refresh_grid_row(
//     layout_traversal<widget_container, widget>& traversal,
//     grid_data<Uniformity>& grid,
//     grid_row_container<Uniformity>& row,
//     layout const& layout_spec)
// {
//     // Add this row to the grid's list of rows.
//     // It doesn't matter what order the list is in, and adding the row to
//     // the front of the list is easier.
//     if (traversal.is_refresh_pass)
//     {
//         row.next = grid.rows;
//         grid.rows = &row;
//         row.grid = &grid;
//     }

//     update_layout_cacher(traversal, row.cacher, layout_spec, FILL |
//     UNPADDED);
// }

// struct scoped_grid_layout
// {
//     scoped_grid_layout()
//     {
//     }

//     template<class Context>
//     scoped_grid_layout(
//         Context& ctx,
//         layout const& layout_spec = default_layout,
//         absolute_length const& column_spacing = absolute_length(0, PIXELS))
//     {
//         begin(ctx, layout_spec, column_spacing);
//     }

//     ~scoped_grid_layout()
//     {
//         end();
//     }

//     template<class Context>
//     void
//     begin(
//         Context& ctx,
//         layout const& layout_spec = default_layout,
//         absolute_length const& column_spacing = absolute_length(0, PIXELS))
//     {
//         concrete_begin(
//             get_layout_traversal(ctx),
//             get_data_traversal(ctx),
//             layout_spec,
//             column_spacing);
//     }

//     void
//     end()
//     {
//         container_.end();
//     }

//  private:
//     void
//     concrete_begin(
//         layout_traversal<widget_container, widget>& traversal,
//         data_traversal& data,
//         layout const& layout_spec,
//         absolute_length const& column_spacing);

//     friend struct scoped_grid_row;

//     scoped_layout_container container_;
//     layout_traversal<widget_container, widget>* traversal_;
//     data_traversal* data_traversal_;
//     grid_data<nonuniform_grid_tag>* data_;
// };

// void
// scoped_grid_layout::concrete_begin(
//     layout_traversal<widget_container, widget>& traversal,
//     data_traversal& data,
//     layout const& layout_spec,
//     absolute_length const& column_spacing)
// {
//     traversal_ = &traversal;
//     data_traversal_ = &data;

//     get_cached_data(data, &data_);
//     refresh_grid(traversal, *data_);

//     layout_container_widget<simple_layout_container<column_layout_logic>>*
//         container;
//     column_layout_logic* logic;
//     get_simple_layout_container(
//         traversal, data, &container, &logic, layout_spec);
//     container_.begin(traversal, container);

//     data_->container = container;

//     layout_scalar resolved_spacing = as_layout_size(
//         resolve_absolute_length(traversal, 0, column_spacing));
//     detect_layout_change(traversal, &data_->column_spacing,
//     resolved_spacing);
// }

// struct scoped_grid_row
// {
//     scoped_grid_row()
//     {
//     }
//     scoped_grid_row(
//         ui_context ctx,
//         scoped_grid_layout const& g,
//         layout const& layout_spec = default_layout)
//     {
//         begin(ctx, g, layout_spec);
//     }
//     ~scoped_grid_row()
//     {
//         end();
//     }
//     void
//     begin(
//         ui_context ctx,
//         scoped_grid_layout const& g,
//         layout const& layout_spec = default_layout);
//     void
//     end();

//  private:
//     scoped_layout_container container_;
// };

// template<class Uniformity>
// struct grid_row_container : widget_container
// {
//     void
//     render(render_event& event) override
//     {
//         auto const& region = get_assignment(this->cacher).region;
//         SkRect bounds;
//         bounds.fLeft = SkScalar(region.corner[0] + event.current_offset[0]);
//         bounds.fTop = SkScalar(region.corner[1] + event.current_offset[1]);
//         bounds.fRight = bounds.fLeft + SkScalar(region.size[0]);
//         bounds.fBottom = bounds.fTop + SkScalar(region.size[1]);
//         if (!event.canvas->quickReject(bounds))
//         {
//             auto original_offset = event.current_offset;
//             event.current_offset += region.corner;
//             render_children(event, *this);
//             event.current_offset = original_offset;
//         }
//     }

//     void
//     hit_test(
//         hit_test_base& test, vector<2, double> const& point) const override
//     {
//         auto const& region = get_assignment(this->cacher).region;
//         if (is_inside(region, vector<2, float>(point)))
//         {
//             auto local_point = point - vector<2, double>(region.corner);
//             for (widget* node = this->widget_container::children; node;
//                  node = node->next)
//             {
//                 node->hit_test(test, local_point);
//             }
//         }
//     }

//     void
//     process_input(dataless_ui_context) override
//     {
//     }

//     matrix<3, 3, double>
//     transformation() const override
//     {
//         return parent->transformation();
//     }

//     layout_box
//     bounding_box() const override
//     {
//         return this->cacher.relative_assignment.region;
//     }

//     void
//     reveal_region(region_reveal_request const& request) override
//     {
//         parent->reveal_region(request);
//     }

//     // implementation of layout interface
//     layout_requirements
//     get_horizontal_requirements() override;
//     layout_requirements
//     get_vertical_requirements(layout_scalar assigned_width) override;
//     void
//     set_relative_assignment(
//         relative_layout_assignment const& assignment) override;

//     void
//     record_content_change(
//         layout_traversal<widget_container, widget>& traversal) override;
//     void
//     record_self_change(layout_traversal<widget_container, widget>&
//     traversal);

//     layout_cacher cacher;

//     // cached requirements for cells within this row
//     grid_column_requirements<Uniformity> requirements;
//     counter_type last_content_query = 0;

//     // reference to the data for the grid that this row belongs to
//     grid_data<Uniformity>* grid = nullptr;

//     // next row in this grid
//     grid_row_container* next = nullptr;
// };

// // Update the requirements for a grid's columns by querying its contents.
// template<class Uniformity>
// void
// update_grid_column_requirements(grid_data<Uniformity>& grid)
// {
//     // Only update if something in the grid has changed since the last
//     // update.
//     if (grid.last_content_query != grid.container->last_content_change)
//     {
//         // Clear the requirements for the grid and recompute them
//         // by iterating through the rows and folding each row's
//         // requirements into the main grid requirements.
//         clear_requirements(grid.requirements);
//         for (grid_row_container<Uniformity>* row = grid.rows; row;
//              row = row->next)
//         {
//             // Again, only update if something in the row has
//             // changed.
//             if (row->last_content_query != row->last_content_change)
//             {
//                 clear_requirements(row->requirements);
//                 for (widget* child = row->widget_container::children; child;
//                      child = child->next)
//                 {
//                     layout_requirements x
//                         = child->get_horizontal_requirements();
//                     add_requirements(row->requirements, x);
//                 }
//                 row->last_content_query = row->last_content_change;
//             }
//             fold_in_requirements(grid.requirements, row->requirements);
//         }
//         grid.last_content_query = grid.container->last_content_change;
//     }
// }

// template<class Uniformity>
// layout_scalar
// get_required_width(grid_data<Uniformity> const& grid)
// {
//     size_t n_columns = get_column_count(grid.requirements);
//     layout_scalar width = 0;
//     for (size_t i = 0; i != n_columns; ++i)
//         width += get_column_requirements(grid.requirements, i).size;
//     if (n_columns > 0)
//         width += grid.column_spacing * layout_scalar(n_columns - 1);
//     return width;
// }

// template<class Uniformity>
// float
// get_total_growth(grid_data<Uniformity> const& grid)
// {
//     size_t n_columns = get_column_count(grid.requirements);
//     float growth = 0;
//     for (size_t i = 0; i != n_columns; ++i)
//         growth += get_column_requirements(grid.requirements,
//         i).growth_factor;
//     return growth;
// }

// template<class Uniformity>
// layout_requirements
// grid_row_container<Uniformity>::get_horizontal_requirements()
// {
//     return cache_horizontal_layout_requirements(
//         cacher, grid->container->last_content_change, [&] {
//             update_grid_column_requirements(*grid);
//             return calculated_layout_requirements{
//                 get_required_width(*grid), 0, 0};
//         });
// }

// template<class Uniformity>
// std::vector<layout_scalar> const&
// calculate_column_assignments(
//     grid_data<Uniformity>& grid, layout_scalar assigned_width)
// {
//     if (grid.last_assignments_update != grid.container->last_content_change)
//     {
//         update_grid_column_requirements(grid);
//         size_t n_columns = get_column_count(grid.requirements);
//         grid.assignments.resize(n_columns);
//         layout_scalar required_width = get_required_width(grid);
//         float total_growth = get_total_growth(grid);
//         layout_scalar extra_width = assigned_width - required_width;
//         for (size_t i = 0; i != n_columns; ++i)
//         {
//             layout_scalar width
//                 = get_column_requirements(grid.requirements, i).size;
//             if (total_growth != 0)
//             {
//                 float growth_factor
//                     = get_column_requirements(grid.requirements, i)
//                           .growth_factor;
//                 layout_scalar extra = round_to_layout_scalar(
//                     (growth_factor / total_growth) * extra_width);
//                 extra_width -= extra;
//                 total_growth -= growth_factor;
//                 width += extra;
//             }
//             grid.assignments[i] = width;
//         }
//         grid.last_assignments_update = grid.container->last_content_change;
//     }
//     return grid.assignments;
// }

// calculated_layout_requirements
// calculate_grid_row_vertical_requirements(
//     grid_data<nonuniform_grid_tag>& grid,
//     grid_row_container<nonuniform_grid_tag>& row,
//     layout_scalar assigned_width)
// {
//     std::vector<layout_scalar> const& column_widths
//         = calculate_column_assignments(grid, assigned_width);
//     calculated_layout_requirements requirements{0, 0, 0};
//     size_t column_index = 0;
//     walk_layout_nodes(row.children, [&](layout_node_interface& node) {
//         fold_in_requirements(
//             requirements,
//             node.get_vertical_requirements(column_widths[column_index]));
//         ++column_index;
//     });
//     return requirements;
// }

// calculated_layout_requirements
// calculate_grid_row_vertical_requirements(
//     grid_data<uniform_grid_tag>& grid,
//     grid_row_container<uniform_grid_tag>& /*row*/,
//     layout_scalar assigned_width)
// {
//     named_block nb;
//     auto& cache = grid.vertical_requirements_cache;
//     if (cache.last_update != grid.container->last_content_change)
//     {
//         update_grid_column_requirements(grid);

//         std::vector<layout_scalar> const& widths
//             = calculate_column_assignments(grid, assigned_width);

//         calculated_layout_requirements& grid_requirements =
//         cache.requirements; grid_requirements =
//         calculated_layout_requirements{0, 0, 0}; for
//         (grid_row_container<uniform_grid_tag>* row = grid.rows; row;
//              row = row->next)
//         {
//             size_t column_index = 0;
//             walk_layout_nodes(row->children, [&](layout_node_interface&
//             node) {
//                 fold_in_requirements(
//                     grid_requirements,
//                     node.get_vertical_requirements(widths[column_index]));
//                 ++column_index;
//             });
//         }

//         cache.last_update = grid.container->last_content_change;
//     }
//     return cache.requirements;
// }

// template<class Uniformity>
// layout_requirements
// grid_row_container<Uniformity>::get_vertical_requirements(
//     layout_scalar assigned_width)
// {
//     return cache_vertical_layout_requirements(
//         cacher, grid->container->last_content_change, assigned_width, [&] {
//             return calculate_grid_row_vertical_requirements(
//                 *grid, *this, assigned_width);
//         });
// }

// template<class Uniformity>
// void
// set_grid_row_relative_assignment(
//     grid_data<Uniformity>& grid,
//     widget* children,
//     layout_vector const& assigned_size,
//     layout_scalar assigned_baseline_y)
// {
//     std::vector<layout_scalar> const& column_widths
//         = calculate_column_assignments(grid, assigned_size[0]);
//     size_t n = 0;
//     layout_vector p = make_layout_vector(0, 0);
//     for (widget* i = children; i; i = i->next, ++n)
//     {
//         layout_scalar this_width = column_widths[n];
//         i->set_relative_assignment(relative_layout_assignment{
//             layout_box(p, make_layout_vector(this_width, assigned_size[1])),
//             assigned_baseline_y});
//         p[0] += this_width + grid.column_spacing;
//     }
// }

// template<class Uniformity>
// void
// grid_row_container<Uniformity>::set_relative_assignment(
//     relative_layout_assignment const& assignment)
// {
//     update_relative_assignment(
//         *this,
//         cacher,
//         grid->container->last_content_change,
//         assignment,
//         [&](auto const& resolved_assignment) {
//             set_grid_row_relative_assignment(
//                 *grid,
//                 widget_container::children,
//                 resolved_assignment.region.size,
//                 resolved_assignment.baseline_y);
//         });
// }

// template<class Uniformity>
// void
// grid_row_container<Uniformity>::record_content_change(
//     layout_traversal<widget_container, widget>& traversal)
// {
//     if (this->last_content_change != traversal.refresh_counter)
//     {
//         this->last_content_change = traversal.refresh_counter;
//         if (this->parent)
//             this->parent->record_content_change(traversal);
//         for (grid_row_container<Uniformity>* row = this->grid->rows; row;
//              row = row->next)
//         {
//             row->record_self_change(traversal);
//         }
//     }
// }

// template<class Uniformity>
// void
// grid_row_container<Uniformity>::record_self_change(
//     layout_traversal<widget_container, widget>& traversal)
// {
//     if (this->last_content_change != traversal.refresh_counter)
//     {
//         this->last_content_change = traversal.refresh_counter;
//         if (this->parent)
//             this->parent->record_content_change(traversal);
//     }
// }

// void
// scoped_grid_row::begin(
//     ui_context, scoped_grid_layout const& grid, layout const& layout_spec)
// {
//     layout_traversal<widget_container, widget>& traversal =
//     *grid.traversal_;

//     grid_row_container<nonuniform_grid_tag>* row;
//     if (get_cached_data(*grid.data_traversal_, &row))
//         initialize(traversal, *row);

//     refresh_grid_row(traversal, *grid.data_, *row, layout_spec);

//     container_.begin(traversal, row);
// }

// void
// scoped_grid_row::end()
// {
//     container_.end();
// }

// struct collapsible_container : widget_container
// {
//     column_layout_logic* logic;
//     layout_cacher cacher;
//     layout_vector assigned_size;

//     value_smoother<float> smoother;

//     float offset_factor = 1;

//     // expansion fraction (0 to 1)
//     float expansion_ = 0;

//     // The following are filled in during layout...

//     // actual content height
//     layout_scalar content_height;

//     // window through which the content is visible
//     layout_box window;

//     void
//     refresh(dataless_ui_context ctx, float expansion)
//     {
//         auto smoothed_expansion = smooth_raw(
//             ctx,
//             this->smoother,
//             expansion,
//             animated_transition{default_curve, 160});

//         if (this->expansion_ != smoothed_expansion)
//         {
//             this->last_content_change
//                 = get_layout_traversal(ctx).refresh_counter;
//         }

//         detect_layout_change(
//             get_layout_traversal(ctx), &this->expansion_,
//             smoothed_expansion);
//     }

//     layout_requirements
//     get_horizontal_requirements() override
//     {
//         return cache_horizontal_layout_requirements(
//             cacher, last_content_change, [&] {
//                 calculated_layout_requirements x
//                     = logic->get_horizontal_requirements(children);
//                 return calculated_layout_requirements{x.size, 0, 0};
//             });
//     }

//     layout_requirements
//     get_vertical_requirements(layout_scalar assigned_width) override
//     {
//         return cache_vertical_layout_requirements(
//             cacher, last_content_change, assigned_width, [&] {
//                 layout_scalar resolved_width = resolve_assigned_width(
//                     this->cacher.resolved_spec,
//                     assigned_width,
//                     this->get_horizontal_requirements());
//                 calculated_layout_requirements y
//                     = logic->get_vertical_requirements(
//                         children, resolved_width);
//                 layout_scalar content_height = y.size;
//                 layout_scalar visible_height = round_to_layout_scalar(
//                     float(content_height) * this->expansion_);
//                 this->content_height = content_height;
//                 return calculated_layout_requirements{visible_height, 0, 0};
//             });
//     }

//     void
//     set_relative_assignment(
//         relative_layout_assignment const& assignment) override
//     {
//         update_relative_assignment(
//             *this,
//             cacher,
//             last_content_change,
//             assignment,
//             [&](auto const& resolved_assignment) {
//                 calculated_layout_requirements y
//                     = logic->get_vertical_requirements(
//                         children, resolved_assignment.region.size[0]);
//                 logic->set_relative_assignment(
//                     children,
//                     make_layout_vector(
//                         resolved_assignment.region.size[0], y.size),
//                     y.size - y.descent);
//                 this->window = resolved_assignment.region;
//             });
//     }

//     void
//     render(render_event& event) override
//     {
//         auto const& region = get_assignment(this->cacher).region;
//         SkRect bounds;
//         bounds.fLeft = SkScalar(region.corner[0] + event.current_offset[0]);
//         bounds.fTop = SkScalar(region.corner[1] + event.current_offset[1]);
//         bounds.fRight = bounds.fLeft + SkScalar(region.size[0]);
//         bounds.fBottom = bounds.fTop + SkScalar(region.size[1]);
//         if (!event.canvas->quickReject(bounds))
//         {
//             event.canvas->save();
//             auto original_offset = event.current_offset;
//             event.canvas->clipRect(bounds);
//             event.current_offset += region.corner;
//             layout_scalar content_offset = round_to_layout_scalar(
//                 this->offset_factor * (1 - expansion_) *
//                 this->content_height);
//             event.current_offset[1] -= content_offset;
//             alia::render_children(event, *this);
//             event.current_offset = original_offset;
//             event.canvas->restore();
//         }
//     }

//     void
//     hit_test(
//         hit_test_base& test, vector<2, double> const& point) const override
//     {
//         auto const& region = get_assignment(this->cacher).region;
//         if (is_inside(region, vector<2, float>(point)))
//         {
//             auto local_point = point - vector<2, double>(region.corner);
//             layout_scalar content_offset = round_to_layout_scalar(
//                 this->offset_factor * (1 - expansion_) *
//                 this->content_height);
//             local_point[1] += content_offset;
//             for (widget* node = this->widget_container::children; node;
//                  node = node->next)
//             {
//                 node->hit_test(test, local_point);
//             }
//         }
//     }

//     void
//     process_input(dataless_ui_context) override
//     {
//     }

//     matrix<3, 3, double>
//     transformation() const override
//     {
//         // TODO
//         return parent->transformation();
//     }

//     layout_box
//     bounding_box() const override
//     {
//         return this->cacher.relative_assignment.region;
//     }

//     void
//     reveal_region(region_reveal_request const& request) override
//     {
//         parent->reveal_region(request);
//     }
// };

// void
// get_collapsible_view(
//     ui_context ctx,
//     std::shared_ptr<collapsible_container>** container,
//     readable<bool> expanded,
//     layout const& layout_spec)
// {
//     if (get_data(ctx, container))
//         **container = std::make_shared<collapsible_container>();

//     if (get_layout_traversal(ctx).is_refresh_pass)
//     {
//         (**container)->refresh(ctx, condition_is_true(expanded) ? 1.f :
//         0.f);

//         if (update_layout_cacher(
//                 get_layout_traversal(ctx),
//                 (**container)->cacher,
//                 layout_spec,
//                 FILL | UNPADDED))
//         {
//             // Since this container isn't active yet, it didn't get marked
//             // as needing recalculation, so we need to do that manually
//             // here.
//             (**container)->last_content_change
//                 = get_layout_traversal(ctx).refresh_counter;
//         }
//     }
// }

// struct scoped_collapsible
// {
//     scoped_collapsible()
//     {
//     }
//     scoped_collapsible(
//         ui_context ctx,
//         readable<bool> expanded,
//         layout const& layout_spec = default_layout)
//     {
//         begin(ctx, expanded, layout_spec);
//     }
//     ~scoped_collapsible()
//     {
//         end();
//     }

//     bool
//     do_content() const
//     {
//         return container_->expansion_ != 0;
//     }

//     void
//     begin(
//         ui_context ctx,
//         readable<bool> expanded,
//         layout const& layout_spec = default_layout)
//     {
//         std::shared_ptr<collapsible_container>* container;
//         get_collapsible_view(ctx, &container, expanded, layout_spec);
//         container_ = container->get();
//         scoping_.begin(get_layout_traversal(ctx), container_);
//     }
//     void
//     end()
//     {
//         scoping_.end();
//     }

//  private:
//     collapsible_container* container_;
//     scoped_layout_container scoping_;
// };

// template<class Content>
// void
// collapsible_content(
//     ui_context ctx, readable<bool> show_content, Content&& content)
// {
//     scoped_collapsible collapsible(ctx, show_content);
//     if_(ctx, collapsible.do_content(), std::forward<Content>(content));
// }

// ///

// struct panel_container : simple_layout_container<column_layout_logic>
// {
//     void
//     render(render_event& event) override
//     {
//         auto const& region = get_assignment(this->cacher).region;
//         SkRect bounds;
//         bounds.fLeft = SkScalar(region.corner[0] + event.current_offset[0]);
//         bounds.fTop = SkScalar(region.corner[1] + event.current_offset[1]);
//         bounds.fRight = bounds.fLeft + SkScalar(region.size[0]);
//         bounds.fBottom = bounds.fTop + SkScalar(region.size[1]);
//         if (!event.canvas->quickReject(bounds))
//         {
//             SkPaint paint;
//             paint.setColor(SkColorSetRGB(0x31, 0x38, 0x44));
//             event.canvas->drawRect(bounds, paint);
//             alia::render_children(event, *this);
//         }
//     }

//     void
//     hit_test(
//         hit_test_base& test, vector<2, double> const& point) const override
//     {
//         auto const& region = get_assignment(this->cacher).region;
//         if (is_inside(region, vector<2, float>(point)))
//         {
//             auto local_point = point - vector<2, double>(region.corner);
//             for (widget* node = this->widget_container::children; node;
//                  node = node->next)
//             {
//                 node->hit_test(test, local_point);
//             }
//         }
//     }

//     void
//     process_input(dataless_ui_context) override
//     {
//     }

//     matrix<3, 3, double>
//     transformation() const override
//     {
//         return parent->transformation();
//     }

//     relative_layout_assignment const&
//     assignment() const
//     {
//         return cacher.relative_assignment;
//     }

//     layout_box
//     bounding_box() const override
//     {
//         return add_border(this->assignment().region, 4.f);
//     }

//     void
//     reveal_region(region_reveal_request const& request) override
//     {
//         parent->reveal_region(request);
//     }
// };

// void
// get_panel_container(
//     ui_context ctx,
//     std::shared_ptr<panel_container>** container,
//     layout const& layout_spec)
// {
//     if (get_data(ctx, container))
//         **container = std::make_shared<panel_container>();

//     if (get_layout_traversal(ctx).is_refresh_pass)
//     {
//         if (update_layout_cacher(
//                 get_layout_traversal(ctx),
//                 (**container)->cacher,
//                 layout_spec,
//                 FILL | UNPADDED))
//         {
//             // Since this container isn't active yet, it didn't get marked
//             // as needing recalculation, so we need to do that manually
//             // here.
//             (**container)->last_content_change
//                 = get_layout_traversal(ctx).refresh_counter;
//         }
//     }
// }

// // rgb(49, 58, 70)

// struct scoped_panel
// {
//     scoped_panel()
//     {
//     }
//     scoped_panel(ui_context ctx, layout const& layout_spec = default_layout)
//     {
//         begin(ctx, layout_spec);
//     }
//     ~scoped_panel()
//     {
//         end();
//     }

//     void
//     begin(ui_context ctx, layout const& layout_spec = default_layout)
//     {
//         std::shared_ptr<panel_container>* container;
//         get_panel_container(ctx, &container, layout_spec);
//         container_ = container->get();
//         scoping_.begin(get_layout_traversal(ctx), container_);
//     }
//     void
//     end()
//     {
//         scoping_.end();
//     }

//  private:
//     panel_container* container_;
//     scoped_layout_container scoping_;
// };

// template<class Content>
// void
// panel(ui_context ctx, Content&& content)
// {
//     scoped_panel panel(ctx);
//     std::forward<Content>(content);
// }

// } // namespace alia

namespace alia {
struct culling_block
{
    culling_block()
    {
    }
    culling_block(ui_context& ctx, layout const& layout_spec = default_layout)
    {
        begin(ctx, layout_spec);
    }
    ~culling_block()
    {
        end();
    }
    void
    begin(ui_context& ctx, layout const& layout_spec = default_layout);
    void
    end();
    bool
    is_relevant() const
    {
        return is_relevant_;
    }

 private:
    ui_context* ctx_;
    scoped_component_container scc_;
    column_layout layout_;
    bool is_relevant_;
};

void
culling_block::begin(ui_context& ctx, layout const& layout_spec)
{
    ctx_ = &ctx;
    scc_.begin(ctx);
    layout_.begin(ctx, layout_spec);
    switch (get_event_category(ctx))
    {
        case REFRESH_CATEGORY:
            is_relevant_ = true;
            break;
        case RENDER_CATEGORY:
            is_relevant_ = is_visible(
                get_geometry_context(ctx), box<2, double>(layout_.region()));
            break;
        case REGION_CATEGORY:
            if (get_event_type(ctx) == MOUSE_HIT_TEST_EVENT
                || get_event_type(ctx) == WHEEL_HIT_TEST_EVENT)
            {
                is_relevant_ = is_mouse_inside_box(
                    ctx, box<2, double>(layout_.region()));
                break;
            }
            // Other region events fall through.
        default:
            is_relevant_ = scc_.is_on_route();
    }
}
void
culling_block::end()
{
    if (ctx_)
    {
        layout_.end();
        scc_.end();
        ctx_ = 0;
    }
}

}

#define alia_culling_block_(ctx, layout_spec)                                 \
    {                                                                         \
        ::alia::culling_block alia__culling_block(ctx, layout_spec);          \
        {                                                                     \
            ::alia::event_dependent_if_block alia__if_block(                  \
                get_data_traversal(ctx), alia__culling_block.is_relevant());  \
            if (alia__culling_block.is_relevant())                            \
            {

#define alia_culling_block(layout_spec) alia_culling_block_(ctx, layout_spec)

namespace alia {
struct layout_node;
struct ui_caching_node;

struct cached_ui_block
{
    cached_ui_block() : ctx_(0)
    {
    }
    cached_ui_block(
        ui_context& ctx,
        id_interface const& id,
        layout const& layout_spec = default_layout)
    {
        begin(ctx, id, layout_spec);
    }
    ~cached_ui_block()
    {
        end();
    }
    void
    begin(
        ui_context& ctx,
        id_interface const& id,
        layout const& layout_spec = default_layout);
    void
    end();
    bool
    is_relevant() const
    {
        return is_relevant_;
    }

 private:
    ui_context* ctx_;
    ui_caching_node* cacher_;
    culling_block culling_;
    bool is_relevant_;
    layout_node** layout_next_ptr_;
};

struct ui_caching_node
{
    // ui_caching_node* parent;

    // cached layout info
    bool layout_valid;
    captured_id layout_id;
    layout_node* layout_subtree_head;
    layout_node** layout_subtree_tail;
};

void
cached_ui_block::begin(
    ui_context& ctx, id_interface const& id, layout const& layout_spec)
{
    ctx_ = &ctx;

    culling_.begin(ctx, layout_spec);

    get_cached_data(ctx, &cacher_);
    ui_caching_node& cacher = *cacher_;

    // cacher.parent = ctx.active_cacher;
    // ctx.active_cacher = &cacher;

    // Caching content in the middle of a validation block is not
    // currently supported. assert(!ctx.validation.detection &&
    // !ctx.validation.reporting);

    // Before doing anything else, see if the content can be culled by
    // the culling block's criteria.
    if (!culling_.is_relevant())
    {
        is_relevant_ = false;
        return;
    }

    if (get_event_type(ctx) == REFRESH_EVENT)
    {
        // Detect if there are changes that require the block to be
        // traversed this pass.
        is_relevant_ = !cacher.layout_valid || !cacher.layout_id.matches(id);
        // If we're going to actually update the layout, record the
        // current value of the layout context's next_ptr, so we'll
        // know where to look for the address of the first node.
        if (is_relevant_)
        {
            layout_next_ptr_ = get_layout_traversal(ctx).next_ptr;
            // Store the ID here because it's only available within
            // this function.
            cacher.layout_id.capture(id);
            // Need to mark it valid here because it could be
            // invalidated by something inside the block. (Is this
            // dangerous?)
            cacher.layout_valid = true;
        }
        // Otherwise, just splice in the cached subtree.
        else
        {
            *get_layout_traversal(ctx).next_ptr = cacher.layout_subtree_head;
            get_layout_traversal(ctx).next_ptr = cacher.layout_subtree_tail;
        }
    }
    else
        is_relevant_ = true;
}
void
cached_ui_block::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;
        ui_caching_node& cacher = *cacher_;

        // If the layout was just updated, record the head and tail of
        // the layout subtree so we can splice it into the parent tree
        // on passes where we skip layout.
        if (is_refresh_pass(ctx) && is_relevant_)
        {
            cacher.layout_subtree_head = *layout_next_ptr_;
            cacher.layout_subtree_tail = get_layout_traversal(ctx).next_ptr;
        }

        switch (get_event_type(ctx))
        {
            case REFRESH_EVENT:
            case RENDER_EVENT:
            case MOUSE_HIT_TEST_EVENT:
            case WHEEL_HIT_TEST_EVENT:
                break;
            default:
                // Any other event that makes it into the block could
                // potentially cause a state change, so record a
                // change.
                cacher.layout_valid = false;
        }

        culling_.end();

        // ctx.active_cacher = cacher.parent;

        ctx_ = 0;
    }
}

#define alia_cached_ui_block_(ctx, id, layout_spec)                           \
    {                                                                         \
        ::alia::cached_ui_block alia__cached_ui_block(ctx, id);               \
        {                                                                     \
            ::alia::event_dependent_if_block alia__if_block(                  \
                get_data_traversal(ctx),                                      \
                alia__cached_ui_block.is_relevant());                         \
            if (alia__cached_ui_block.is_relevant())                          \
            {

#define alia_cached_ui_block(id, layout_spec)                                 \
    alia_cached_ui_block_(ctx, id, layout_spec)

}

void
my_ui(ui_context ctx)
{
    scoped_scrollable_view scrollable(ctx, GROW); //, 3, 2);

    // scoped_panel panel(ctx, GROW | UNPADDED);

    column_layout column(ctx, GROW | PADDED);

    auto selected = get_state(ctx, int(0));

    auto my_style
        = text_style{"roboto/Roboto-Regular", 22.f, rgb8(173, 181, 189)};

    panel_style_info pstyle{
        box_border_width<float>{4, 4, 4, 4},
        box_border_width<float>{0, 0, 0, 0},
        box_border_width<float>{4, 4, 4, 4}};

    do_text(ctx, direct(my_style), value("Lorem ipsum"));

    do_spacer(ctx, size(20, 100, PIXELS));

    do_slider(ctx, get_state(ctx, value(1.5)), 0, 5, 0.01);

    do_spacer(ctx, size(20, 100, PIXELS));

    {
        row_layout blah(ctx);

        do_spacer(ctx, size(100, 100, PIXELS));

        {
            column_layout col(ctx);

            auto state1 = get_state(ctx, false);
            do_switch(ctx, disable_writes(state1));
            do_switch(ctx, state1);
            do_switch(ctx, get_state(ctx, false));
            do_switch(ctx, get_state(ctx, false));
        }

        do_spacer(ctx, size(100, 100, PIXELS));

        {
            column_layout col(ctx);
            auto state1 = get_state(ctx, false);
            {
                row_layout row(ctx);
                do_checkbox(ctx, disable_writes(state1));
                // scoped_transformation transform(
                //     ctx, translation_matrix(make_vector(0., 1.)));
                do_text(ctx, direct(my_style), value("One"), CENTER_Y);
            }
            {
                row_layout row(ctx);
                do_checkbox(ctx, state1);
                // scoped_transformation transform(
                //     ctx, translation_matrix(make_vector(0., 1.)));
                do_text(ctx, direct(my_style), value("Two"), CENTER_Y);
            }
            {
                row_layout row(ctx);
                do_checkbox(ctx, get_state(ctx, false));
                // scoped_transformation transform(
                //     ctx, translation_matrix(make_vector(0., 1.)));
                do_text(ctx, direct(my_style), value("Three"), CENTER_Y);
            }
        }

        do_spacer(ctx, size(100, 100, PIXELS));

        {
            column_layout col(ctx);
            {
                row_layout row(ctx);
                do_radio_button(
                    ctx,
                    disable_writes(make_radio_signal(selected, value(2))));
                do_text(ctx, direct(my_style), value("One"), CENTER_Y);
            }
            {
                row_layout row(ctx);
                auto id = get_widget_id(ctx);
                do_box_region(ctx, id, row.region());
                do_radio_button(
                    ctx,
                    make_radio_signal(selected, value(2)),
                    default_layout,
                    id);
                do_text(ctx, direct(my_style), value("Two"), CENTER_Y);
            }
            {
                row_layout row(ctx);
                do_radio_button(ctx, make_radio_signal(selected, value(3)));
                do_text(ctx, direct(my_style), value("Three"), CENTER_Y);
            }
        }
    }

    do_spacer(ctx, size(20, 100, PIXELS));

    // {
    //     scoped_grid_layout grid(ctx);
    //     for (int i = 0; i != 4; ++i)
    //     {
    //         {
    //             scoped_grid_row row(ctx, grid);
    //             do_box(
    //                 ctx, SK_ColorMAGENTA, actions::noop(), width(200,
    //                 PIXELS));
    //             do_box(
    //                 ctx, SK_ColorMAGENTA, actions::noop(), width(200,
    //                 PIXELS));
    //         }
    //         {
    //             scoped_grid_row row(ctx, grid);
    //         }
    //     }
    // }

    for (int i = 0; i != 0; ++i)
    {
        alia_cached_ui_block(unit_id, default_layout)
        {
            panel p(ctx, direct(pstyle));
            // do_text(ctx, direct(my_style), value("Lorem ipsum"));
            //  column_layout column(ctx);
            flow_layout flow(ctx);
            for (int j = 0; j != 100; ++j)
            {
                do_box(ctx, SK_ColorLTGRAY, actions::noop());
                do_box(ctx, SK_ColorDKGRAY, actions::noop());
                do_box(ctx, SK_ColorMAGENTA, actions::noop());
                do_box(ctx, SK_ColorLTGRAY, actions::noop());
                do_box(ctx, SK_ColorMAGENTA, actions::noop());
                do_box(ctx, SK_ColorRED, actions::noop());
                do_box(ctx, SK_ColorBLUE, actions::noop());
            }
        }
        alia_end
    }

    // auto show_text = get_state(ctx, false);

    // auto show_other_text = get_state(ctx, false);

    // {
    //     scoped_flow_layout row(ctx, UNPADDED | FILL);
    //     do_box(ctx, SK_ColorLTGRAY, actions::toggle(show_text));
    //     do_box(ctx, SK_ColorLTGRAY, actions::toggle(show_other_text));
    //     do_tree_expander(ctx);
    //     // do_svg_image(ctx);
    // }

    // collapsible_content(ctx, show_other_text, [&] {
    //     do_spacer(ctx, height(20, PIXELS));
    //     do_text(ctx, value("Könnten Sie mir das übersetzen?"));
    //     do_wrapped_text(
    //         ctx,
    //         value("\xce\xa3\xe1\xbd\xb2\x20\xce\xb3\xce\xbd\xcf\x89\xcf\x81"
    //               "\xe1\xbd\xb7\xce\xb6\xcf\x89\x20\xe1\xbc\x80\xcf\x80\xe1"
    //               "\xbd\xb8\x20\xcf\x84\xe1\xbd\xb4\xce\xbd\x20\xce\xba\xe1"
    //               "\xbd\xb9\xcf\x88\xce\xb7"));
    //     do_wrapped_text(
    //         ctx,
    //         value("\x58\x20\x2d\x20\xd9\x82\xd9\x84\xd9\x85\x20\xd8\xb1\xd8"
    //               "\xb5\xd8\xa7\xd8\xb5\x20\x2d\x20\x59"));
    //     do_wrapped_text(
    //         ctx,
    //         value("\xe7\x8e\x8b\xe6\x98\x8e\xef\xbc\x9a\xe8\xbf\x99\xe6\x98"
    //               "\xaf\xe4\xbb\x80\xe4\xb9\x88\xef\xbc\x9f"));

    // {
    //     grid_layout grid(ctx);
    //     for (int i = 0; i != 4; ++i)
    //     {
    //         {
    //             grid_row row(grid);
    //             do_box(
    //                 ctx, SK_ColorMAGENTA, actions::noop(), width(200,
    //                 PIXELS));
    //             do_box(
    //                 ctx, SK_ColorMAGENTA, actions::noop(), width(200,
    //                 PIXELS));
    //         }
    //         {
    //             grid_row row(grid);
    //             do_box(ctx, SK_ColorLTGRAY, actions::noop());
    //             do_box(ctx, SK_ColorLTGRAY, actions::noop());
    //         }
    //     }
    // }

    // do_spacer(ctx, height(100, PIXELS));

    {
        for (int outer = 0; outer != 0; ++outer)
        {
            do_wrapped_text(
                ctx,
                direct(my_style),
                value("Lorem ipsum dolor sit amet, consectetur "
                      "adipisg elit. "
                      "Phasellus lacinia elementum diam consequat "
                      "alicinquet. "
                      "Vestibulum ut libero justo. Pellentesque lectus "
                      "lectus, "
                      "scelerisque a elementum sed, bibendum id libero. "
                      "Maecenas venenatis est sed sem "
                      "consequat mollis. Ut "
                      "nequeodio, hendrerit ut justo venenatis, consequat "
                      "molestie eros. Nam fermentum, mi malesuada eleifend"
                      "dapibus, lectus dolor luctus orci, nec posuere lor "
                      "lorem ac sem. Nullam interdum laoreet ipsum in "
                      "dictum.\n\n"
                      "Duis quis blandit felis. Pellentesque et lectus "
                      "magna. "
                      "Maecenas dui lacus, sollicitudin a eros in, vehicula "
                      "posuere metus. Etiam nec dolor mattis, tincidunt sem "
                      "vitae, maximus tellus. Vestibulum ut nisi nec neque "
                      "rutrum interdum. In justo massa, consequat quis dui "
                      "eget, cursus ultricies sem. Quisque a lectus quisest "
                      "porttitor ullamcorper ac sed odio.\n\n"
                      "Vestibulum sed turpis et lacus rutrum scelerisqueet "
                      "sit "
                      "amet ipsum. Sed congue hendrerit augue, sed "
                      "pellentesque "
                      "neque. Integer efficitur nisi idmassa placerat, at "
                      "ullamcorper arcu fermentum. Donec sed tellus quis "
                      "velit "
                      "placerat viverra necvel diam. Vestibulum faucibus "
                      "molestie ipsum eget rhoncus. Donec vitae bibendum "
                      "nibh, "
                      "quis pellentesque enim. Donec eget consectetur "
                      "massa, "
                      "eget mollis elit. Orci varius natoque penatibus et"
                      "magnis dis parturient montes, nascetur ridiculus "
                      "mus. "
                      "Donec accumsan, nisi egestas "
                      "sollicitudinullamcorper, "
                      "diam dolor faucibus neque, euscelerisque mi erat vel "
                      "erat. Etiam nec leo acrisus porta ornare ut accumsan "
                      "lorem.\n\n"
                      "Aenean at euismod ligula. Sed augue est, imperdietut "
                      "sem sit amet, efficitur dictum enim. Namsodales id "
                      "risus non pulvinar. Morbi id mollismassa. Nunc elit "
                      "velit, pellentesque et lobortisut, luctus sed justo. "
                      "Morbi eleifend quam velmagna accumsan, eu consequat "
                      "nisl ultrices.Mauris dictum eu quam sit amet "
                      "aliquet. "
                      "Sedrhoncus turpis quis sagittis imperdiet. Lorem "
                      "ipsum"
                      "dolor sit amet, consectetur adipiscing elit. "
                      "Pellentesque convallis suscipit ex et "
                      "hendrerit.Donec "
                      "est ex, varius eu nulla id, tristiquelobortis metus. "
                      "Sed id sem justo. Nulla at portaneque, id elementum "
                      "lacus.\n\n"
                      "Mauris leotortor, tincidunt sit amet sem sit amet, "
                      "egestastempor massa. In diam ipsum, fermentum "
                      "pulvinar "
                      "posuere ut, scelerisque sit amet odio. Nam necjusto "
                      "quis felis ultrices ornare sit amet eumassa. Nam "
                      "gravida lacus eget tortor porttitor,eget scelerisque "
                      "est imperdiet. Duis quisimperdiet libero. Nullam "
                      "justo "
                      "erat, blandit etnisi sit amet, aliquet mattis leo. "
                      "Sed "
                      "a auguenon felis lacinia ultrices. Aenean porttitor "
                      "bibendum sem, in consectetur arcu suscipit id.Etiam "
                      "varius dictum gravida. Nulla molestiefermentum odio "
                      "vitae tincidunt. Quisque dictum,magna vitae "
                      "porttitor "
                      "accumsan, felis felisconsectetur nisi, ut venenatis "
                      "felis justo utante.\n\n"),
                FILL);
        }

        // do_box(
        //     ctx,
        //     SK_ColorRED,
        //     actions::toggle(show_text),
        //     size(400, 400, PIXELS));
        // do_spacer(ctx, height(100, PIXELS));

        // for (int outer = 0; outer != 2; ++outer)
        // {
        //     scoped_flow_layout flow(ctx, UNPADDED);

        //     auto my_style = text_style{
        //         "roboto/Roboto-Regular", 22.f, rgb8(173, 181, 189)};

        //     for (int i = 0; i != 100; ++i)
        //         do_text(
        //             ctx,
        //             direct(my_style),
        //             alia::printf(
        //                 ctx,
        //                 "Revenue today - Könnten Sie mir das
        //                 übersetzen?", i));

        //     for (int i = 0; i != 100; ++i)
        //     {
        //         // if_(ctx, show_text, [&] {
        //         //     // do_spacer(ctx, size(60, 40, PIXELS));
        //         //     do_text(ctx, alia::printf(ctx, "text%i", i));
        //         //     do_text(ctx, value("Könnten Sie mir das
        //         übersetzen?"));
        //         // });

        //         {
        //             scoped_column col(ctx);

        //             do_box(
        //                 ctx,
        //                 SK_ColorMAGENTA,
        //                 actions::noop(),
        //                 width(100, PIXELS));

        //             // color::yiq<std::uint8_t> y1 =
        //             // ::color::constant::blue_t{};
        //             // color::yiq<std::uint8_t> y2 =
        //             // ::color::constant::red_t{};
        //             // color::yiq<std::uint8_t> yr =
        //             // color::operation::mix(
        //             //     y1,
        //             //     std::max(
        //             //         0.0,
        //             //         std::min(
        //             //             1.0,
        //             //             std::fabs(std::sin(
        //             //                 get_raw_animation_tick_count(ctx)
        //             //                 / 1000.0)))),
        //             //     y2);
        //             // color::rgb<std::uint8_t> r(yr);
        //             // color::rgb<std::uint8_t> r(y1);

        //             do_box(ctx, SK_ColorLTGRAY, actions::noop());
        //         }

        //         {
        //             scoped_column col(ctx);

        //             static SkColor clicky_color = SK_ColorRED;
        //             // event_handler<mouse_button_event>(
        //             //     ctx, [&](auto, auto&) { clicky_color =
        //             //     SK_ColorBLUE; });
        //             do_box(ctx, clicky_color, actions::noop());

        //             do_box(ctx, SK_ColorLTGRAY, actions::noop());

        //             do_box(ctx, SK_ColorGRAY, actions::noop());
        //         }
        //     }
        //}
    }
}
