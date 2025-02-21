#include <alia/core/flow/data_graph.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/core/timing/cubic_bezier.hpp>
#include <alia/core/timing/smoothing.hpp>
#include <alia/ui.hpp>
#include <alia/ui/color.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/layout/grids.hpp>
#include <alia/ui/layout/simple.hpp>
#include <alia/ui/layout/spacer.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/library/bullets.hpp>
#include <alia/ui/library/checkbox.hpp>
#include <alia/ui/library/collapsible.hpp>
#include <alia/ui/library/lazy_list.hpp>
#include <alia/ui/library/node_expander.hpp>
#include <alia/ui/library/panels.hpp>
#include <alia/ui/library/radio_button.hpp>
#include <alia/ui/library/separator.hpp>
#include <alia/ui/library/slider.hpp>
#include <alia/ui/library/switch.hpp>
#include <alia/ui/scrolling.hpp>
#include <alia/ui/system/api.hpp>
#include <alia/ui/system/input_constants.hpp>
#include <alia/ui/system/object.hpp>
#include <alia/ui/text/fonts.hpp>
#include <alia/ui/utilities/cached_ui.hpp>
#include <alia/ui/utilities/culling.hpp>
#include <alia/ui/utilities/keyboard.hpp>
#include <alia/ui/utilities/mouse.hpp>
#include <alia/ui/utilities/regions.hpp>
#include <alia/ui/utilities/rendering.hpp>
#include <alia/ui/utilities/skia.hpp>
#include <alia/ui/utilities/styling.hpp>

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

#include "include/core/SkColor.h"
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
    action<> on_click,
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
            add_to_focus_order(ctx, id);

            if (detect_click(ctx, id, mouse_button::LEFT)
                || detect_keyboard_click(ctx, data.keyboard_click_state_, id))
            {
                // data.state_ = !data.state_;
                perform_action(on_click);
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
            // if (data.state_)
            // {
            //     c = rgb8(0x40, 0x40, 0x40);
            // }
            // else
            {
                c = rgb8(
                    std::uint8_t(SkColorGetR(color)),
                    std::uint8_t(SkColorGetG(color)),
                    std::uint8_t(SkColorGetB(color)));
            }
            if (blend_factor != 0)
            {
                c = lerp(c, rgb8(0xff, 0xff, 0xff), blend_factor);
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
                paint.setColor(SK_ColorBLUE);
                canvas.drawRect(rect, paint);
            }
            break;
        }
    }
    alia_end
}

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

// enum class color_theme
// {
//     VIOLET,
//     BLUE,
//     INDIGO,
//     PINK
// };

// color_theme selected_theme = color_theme::BLUE;

// bool light_theme = false;

// theme_colors* active_theme = nullptr;

// void
// update_theme(ui_context ctx)
// {
//     theme_colors* new_theme = nullptr;
//     switch (selected_theme)
//     {
//         case color_theme::VIOLET:
//             if (light_theme)
//                 new_theme = &violet_light_theme;
//             else
//                 new_theme = &violet_dark_theme;
//             break;
//         case color_theme::BLUE:
//             if (light_theme)
//                 new_theme = &blue_light_theme;
//             else
//                 new_theme = &blue_dark_theme;
//             break;
//         case color_theme::INDIGO:
//             if (light_theme)
//                 new_theme = &indigo_light_theme;
//             else
//                 new_theme = &indigo_dark_theme;
//             break;
//         case color_theme::PINK:
//             if (light_theme)
//                 new_theme = &pink_light_theme;
//             else
//                 new_theme = &pink_dark_theme;
//             break;
//     }
//     if (new_theme && new_theme != active_theme)
//     {
//         active_theme = new_theme;
//         get_system(ctx).theme = *new_theme;
//     }
// }

void
binary_number_ui(ui_context ctx, /*grid_layout& grid,*/ int number)
{
    panel_style_info style{
        .margin = box_border_width<float>{4, 4, 4, 4},
        .border_width = box_border_width<float>{0, 0, 0, 0},
        .padding = box_border_width<float>{8, 8, 8, 8},
        .background_color = get_system(ctx).theme.background.stronger[0].main};
    panel p(ctx, direct(style), size(100, 20, PIXELS));

    auto code_style = style_info{
        font_info{&get_font("Roboto_Mono/static/RobotoMono-Regular", 20.f)},
        get_system(ctx).theme.foreground.base.main};
    scoped_style_info scoped_style(ctx, code_style);

    auto n = get_state(ctx, number);
    row_layout row(ctx);
    do_text(ctx, printf(ctx, "%d", n), layout(width(100, PIXELS), RIGHT));
    for (int i = 0; i != 12; ++i)
    {
        auto bit = read_signal(n) & (1 << (11 - i));
        do_box(
            ctx,
            as_skcolor(
                bit ? get_system(ctx).theme.accent.base.main
                    : get_system(ctx).theme.structural.base.main),
            callback([&] { n.write(read_signal(n) ^ (1 << (11 - i))); }));
    }
}

#include "include/core/SkColor.h"
#include <algorithm>
#include <cmath>
#include <vector>

color_palette the_palette;

// color_ramp
// make_local_color_ramp(hsl seed, float step_size = 0.04)
// {
//     color_ramp ramp;
//     ramp[color_ramp_half_step_count] = to_rgb8(seed);

//     for (unsigned i = 0; i != color_ramp_half_step_count; ++i)
//     {
//         auto const adjusted_step
//             = (std::min)(step_size, seed.l / (color_ramp_half_step_count +
//             1));
//         float const lightness = seed.l - adjusted_step * (i + 1);
//         ramp[color_ramp_half_step_count - i - 1]
//             = to_rgb8({seed.h, seed.s, std::max(0.0f, lightness)});
//     }

//     for (unsigned i = 0; i != color_ramp_half_step_count; ++i)
//     {
//         auto const adjusted_step = (std::min)(
//             step_size, (1.0f - seed.l) / (color_ramp_half_step_count + 1));
//         float const lightness = seed.l + adjusted_step * (i + 1);
//         ramp[color_ramp_half_step_count + i + 1]
//             = to_rgb8({seed.h, seed.s, std::min(lightness, 1.0f)});
//     }

//     return ramp;
// }

void
show_color_ramp(ui_context ctx, color_ramp ramp)
{
    {
        row_layout row(ctx);
        for (int i = 0; i != color_ramp_step_count; ++i)
        {
            do_box(ctx, as_skcolor(ramp[i].rgb), actions::noop(), UNPADDED);
        }
    }
}

void
show_contrasting_color_pair(ui_context ctx, contrasting_color_pair pair)
{
    panel_style_info style{
        .margin = box_border_width<float>{4, 4, 4, 4},
        .border_width = box_border_width<float>{0, 0, 0, 0},
        .padding = box_border_width<float>{8, 8, 8, 8},
        .background_color = pair.main};
    panel p(ctx, direct(style), size(100, 20, PIXELS));

    auto my_style = style_info{
        font_info{&get_font("Roboto/Roboto-Regular", 20.f)}, pair.contrasting};
    scoped_style_info scoped_style(ctx, my_style);
    do_text(ctx, value("Contrasting"), UNPADDED);
}

void
show_color_swatch(ui_context ctx, color_swatch swatch)
{
    {
        row_layout row(ctx);
        show_contrasting_color_pair(ctx, swatch.base);
        show_contrasting_color_pair(ctx, swatch.stronger[0]);
        show_contrasting_color_pair(ctx, swatch.stronger[1]);
        show_contrasting_color_pair(ctx, swatch.weaker[0]);
    }
}

void
draw_thumb(
    dataless_ui_context ctx, layout_vector const& thumb_position, rgb8 color)
{
    auto& event = cast_event<render_event>(ctx);
    SkCanvas& canvas = *event.canvas;

    {
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(as_skcolor(color));
        canvas.drawPath(
            SkPath::Rect(SkRect::MakeXYWH(
                thumb_position[0] - 24.f,
                thumb_position[1] - 24.f,
                48.f,
                48.f)),
            paint);

        const SkScalar blur_sigma = 16.0f;
        const SkScalar x_drop = 2.0f;
        const SkScalar y_drop = 2.0f;

        paint.setMaskFilter(
            SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, blur_sigma, false));

        canvas.drawPath(
            SkPath::Rect(SkRect::MakeXYWH(
                thumb_position[0] - 24.f + x_drop,
                thumb_position[1] - 24.f + y_drop,
                48.f,
                48.f)),
            paint);
    }
}

void
render_demo_slider(
    dataless_ui_context ctx,
    unsigned axis,
    slider_data& data,
    duplex<double> value,
    rgb8 color,
    slider_style_info const& style)
{
    draw_track(
        ctx,
        axis,
        get_track_position(ctx, data, axis),
        get_track_length(ctx, data, axis),
        style);
    if (signal_has_value(value))
    {
        draw_thumb(
            ctx, get_thumb_position(ctx, data, axis, 0, 1, value), color);
    }
}

void
do_demo_slider(
    ui_context ctx,
    grid_layout& grid,
    bool on_off,
    millisecond_count duration,
    rgb8 color,
    char const* label,
    unit_cubic_bezier const& bezier)
{
    grid_row row(grid);
    {
        panel_style_info style{
            .margin = box_border_width<float>{4, 4, 4, 4},
            .border_width = box_border_width<float>{0, 0, 0, 0},
            .padding = box_border_width<float>{8, 8, 8, 8},
            .background_color
            = get_system(ctx).theme.background.stronger[0].main};
        panel p(ctx, direct(style), size(100, 20, PIXELS));
        do_text(ctx, value(label), CENTER_Y);
    }
    do_spacer(ctx, size(10, 10, PIXELS));
    {
        value_smoother<double>* smoother;
        get_cached_data(ctx, &smoother);
        auto& slider_data = get_slider_data(ctx);
        animated_transition transition{bezier, duration};
        if (get_event_category(ctx) == RENDER_CATEGORY)
        {
            render_demo_slider(
                ctx,
                0,
                slider_data,
                fake_writability(value(smooth_raw(
                    ctx, *smoother, on_off ? 1.0 : 0.0, transition))),
                color,
                extract_slider_style_info(ctx));
        }
        else
        {
            do_slider(
                ctx,
                slider_data,
                fake_writability(value(smooth_raw(
                    ctx, *smoother, on_off ? 1.0 : 0.0, transition))),
                0,
                1,
                0.01,
                layout(size(600, 60, PIXELS), CENTER_Y));
        }
    }
}

seed_colors const seed_sets[] = {
    {.primary = hex_color("#154DCF"),
     .secondary = hex_color("#6C36AE"),
     .tertiary = hex_color("#E01D23"),
     .neutral = hex_color("#1f212a"),
     .warning = hex_color("#FF9D00"),
     .danger = hex_color("#E01D23")},
    {.primary = hex_color("#6f42c1"),
     .secondary = hex_color("#7d8bae"),
     .tertiary = hex_color("#f1b2b2"),
     .neutral = hex_color("#1f212a"),
     .warning = hex_color("#e5857b"),
     .danger = hex_color("#d31638")},
    {.primary = hex_color("#a52e45"),
     .secondary = hex_color("#2b5278"),
     .tertiary = hex_color("#61787b"),
     .neutral = hex_color("#1f212a"),
     .warning = hex_color("#ead8b1"),
     .danger = hex_color("#d31638")},
    {.primary = hex_color("#b0edef"),
     .secondary = hex_color("#505888"),
     .tertiary = hex_color("#e1b7c5"),
     .neutral = hex_color("#1f212a"),
     .warning = hex_color("#ded0d1"),
     .danger = hex_color("#d31638")},
};

seed_colors const* seeds = &seed_sets[0];

using cubic_bezier = unit_cubic_bezier;

void
do_section_divider(ui_context ctx)
{
    do_spacer(ctx, height(40, PIXELS));
    do_separator(ctx);
    do_spacer(ctx, height(40, PIXELS));
}

void
do_heading(ui_context ctx, char const* text)
{
    auto heading_style = style_info{
        font_info{&get_font("Roboto/Roboto-Bold", 32.f)},
        get_system(ctx).theme.foreground.stronger[1].main};
    scoped_style_info scoped_style(ctx, heading_style);
    do_text(ctx, value(text));
    do_spacer(ctx, height(20, PIXELS));
}

void
do_radio_button(ui_context ctx, duplex<bool> selected, const char* label)
{
    row_layout row(ctx);
    auto id = get_widget_id(ctx);
    do_box_region(ctx, id, row.region());
    do_radio_button(ctx, selected, BASELINE_Y, id);
    do_text(ctx, value(label), BASELINE_Y);
}

void
do_checkbox(ui_context ctx, duplex<bool> checked, const char* label)
{
    row_layout row(ctx);
    auto id = get_widget_id(ctx);
    do_box_region(ctx, id, row.region());
    do_checkbox(ctx, checked, BASELINE_Y, id);
    do_text(ctx, value(label), BASELINE_Y);
}

std::vector<std::string>
split_string(char const* str)
{
    std::vector<std::string> ret;
    char const* line_start = str;
    char const* p = str;
    for (; *p; ++p)
    {
        if (*p == '\r' || *p == '\n')
        {
            if (line_start != p)
                ret.push_back(std::string(line_start, p - line_start));
            line_start = p + 1;
        }
    }
    if (line_start != p)
        ret.push_back(std::string(line_start, p - line_start));
    return ret;
}

void
do_code_snippet(ui_context ctx, std::vector<std::string> const& snippets)
{
    panel_style_info style{
        .margin = box_border_width<float>{4, 4, 4, 4},
        .border_width = box_border_width<float>{0, 0, 0, 0},
        .padding = box_border_width<float>{8, 8, 8, 8},
        .background_color = get_system(ctx).theme.background.stronger[0].main};
    panel p(ctx, direct(style), layout(size(100, 20, PIXELS), FILL));

    auto my_style = style_info{
        font_info{&get_font("Roboto_Mono/static/RobotoMono-Regular", 20.f)},
        get_system(ctx).theme.foreground.base.main};
    scoped_style_info scoped_style(ctx, my_style);

    for (auto const& snippet : snippets)
    {
        do_text(ctx, value(snippet));
    }
}

void
my_ui(ui_context ctx)
{
    static bool light_theme = false;

    static bool theme_update_needed = true;

    if (theme_update_needed)
    {
        the_palette = generate_color_palette(*seeds);

        contrast_parameters contrast;
        contrast.light_on_dark_ratio = 6;
        contrast.dark_on_light_ratio = 8;

        theme_colors theme;
        theme = generate_theme_colors(
            light_theme ? ui_lightness_mode::LIGHT_MODE
                        : ui_lightness_mode::DARK_MODE,
            *seeds,
            contrast);
        get_system(ctx).theme = theme;

        theme_update_needed = false;
    }

    auto const& theme = get_system(ctx).theme;

    auto main_style = style_info{
        font_info{&get_font("Roboto/Roboto-Regular", 22.f)},
        theme.foreground.base.main};
    scoped_style_info main_scoped_style(ctx, main_style);

    column_layout root_column(ctx, GROW);

    {
        scoped_scrollable_view scrollable(ctx, GROW);

        bordered_layout bordered(
            ctx,
            box_border_width<absolute_length>{
                absolute_length(24, PIXELS),
                absolute_length(24, PIXELS),
                absolute_length(24, PIXELS),
                absolute_length(24, PIXELS)},
            GROW);

        grid_layout code_snippet_grid(ctx);

#define DO_CODE_SNIPPET(snippet, code)                                        \
    {                                                                         \
        do_spacer(ctx, height(10, PIXELS));                                   \
        grid_row code_snippet_row(code_snippet_grid);                         \
        static std::vector<std::string> lines = split_string(snippet);        \
        do_code_snippet(ctx, lines);                                          \
        do_spacer(ctx, width(10, PIXELS));                                    \
        {                                                                     \
            column_layout code_column(ctx, CENTER_Y);                         \
            code                                                              \
        }                                                                     \
    }

        {
            do_heading(ctx, "Switches");

            DO_CODE_SNIPPET("do_switch(ctx, get_state(ctx, true));", {
                do_switch(ctx, get_state(ctx, true));
            });

            DO_CODE_SNIPPET("do_switch(ctx, get_state(ctx, false));", {
                do_switch(ctx, get_state(ctx, false));
            });

            DO_CODE_SNIPPET(
                R"(
auto state = get_state(ctx, false);
do_switch(ctx, disable_writes(state));
do_switch(ctx, state);
)",
                {
                    auto state = get_state(ctx, false);
                    do_switch(ctx, disable_writes(state));
                    do_switch(ctx, state);
                });
        }

        do_section_divider(ctx);

        {
            do_heading(ctx, "Checkboxes");

            DO_CODE_SNIPPET(
                "do_checkbox(ctx, get_state(ctx, true), \"One\");",
                { do_checkbox(ctx, get_state(ctx, true), "One"); });

            DO_CODE_SNIPPET(
                "do_checkbox(ctx, get_state(ctx, false), \"Two\");",
                { do_checkbox(ctx, get_state(ctx, false), "Two"); });

            DO_CODE_SNIPPET(
                R"(
auto state = get_state(ctx, false);
do_checkbox(ctx, state, "Three");
do_checkbox(ctx, disable_writes(state), "Disabled Three");
)",
                {
                    auto state = get_state(ctx, false);
                    do_checkbox(ctx, state, "Three");
                    do_checkbox(ctx, disable_writes(state), "Disabled Three");
                });
        }

        do_section_divider(ctx);

        {
            do_heading(ctx, "Radio Buttons");

            auto selected = get_state(ctx, int(2));
            DO_CODE_SNIPPET("auto selected = get_state(ctx, int(2));", {});

            DO_CODE_SNIPPET(
                R"(
do_radio_button(
    ctx, make_radio_signal(selected, value(0)), "Option One");
)",
                {
                    do_radio_button(
                        ctx,
                        make_radio_signal(selected, value(0)),
                        "Option One");
                });

            DO_CODE_SNIPPET(
                R"(
do_radio_button(
    ctx, make_radio_signal(selected, value(1)), "Option Two");
)",
                {
                    do_radio_button(
                        ctx,
                        make_radio_signal(selected, value(1)),
                        "Option Two");
                });

            DO_CODE_SNIPPET(
                R"(
do_radio_button(
    ctx, make_radio_signal(selected, value(2)), "Option Three");
)",
                {
                    do_radio_button(
                        ctx,
                        make_radio_signal(selected, value(2)),
                        "Option Three");
                });

            DO_CODE_SNIPPET(
                R"(
do_radio_button(
    ctx,
    disable_writes(make_radio_signal(selected, value(2))),
    "Disabled Option Three");
)",
                {
                    do_radio_button(
                        ctx,
                        disable_writes(make_radio_signal(selected, value(2))),
                        "Disabled Option Three");
                });
        }

        do_section_divider(ctx);

        {
            do_heading(ctx, "Node Expanders");

            DO_CODE_SNIPPET("do_node_expander(ctx, get_state(ctx, true));", {
                do_node_expander(ctx, get_state(ctx, true));
            });

            DO_CODE_SNIPPET("do_node_expander(ctx, get_state(ctx, false));", {
                do_node_expander(ctx, get_state(ctx, false));
            });

            DO_CODE_SNIPPET(
                R"(
auto state = get_state(ctx, false);
do_node_expander(ctx, disable_writes(state));
do_node_expander(ctx, state);
)",
                {
                    auto state = get_state(ctx, false);
                    do_node_expander(ctx, disable_writes(state));
                    do_node_expander(ctx, state);
                });
        }

        do_section_divider(ctx);

        {
            do_heading(ctx, "Transitions");

            auto duration_in_seconds = get_state(ctx, value(0.5));

            {
                auto code_style = style_info{
                    font_info{&get_font(
                        "Roboto_Mono/static/RobotoMono-Regular", 20.f)},
                    theme.foreground.base.main};
                scoped_style_info scoped_style(ctx, code_style);

                auto const duration = millisecond_count(
                    read_signal(duration_in_seconds) * 1000);
                auto on_off = square_wave(
                    ctx, value(duration + 500), value(duration + 500));

#define STRINGIFY(...) #__VA_ARGS__
#define ESCAPE(...) __VA_ARGS__

#define DO_DEMO_SLIDER(color, bezier)                                         \
    do_demo_slider(                                                           \
        ctx,                                                                  \
        code_snippet_grid,                                                    \
        condition_is_true(on_off),                                            \
        duration,                                                             \
        color,                                                                \
        STRINGIFY(bezier),                                                    \
        bezier)

                DO_DEMO_SLIDER(the_palette.primary[7].rgb, linear_curve);
                DO_DEMO_SLIDER(the_palette.secondary[7].rgb, ease_in_curve);
                DO_DEMO_SLIDER(the_palette.tertiary[7].rgb, ease_out_curve);
                DO_DEMO_SLIDER(theme.structural.base.main, ease_in_out_curve);
                DO_DEMO_SLIDER(
                    theme.danger.base.main,
                    ESCAPE(cubic_bezier{0.17, 0.67, 0.78, 0.3}));
            }

            do_spacer(ctx, height(20, PIXELS));

            {
                row_layout row(ctx);
                do_text(ctx, value("Duration"));
                do_slider(ctx, duration_in_seconds, 0, 3, 0.01, CENTER_Y);
                do_text(
                    ctx,
                    alia::printf(ctx, "%.3f s", duration_in_seconds),
                    CENTER_Y);
            }
        }

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

        // for (int i = 0; i != 0; ++i)
        // {
        //     cached_ui_block<column_layout>(ctx, unit_id, default_layout, [&]
        //     {
        //         panel p(ctx, direct(pstyle));
        //         // do_text(ctx, value("Lorem ipsum"));
        //         //  column_layout column(ctx);
        //         flow_layout flow(ctx);
        //         for (int j = 0; j != 100; ++j)
        //         {
        //             do_box(ctx, SK_ColorLTGRAY, actions::noop());
        //             do_box(ctx, SK_ColorDKGRAY, actions::noop());
        //             do_box(ctx, SK_ColorMAGENTA, actions::noop());
        //             do_box(ctx, SK_ColorLTGRAY, actions::noop());
        //             do_box(ctx, SK_ColorMAGENTA, actions::noop());
        //             do_box(ctx, SK_ColorRED, actions::noop());
        //             do_box(ctx, SK_ColorBLUE, actions::noop());
        //         }
        //     });
        // }

        // auto show_text = get_state(ctx, false);

        // {
        //     scoped_flow_layout row(ctx, UNPADDED | FILL);
        //     do_box(ctx, SK_ColorLTGRAY, actions::toggle(show_text));
        //     do_box(ctx, SK_ColorLTGRAY, actions::toggle(show_other_text));
        //     do_tree_expander(ctx);
        //     // do_svg_image(ctx);
        // }

        // {
        //     auto show_content = get_state(ctx, false);
        //     // grid_layout grid(ctx);

        //     {
        //         row_layout row(ctx);
        //         do_node_expander(ctx, show_content);
        //         do_text(ctx, value("Some Text"), CENTER_Y);
        //     }

        //     {
        //         row_layout row(ctx);
        //         do_spacer(ctx, width(40, PIXELS));
        //         collapsible_content(ctx, show_content, GROW, [&] {
        //             do_wrapped_text(
        //                 ctx,
        //                 value("Lorem ipsum dolor sit amet, consectetur "
        //                       "adipisg elit. "
        //                       "Phasellus lacinia elementum diam consequat "
        //                       "alicinquet. "
        //                       "Vestibulum ut libero justo. Pellentesque
        //                       lectus " "lectus, " "scelerisque a elementum
        //                       sed, bibendum id libero. " "Maecenas venenatis
        //                       est sed sem " "consequat mollis. Ut "
        //                       "nequeodio, hendrerit ut justo venenatis,
        //                       consequat " "molestie eros. Nam fermentum, mi
        //                       malesuada eleifend" "dapibus, lectus dolor
        //                       luctus orci, nec posuere lor " "lorem ac sem.
        //                       Nullam interdum laoreet ipsum in "
        //                       "dictum."));
        //         });
        //     }
        // }

        // do_wrapped_text(
        //     ctx,
        //     value("\xce\xa3\xe1\xbd\xb2\x20\xce\xb3\xce\xbd\xcf\x89\xcf\x81"
        //           "\xe1\xbd\xb7\xce\xb6\xcf\x89\x20\xe1\xbc\x80\xcf\x80\xe1"
        //           "\xbd\xb8\x20\xcf\x84\xe1\xbd\xb4\xce\xbd\x20\xce\xba\xe1"
        //           "\xbd\xb9\xcf\x88\xce\xb7"));
        // do_wrapped_text(
        //     ctx,
        //     value("\x58\x20\x2d\x20\xd9\x82\xd9\x84\xd9\x85\x20\xd8\xb1\xd8"
        //           "\xb5\xd8\xa7\xd8\xb5\x20\x2d\x20\x59"));
        // do_wrapped_text(
        //     ctx,
        //     value("\xe7\x8e\x8b\xe6\x98\x8e\xef\xbc\x9a\xe8\xbf\x99\xe6\x98"
        //           "\xaf\xe4\xbb\x80\xe4\xb9\x88\xef\xbc\x9f"));

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

        do_section_divider(ctx);

        {
            do_heading(ctx, "Text");

            {
                for (int outer = 0; outer != 1; ++outer)
                {
                    do_wrapped_text(
                        ctx,
                        value(
                            "Lorem ipsum dolor sit amet, consectetur "
                            "adipisg elit. "
                            "Phasellus lacinia elementum diam consequat "
                            "alicinquet. "
                            "Vestibulum ut libero justo. Pellentesque lectus "
                            "lectus, "
                            "scelerisque a elementum sed, bibendum id libero. "
                            "Maecenas venenatis est sed sem "
                            "consequat mollis. Ut "
                            "nequeodio, hendrerit ut justo venenatis, "
                            "consequat "
                            "molestie eros. Nam fermentum, mi malesuada "
                            "eleifend"
                            "dapibus, lectus dolor luctus orci, nec posuere "
                            "lor "
                            "lorem ac sem. Nullam interdum laoreet ipsum in "
                            "dictum."),
                        FILL);
                }
            }

            do_wrapped_text(
                ctx, value("Dies ist ein Beispieltext für Testzwecke."), FILL);

            do_wrapped_text(
                ctx, value("这是一些用于测试目的的示例国际文本"), FILL);
        }

        do_section_divider(ctx);

        {
            do_heading(ctx, "Color Palette");

            show_color_ramp(ctx, the_palette.primary);
            do_spacer(ctx, size(10, 10, PIXELS));
            show_color_ramp(ctx, the_palette.secondary);
            do_spacer(ctx, size(10, 10, PIXELS));
            show_color_ramp(ctx, the_palette.tertiary);
            do_spacer(ctx, size(10, 10, PIXELS));
            show_color_ramp(ctx, the_palette.neutral);
            do_spacer(ctx, size(10, 10, PIXELS));
            show_color_ramp(ctx, the_palette.warning);
            do_spacer(ctx, size(10, 10, PIXELS));
            show_color_ramp(ctx, the_palette.danger);
        }

        do_section_divider(ctx);

        {
            do_heading(ctx, "Color Swatches");

            show_color_swatch(ctx, theme.primary);
            show_color_swatch(ctx, theme.secondary);
            show_color_swatch(ctx, theme.tertiary);
            show_color_swatch(ctx, theme.background);
            show_color_swatch(ctx, theme.structural);
            show_color_swatch(ctx, theme.foreground);
            show_color_swatch(ctx, theme.accent);
            show_color_swatch(ctx, theme.warning);
            show_color_swatch(ctx, theme.danger);
        }

        // {
        //     scoped_flow_layout flow(ctx, UNPADDED);

        //     auto my_style = text_style{
        //         "roboto/Roboto-Regular", 22.f, rgb8(173, 181, 189)};

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

        do_section_divider(ctx);

        {
            do_heading(ctx, "Lazy Lists");

            lazy_list_ui(ctx, 262'144, [&](ui_context ctx, size_t index) {
                binary_number_ui(ctx, /*grid,*/ int(index));
            });
        }
    }

    {
        panel_style_info style{
            .margin = box_border_width<float>{0, 0, 0, 0},
            .border_width = box_border_width<float>{0, 0, 0, 0},
            .padding = box_border_width<float>{8, 8, 8, 8},
            .background_color = lerp(
                get_system(ctx).theme.background.stronger[0].main,
                get_system(ctx).theme.background.base.main,
                0.5)};
        panel p(ctx, direct(style), FILL | UNPADDED);

        // TODO: Use bordered layout.
        column_layout column1(ctx, FILL | PADDED);
        column_layout column2(ctx, FILL | PADDED);
        column_layout column3(ctx, FILL | PADDED);

        row_layout row(ctx);

        {
            {
                do_switch(
                    ctx,
                    add_write_action(direct(light_theme), callback([&](bool) {
                                         theme_update_needed = true;
                                     })));
                do_text(ctx, value("Light"));
            }

            do_spacer(ctx, width(10, PIXELS));
            do_separator(ctx);
            do_spacer(ctx, width(10, PIXELS));

            {
                {
                    do_radio_button(
                        ctx,
                        add_write_action(
                            make_radio_signal(
                                direct(seeds), value(&seed_sets[0])),
                            callback([&](bool) {
                                theme_update_needed = true;
                            })),
                        "One");
                }

                do_spacer(ctx, width(10, PIXELS));
                do_separator(ctx);
                do_spacer(ctx, width(10, PIXELS));

                {
                    do_radio_button(
                        ctx,
                        add_write_action(
                            make_radio_signal(
                                direct(seeds), value(&seed_sets[1])),
                            callback([&](bool) {
                                theme_update_needed = true;
                            })),
                        "Two");
                }

                do_spacer(ctx, width(10, PIXELS));
                do_separator(ctx);
                do_spacer(ctx, width(10, PIXELS));

                {
                    do_radio_button(
                        ctx,
                        add_write_action(
                            make_radio_signal(
                                direct(seeds), value(&seed_sets[2])),
                            callback([&](bool) {
                                theme_update_needed = true;
                            })),
                        "Three");
                }

                do_spacer(ctx, width(10, PIXELS));
                do_separator(ctx);
                do_spacer(ctx, width(10, PIXELS));

                {
                    do_radio_button(
                        ctx,
                        add_write_action(
                            make_radio_signal(
                                direct(seeds), value(&seed_sets[3])),
                            callback([&](bool) {
                                theme_update_needed = true;
                            })),
                        "Four");
                }
            }
        }
    }
}
