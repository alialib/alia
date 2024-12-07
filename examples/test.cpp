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
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/library/bullets.hpp>
#include <alia/ui/library/checkbox.hpp>
#include <alia/ui/library/collapsible.hpp>
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

enum class color_theme
{
    VIOLET,
    BLUE,
    INDIGO,
    PINK
};

color_theme selected_theme = color_theme::BLUE;

bool light_theme = false;

theme_colors* active_theme = nullptr;

void
update_theme(ui_context ctx)
{
    theme_colors* new_theme = nullptr;
    switch (selected_theme)
    {
        case color_theme::VIOLET:
            if (light_theme)
                new_theme = &violet_light_theme;
            else
                new_theme = &violet_dark_theme;
            break;
        case color_theme::BLUE:
            if (light_theme)
                new_theme = &blue_light_theme;
            else
                new_theme = &blue_dark_theme;
            break;
        case color_theme::INDIGO:
            if (light_theme)
                new_theme = &indigo_light_theme;
            else
                new_theme = &indigo_dark_theme;
            break;
        case color_theme::PINK:
            if (light_theme)
                new_theme = &pink_light_theme;
            else
                new_theme = &pink_dark_theme;
            break;
    }
    if (new_theme && new_theme != active_theme)
    {
        active_theme = new_theme;
        get_system(ctx).theme = *new_theme;
    }
}

void
my_ui(ui_context ctx)
{
    update_theme(ctx);

    auto my_style = style_info{
        font_info{&get_font("Roboto/Roboto-Regular", 22.f)},
        get_system(ctx).theme.on_surface};
    scoped_style_info scoped_style(ctx, my_style);

    scoped_scrollable_view scrollable(ctx, GROW); //, 3, 2);

    // scoped_panel panel(ctx, GROW | UNPADDED);

    column_layout column(ctx, GROW | PADDED);
    column_layout column2(ctx, GROW | PADDED);
    column_layout column3(ctx, GROW | PADDED);

    // auto my_style = text_style{
    //     "Roboto_Mono/static/RobotoMono-Regular", 22.f, rgb8(173, 181, 189)};

    // panel_style_info pstyle{
    //     box_border_width<float>{4, 4, 4, 4},
    //     box_border_width<float>{0, 0, 0, 0},
    //     box_border_width<float>{4, 4, 4, 4}};

    // {
    //     bulleted_list list(ctx);
    //     {
    //         bulleted_item item(list);
    //         do_text(ctx, value("this"));
    //     }
    //     {
    //         bulleted_item item(list);
    //         do_text(ctx, value("that"));
    //     }
    //     {
    //         bulleted_item item(list);
    //         do_text(ctx, value("the other"));
    //     }
    // }

    do_spacer(ctx, height(40, PIXELS));
    do_separator(ctx);
    do_spacer(ctx, height(40, PIXELS));

    do_text(ctx, value("Theme Options"));
    do_spacer(ctx, height(20, PIXELS));

    {
        grid_layout grid(ctx);

        {
            grid_row row(grid);
            do_switch(ctx, direct(light_theme));
            do_text(ctx, value("Light"));
        }

        do_spacer(ctx, height(10, PIXELS));

        {
            {
                grid_row row(grid);
                do_radio_button(
                    ctx,
                    make_radio_signal(
                        direct(selected_theme), value(color_theme::VIOLET)));
                do_text(ctx, value("Violet"));
            }
            {
                grid_row row(grid);
                do_radio_button(
                    ctx,
                    make_radio_signal(
                        direct(selected_theme), value(color_theme::BLUE)));
                do_text(ctx, value("Blue"));
            }
            {
                grid_row row(grid);
                do_radio_button(
                    ctx,
                    make_radio_signal(
                        direct(selected_theme), value(color_theme::PINK)));
                do_text(ctx, value("Pink"));
            }
            {
                grid_row row(grid);
                do_radio_button(
                    ctx,
                    make_radio_signal(
                        direct(selected_theme), value(color_theme::INDIGO)));
                do_text(ctx, value("Indigo"));
            }
        }
    }

    do_spacer(ctx, height(40, PIXELS));
    do_separator(ctx);
    do_spacer(ctx, height(40, PIXELS));

    do_text(ctx, value("Some Widgets"));
    do_spacer(ctx, height(20, PIXELS));

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
                do_node_expander(ctx, disable_writes(state1));
                do_text(ctx, value("One"));
            }
            {
                row_layout row(ctx);
                do_node_expander(ctx, state1);
                do_text(ctx, value("Two"));
            }
            {
                row_layout row(ctx);
                do_node_expander(ctx, get_state(ctx, false));
                do_text(ctx, value("Three"));
            }
        }

        do_spacer(ctx, size(100, 100, PIXELS));

        {
            column_layout col(ctx);
            auto state1 = get_state(ctx, false);
            {
                row_layout row(ctx);
                do_checkbox(ctx, disable_writes(state1));
                do_text(ctx, value("One"));
            }
            {
                row_layout row(ctx);
                do_checkbox(ctx, state1);
                do_text(ctx, value("Two"));
            }
            {
                row_layout row(ctx);
                do_checkbox(ctx, get_state(ctx, false));
                do_text(ctx, value("Three"));
            }
        }

        do_spacer(ctx, size(100, 100, PIXELS));

        {
            column_layout col(ctx);
            auto selected = get_state(ctx, int(0));
            {
                row_layout row(ctx);
                do_radio_button(
                    ctx,
                    disable_writes(make_radio_signal(selected, value(2))));
                do_text(ctx, value("Option One"));
            }
            {
                // TODO: Implement radio buttons with clickable text.
                row_layout row(ctx);
                auto id = get_widget_id(ctx);
                do_box_region(ctx, id, row.region());
                do_radio_button(
                    ctx,
                    make_radio_signal(selected, value(2)),
                    BASELINE_Y,
                    id);
                do_text(ctx, value("Option Two"));
            }
            {
                row_layout row(ctx);
                do_radio_button(ctx, make_radio_signal(selected, value(3)));
                do_text(ctx, value("Option Three"));
            }
        }
    }

    do_spacer(ctx, height(20, PIXELS));

    {
        row_layout row(ctx);
        auto slider_value = get_state(ctx, value(1.5));
        do_slider(ctx, slider_value, 0, 5, 0.01);
        do_text(ctx, alia::printf(ctx, "%.2f", slider_value));
    }

    do_spacer(ctx, height(40, PIXELS));
    do_separator(ctx);
    do_spacer(ctx, height(40, PIXELS));

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
    //     cached_ui_block<column_layout>(ctx, unit_id, default_layout, [&] {
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

    {
        auto show_content = get_state(ctx, false);
        // grid_layout grid(ctx);

        {
            row_layout row(ctx);
            do_node_expander(ctx, show_content);
            do_text(ctx, value("Some Text"), CENTER_Y);
        }

        {
            row_layout row(ctx);
            do_spacer(ctx, width(40, PIXELS));
            collapsible_content(ctx, show_content, GROW, [&] {
                do_wrapped_text(
                    ctx,
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
                          "dictum."));
            });
        }
    }

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

    do_spacer(ctx, height(100, PIXELS));

    // do_box(
    //     ctx,
    //     SK_ColorRED,
    //     actions::toggle(get_state(ctx, false)),
    //     size(400, 400, PIXELS));
    // do_spacer(ctx, height(100, PIXELS));

    {
        for (int outer = 0; outer != 0; ++outer)
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
