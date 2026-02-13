// #define WIN32_LEAN_AND_MEAN
// #include <windows.h>

#include <algorithm>
#include <chrono>
#include <functional> // For std::hash
#include <iomanip>
#include <iostream>
#include <random>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "roboto-msdf.h"

#include <alia/renderers/gl/renderer.hpp>

#include <alia/abi/base/arena.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/events.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/layout/system.h>
#include <alia/abi/ui/layout/utilities.h>
#include <alia/abi/ui/style.h>
#include <alia/color.hpp>
#include <alia/context.hpp>
#include <alia/events.hpp>
#include <alia/flow/dispatch.hpp>
#include <alia/impl/ui/layout.hpp>
#include <alia/input/elements.hpp>
#include <alia/input/pointer.hpp>
#include <alia/input/regions.hpp>
#include <alia/platforms/glfw/window.hpp>
#include <alia/system/api.hpp>
#include <alia/system/input_processing.hpp>
#include <alia/system/interface.hpp>
#include <alia/system/object.hpp>
#include <alia/text_engines/msdf/msdf.hpp>
#include <alia/theme.hpp>
#include <alia/ui/drawing.h>
#include <alia/ui/layout/components.hpp>
#include <alia/ui/layout/flags.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

using namespace alia;
using namespace alia::operators;

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
};

alia::ui_system the_system;
GLFWwindow* the_window;
gl_renderer the_renderer;
alia_draw_system the_draw_system;
alia_arena the_display_list_arena;
msdf_text_engine* the_msdf_text_engine;
alia_style the_style = {.padding = 10.0f};
theme_colors the_theme;
seed_colors const* seeds = &seed_sets[0];
color_palette the_palette;
float the_time = 0.0f;
static uintptr_t the_element_counter = 0;

bool
detect_click(context& ctx, float x, float y, float width, float height)
{
    return get_event_type(ctx) == ALIA_EVENT_MOUSE_PRESS
        && as_mouse_press_event(ctx).x >= x
        && as_mouse_press_event(ctx).x <= x + width
        && as_mouse_press_event(ctx).y >= y
        && as_mouse_press_event(ctx).y <= y + height;
}

bool
do_rect(
    context& ctx,
    alia_z_index z_index,
    alia_element_id id,
    alia_vec2f size,
    alia_rgba color,
    layout_flag_set flags)
{
    switch (get_event_category(ctx))
    {
        case ALIA_CATEGORY_REFRESH: {
            alia_layout_leaf_emit(&ctx, size, raw_code(flags));
            break;
        }
        case ALIA_CATEGORY_SPATIAL: {
            alia_box box = alia_layout_leaf_read(&ctx);
            do_box_region(ctx, id, box);
            break;
        }
        case ALIA_CATEGORY_DRAWING: {
            alia_box box = alia_layout_leaf_read(&ctx);
            auto status = get_interaction_status(ctx, id);
            if (status & ELEMENT_HOVERED)
            {
                color = {0.0f, 0.0f, 1.0f, 1.0f};
            }
            else if (status & ELEMENT_ACTIVE)
            {
                color = {1.0f, 0.0f, 1.0f, 1.0f};
            }
            alia_draw_box(
                as_draw_event(ctx).context, z_index, box, color, 10.0f);
            break;
        }
        case ALIA_CATEGORY_INPUT: {
            alia_box box = alia_layout_leaf_read(&ctx);
            if (detect_click(ctx, id, ALIA_BUTTON_LEFT))
                return true;
            break;
        }
    }
    return false;
}

bool
do_rect_with_offset(
    context& ctx,
    alia_z_index z_index,
    alia_vec2f size,
    alia_rgba color,
    layout_flag_set flags,
    alia_vec2f offset)
{
    switch (get_event_category(ctx))
    {
        case ALIA_CATEGORY_REFRESH: {
            alia_layout_leaf_emit(&ctx, size, raw_code(flags));
            break;
        }
        case ALIA_CATEGORY_SPATIAL: {
            alia_box box = alia_layout_leaf_read(&ctx);
            // box_region(ctx, id, box);
            break;
        }
        case ALIA_CATEGORY_DRAWING: {
            alia_box box = alia_layout_leaf_read(&ctx);
            alia_draw_box(
                as_draw_event(ctx).context, z_index, box, color, 0.0f);
            break;
        }
        case ALIA_CATEGORY_INPUT: {
            alia_box box = alia_layout_leaf_read(&ctx);
            if (detect_click(
                    ctx, box.min.x, box.min.y, box.size.x, box.size.y))
                return true;
            break;
        }
    }
    return false;
}

template<class T>
struct is_layout_modifier : std::false_type
{
};

template<class T>
concept LayoutModifier = is_layout_modifier<T>::value;

template<class T>
concept LayoutModifierPack
    = requires(T t) { typename std::remove_cvref_t<T>::values; };

template<class... Ts>
struct layout_mods_t
{
    std::tuple<Ts...> values;
};

// Modifier | Modifier
template<LayoutModifier A, LayoutModifier B>
constexpr auto
operator|(A a, B b)
{
    return layout_mods_t<A, B>{std::tuple{a, b}};
}

// Modifier | Pack
template<LayoutModifier A, class... Bs>
constexpr auto
operator|(A a, layout_mods_t<Bs...> b)
{
    return layout_mods_t<A, Bs...>{std::tuple_cat(std::tuple{a}, b.values)};
}

// Pack | Modifier
template<class... As, LayoutModifier B>
constexpr auto
operator|(layout_mods_t<As...> a, B b)
{
    return layout_mods_t<As..., B>{std::tuple_cat(a.values, std::tuple{b})};
}

// Pack | Pack
template<class... As, class... Bs>
constexpr auto
operator|(layout_mods_t<As...> a, layout_mods_t<Bs...> b)
{
    return layout_mods_t<As..., Bs...>{std::tuple_cat(a.values, b.values)};
}

template<class Context, class Content>
void
apply_mods(Context& ctx, layout_mods_t<>, Content&& content)
{
    std::forward<Content>(content)();
}

template<class Context, class Content, LayoutModifier Mod, class... Rest>
void
apply_mods(Context& ctx, layout_mods_t<Mod, Rest...> mods, Content&& content)
{
    apply_mod(ctx, std::get<Mod>(mods.values), [&] {
        apply_mods(
            ctx,
            layout_mods_t<Rest...>{
                std::tuple<Rest...>{std::get<Rest>(mods.values)...}},
            std::forward<Content>(content));
    });
}

struct align_right_t
{
};
template<>
struct is_layout_modifier<align_right_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, align_right_t, Content&& content)
{
    alignment_override(ctx, ALIGN_RIGHT, std::forward<Content>(content));
}

struct fill_x_t
{
};
template<>
struct is_layout_modifier<fill_x_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, fill_x_t, Content&& content)
{
    alignment_override(ctx, FILL_X, std::forward<Content>(content));
}

struct center_y_t
{
};
template<>
struct is_layout_modifier<center_y_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, center_y_t, Content&& content)
{
    alignment_override(ctx, CENTER_Y, std::forward<Content>(content));
}

struct min_width_t
{
    float width;
};
template<>
struct is_layout_modifier<min_width_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, min_width_t m, Content&& content)
{
    min_size_constraint(
        ctx, {.x = m.width, .y = 0}, std::forward<Content>(content));
}

struct min_height_t
{
    float height;
};
template<>
struct is_layout_modifier<min_height_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, min_height_t m, Content&& content)
{
    min_size_constraint(
        ctx, {.x = 0, .y = m.height}, std::forward<Content>(content));
}

struct min_size_t
{
    alia_vec2f size;
};
template<>
struct is_layout_modifier<min_size_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, min_size_t m, Content&& content)
{
    min_size_constraint(ctx, m.size, std::forward<Content>(content));
}

struct margins_t
{
    alia_insets insets;
};
template<>
struct is_layout_modifier<margins_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, margins_t m, Content&& content)
{
    inset(ctx, m.insets, std::forward<Content>(content));
}

constexpr align_right_t align_right;
constexpr fill_x_t fill_x;
constexpr center_y_t center_y;

constexpr min_width_t
min_width(float w)
{
    return {w};
}
constexpr min_height_t
min_height(float h)
{
    return {h};
}
constexpr min_size_t
min_size(alia_vec2f size)
{
    return {size};
}
constexpr margins_t
margins(alia_insets insets)
{
    return {insets};
}

template<class LayoutMods>
void
do_rect(
    context& ctx,
    alia_z_index z_index,
    alia_element_id id,
    alia_vec2f size,
    alia_rgba color,
    LayoutMods mods)
{
    apply_mods(
        ctx, mods, [&] { do_rect(ctx, z_index, id, size, color, FILL); });
}

template<class Content>
void
concrete_panel(
    context& ctx,
    alia_z_index z_index,
    alia_rgba color,
    layout_flag_set flags,
    Content&& content)
{
    placement_hook(ctx, flags, [&](auto const& placement) {
        if (get_event_type(ctx) == ALIA_EVENT_DRAW)
        {
            alia_draw_box(
                as_draw_event(ctx).context,
                z_index,
                placement.box,
                color,
                0.0f);
        }

        std::forward<Content>(content)();
    });
}

template<class Content, class LayoutMods>
void
panel(
    context& ctx,
    alia_z_index z_index,
    color color,
    LayoutMods mods,
    Content&& content)
{
    apply_mods(ctx, mods, [&] {
        concrete_panel(
            ctx, z_index, color, FILL, std::forward<Content>(content));
    });
}

template<class Content>
void
concrete_button(
    context& ctx,
    alia_z_index z_index,
    alia_rgba color,
    layout_flag_set flags,
    Content&& content)
{
    placement_hook(ctx, flags, [&](auto const& placement) {
        if (get_event_type(ctx) == ALIA_EVENT_DRAW)
        {
            alia_draw_box(
                as_draw_event(ctx).context, z_index, placement.box, color);
        }

        std::forward<Content>(content)();
    });
}

/// ----

struct msdf_text_layout_node
{
    alia_layout_node base;
    alia_layout_flags_t flags;
    float padding;
    msdf_text_engine* engine;
    char const* text;
    float font_size;
};

alia_horizontal_requirements
measure_text_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& text = *reinterpret_cast<msdf_text_layout_node*>(node);
    float width = measure_text_width(
        text.engine, text.text, strlen(text.text), text.font_size);
    return alia_horizontal_requirements{
        .min_size = width + text.padding * 2, .growth_factor = 0};
}

void
assign_text_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    // TODO: Implement
}

alia_vertical_requirements
measure_text_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& text = *reinterpret_cast<msdf_text_layout_node*>(node);
    auto const* metrics = get_msdf_font_metrics(text.engine);
    return alia_vertical_requirements{
        .min_size = metrics->line_height * text.font_size + text.padding * 2,
        .growth_factor = 0,
        .ascent = (text.flags & ALIA_Y_ALIGNMENT_MASK) == ALIA_BASELINE_Y
                    ? metrics->ascender * text.font_size + text.padding
                    : 0.0f,
        .descent = (text.flags & ALIA_Y_ALIGNMENT_MASK) == ALIA_BASELINE_Y
                     ? -metrics->descender * text.font_size + text.padding
                     : 0.0f};
}

struct text_layout_placement_header
{
    int fragment_count;
};

struct text_layout_placement_fragment
{
    alia_vec2f position;
    alia_vec2f size;
    char const* text;
    size_t length;
};

void
assign_text_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& text = *reinterpret_cast<msdf_text_layout_node*>(node);
    auto const* metrics = get_msdf_font_metrics(text.engine);

    // TODO: Don't repeatedly measure the text width.
    float width = measure_text_width(
        text.engine, text.text, strlen(text.text), text.font_size);

    auto const placement = alia_resolve_leaf_box(
        alia_fold_in_cross_axis_flags(text.flags, main_axis),
        box.size,
        baseline,
        alia_vec2f{width, metrics->line_height * text.font_size},
        metrics->ascender * text.font_size,
        {text.padding, text.padding});

    text_layout_placement_header* header
        = arena_alloc<text_layout_placement_header>(ctx->arena);
    header->fragment_count = 1;

    text_layout_placement_fragment* fragment
        = arena_alloc<text_layout_placement_fragment>(ctx->arena);
    fragment->position = box.min + placement.min;
    fragment->size = placement.size;
    fragment->text = text.text;
    fragment->length = strlen(text.text);
}

alia_horizontal_requirements
measure_text_wrapped_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    return alia_horizontal_requirements{0, 0};
}

alia_wrapping_requirements
measure_text_wrapped_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float current_x_offset,
    float line_width)
{
    auto& text = *reinterpret_cast<msdf_text_layout_node*>(node);
    auto const* metrics = get_msdf_font_metrics(text.engine);

    size_t length = strlen(text.text);

    alia_wrapping_requirements requirements;

    auto first_break = break_text(
        text.engine,
        text.text,
        0,
        length,
        length,
        text.font_size,
        line_width - current_x_offset - text.padding * 2,
        current_x_offset == 0);

    // If there is content on the first line, then we need to assign the
    // vertical requirements for it.
    if (first_break.first != 0)
    {
        requirements.first_line = {
            .height = metrics->line_height * text.font_size + text.padding * 2,
            .ascent = metrics->ascender * text.font_size + text.padding,
            .descent = -metrics->descender * text.font_size + text.padding};
    }
    else
    {
        requirements.first_line = {.height = 0, .ascent = 0, .descent = 0};
    }

    // If everything fits on the first line, then we're done.
    if (first_break.first == length)
    {
        requirements.interior_height = 0;
        requirements.last_line = {.height = 0, .ascent = 0, .descent = 0};
        requirements.end_x
            = current_x_offset + first_break.second + text.padding * 2;
        return requirements;
    }

    // Otherwise, wrap the rest of the text...

    int wrap_count = 0;
    size_t index = first_break.first;
    float new_x = 0;
    while (index < length)
    {
        ++wrap_count;
        auto break_result = break_text(
            text.engine,
            text.text,
            index,
            length,
            length,
            text.font_size,
            line_width - text.padding * 2,
            true);
        index = break_result.first;
        new_x = break_result.second;
    }

    requirements.interior_height
        = (wrap_count - 1)
        * (metrics->line_height * text.font_size + text.padding * 2);
    requirements.last_line
        = {.height = metrics->line_height * text.font_size + text.padding * 2,
           .ascent = metrics->ascender * text.font_size + text.padding,
           .descent = -metrics->descender * text.font_size + text.padding};
    requirements.end_x = new_x + text.padding * 2;

    return requirements;
}

void
assign_text_wrapped_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_wrapping_assignment const* assignment)
{
    auto& text = *reinterpret_cast<msdf_text_layout_node*>(node);
    auto const* metrics = get_msdf_font_metrics(text.engine);

    size_t length = strlen(text.text);

    text_layout_placement_header* header
        = arena_alloc<text_layout_placement_header>(ctx->arena);
    header->fragment_count = 0;

    // TODO: This all feels a bit hacky, but it works for now, and it feels
    // like there is more significant restructuring to come anyway.

    float x = assignment->first_line_x_offset;
    float y = assignment->y_base + assignment->first_line.baseline_offset;
    float next_y = assignment->y_base + assignment->first_line.line_height
                 + metrics->ascender * text.font_size + text.padding;

    size_t index = 0;
    while (index < length)
    {
        auto break_result = break_text(
            text.engine,
            text.text,
            index,
            length,
            length,
            text.font_size,
            assignment->line_width - x - text.padding * 2,
            x == 0);
        size_t const end_index = break_result.first;

        if (end_index == length)
        {
            y += assignment->last_line.baseline_offset
               - (metrics->ascender * text.font_size + text.padding);
        }

        text_layout_placement_fragment* fragment
            = arena_alloc<text_layout_placement_fragment>(ctx->arena);
        fragment->position
            = {x + assignment->x_base + text.padding,
               y - metrics->ascender * text.font_size};
        fragment->size
            = {assignment->line_width - x - text.padding * 2,
               metrics->line_height * text.font_size};
        fragment->text = text.text + index;
        fragment->length = end_index - index;
        ++header->fragment_count;

        x = 0;
        y = next_y;
        next_y += metrics->line_height * text.font_size + text.padding * 2;
        index = end_index;
    }
}

alia_layout_node_vtable text_layout_vtable
    = {measure_text_horizontal,
       assign_text_widths,
       measure_text_vertical,
       assign_text_boxes,
       measure_text_wrapped_horizontal,
       measure_text_wrapped_vertical,
       assign_text_wrapped_boxes};

template<class Content>
void
with_padding(context& ctx, float padding, Content&& content)
{
    float old_padding = ctx.style->padding;
    ctx.style->padding = padding;
    content();
    ctx.style->padding = old_padding;
}

bool
do_text(
    context& ctx,
    alia_z_index z_index,
    color color,
    float scale,
    char const* text,
    layout_flag_set flags = NO_FLAGS)
{
    bool result = false;
    switch (get_event_category(ctx))
    {
        case ALIA_CATEGORY_REFRESH: {
            auto& emission = ctx.layout->emission;
            msdf_text_layout_node* new_node
                = arena_alloc<msdf_text_layout_node>(emission.arena);
            new_node->base.vtable = &text_layout_vtable;
            new_node->base.next_sibling = nullptr;
            new_node->flags = raw_code(flags);
            new_node->text = text;
            new_node->font_size = scale;
            new_node->engine = the_msdf_text_engine;
            new_node->padding = ctx.style->padding;
            *emission.next_ptr = &new_node->base;
            emission.next_ptr = &new_node->base.next_sibling;
            break;
        }
        case ALIA_CATEGORY_SPATIAL: {
            auto& text_placement = *arena_alloc<text_layout_placement_header>(
                *alia_layout_placement_arena(&ctx));
            for (int i = 0; i < text_placement.fragment_count; ++i)
            {
                auto& fragment = *arena_alloc<text_layout_placement_fragment>(
                    *alia_layout_placement_arena(&ctx));
                // box_region(ctx, id, box);
            }
            break;
        }
        case ALIA_CATEGORY_DRAWING: {
            auto& text_placement = *arena_alloc<text_layout_placement_header>(
                *alia_layout_placement_arena(&ctx));
            for (int i = 0; i < text_placement.fragment_count; ++i)
            {
                auto& fragment = *arena_alloc<text_layout_placement_fragment>(
                    *alia_layout_placement_arena(&ctx));
                draw_text(
                    the_msdf_text_engine,
                    as_draw_event(ctx).context,
                    z_index,
                    fragment.text,
                    fragment.length,
                    scale,
                    fragment.position,
                    color);
            }
            break;
        }
        case ALIA_CATEGORY_INPUT: {
            auto& text_placement = *arena_alloc<text_layout_placement_header>(
                *alia_layout_placement_arena(&ctx));
            for (int i = 0; i < text_placement.fragment_count; ++i)
            {
                auto& fragment = *arena_alloc<text_layout_placement_fragment>(
                    *alia_layout_placement_arena(&ctx));
                alia_box box
                    = {.min = fragment.position, .size = fragment.size};
                if (detect_click(
                        ctx, box.min.x, box.min.y, box.size.x, box.size.y))
                {
                    // TODO: Perform action, abort traversal.
                    result = true;
                }
            }
            break;
        }
    }
    return result;
}

char const* lorem_ipsum
    = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin sed "
      "dictum massa. Maecenas et euismod lorem, ut dapibus eros. Nam maximus, "
      "purus vitae mollis ornare, tortor justo posuere neque, at lacinia ante "
      "metus eget diam. Aenean sit amet posuere metus. In hac habitasse "
      "platea dictumst. Nam sed turpis ultricies tellus auctor egestas. Ut "
      "laoreet nisi nisi, id posuere tortor tincidunt a. Pellentesque "
      "placerat vulputate massa at semper. Fusce malesuada porttitor enim "
      "dignissim viverra. In aliquam, odio nec sagittis elementum, elit enim "
      "auctor turpis, sit amet volutpat enim massa ac orci. Maecenas iaculis, "
      "ex at pulvinar volutpat, ligula nulla pellentesque tellus, vel aliquam "
      "nunc dolor eu risus.";

alia_element_id
make_fake_element_id(context& ctx)
{
    return reinterpret_cast<alia_element_id>(the_element_counter++);
}

void
rectangle_demo(context& ctx)
{
    static bool invert = false;

    with_padding(ctx, 0, [&] {
        column(ctx, [&]() {
            flow(ctx, [&]() {
                float x = 0.0f;
                for (int i = 0; i < 10; ++i)
                {
                    flow(ctx, [&]() {
                        alia_z_index const rect_z_index = 0;
                        alia_z_index const text_z_index = 1;

                        for (int j = 0; j < 500; ++j)
                        {
                            float f = fmod(x, 1.0f);
                            if (do_rect(
                                    ctx,
                                    rect_z_index,
                                    make_fake_element_id(ctx),
                                    {24, 24},
                                    invert ? color{f, 0.1f, 1.0f - f, 1}
                                           : color{1.0f - f, 0.1f, f, 1},
                                    ALIGN_TOP | ALIGN_LEFT))
                            {
                                invert = !invert;
                                return;
                            }
                            x += 0.0015f;
                        }

                        with_padding(ctx, 20, [&] {
                            for (int j = 0; j < 1; ++j)
                            {
                                do_text(
                                    ctx,
                                    text_z_index,
                                    GRAY,
                                    24 + i * 6 + j * 4,
                                    "lorem ipsum");
                                if (do_text(
                                        ctx,
                                        text_z_index,
                                        GRAY,
                                        20 + i * 12 + j * 4,
                                        lorem_ipsum))
                                {
                                    invert = !invert;
                                    return;
                                }
                            }
                        });
                    });
                }
            });
        });
    });
}

void
text_demo(context& ctx)
{
    static bool invert = false;

    inset(ctx, {.left = 10, .right = 10, .top = 10, .bottom = 10}, [&]() {
        column(ctx, [&]() {
            for (int i = 0; i < 10; ++i)
            {
                with_padding(ctx, 8, [&] {
                    row(ctx, [&]() {
                        do_text(ctx, 1, GRAY, 40, "test");
                        flow(ctx, GROW, [&]() {
                            for (int j = 0; j < 10; ++j)
                            {
                                flow(ctx, [&]() {
                                    do_text(
                                        ctx, 1, GRAY, 10 + i * 6, lorem_ipsum);
                                });
                                do_text(ctx, 1, GRAY, 16 + i * 4, "lorum");
                                do_text(ctx, 1, GRAY, 20 + i * 2, "ipsum");
                            }
                        });
                    });
                });
            }
        });
    });
}

void
simple_text_demo(context& ctx)
{
    static bool invert = false;

    inset(ctx, {.left = 10, .right = 10, .top = 10, .bottom = 10}, [&]() {
        column(ctx, [&]() {
            for (int i = 0; i < 12; ++i)
            {
                do_text(
                    ctx,
                    1,
                    GRAY,
                    20 + (10 - i) * 4,
                    " !\"#$%&'()*+,-./0123456789:;<=>?@AZaz[]^_`{|}~");
            }
        });
    });
}

void
nested_flow_demo(context& ctx)
{
    static bool invert = false;

    inset(ctx, {.left = 100, .right = 100, .top = 100, .bottom = 100}, [&]() {
        flow(ctx, [&]() {
            for (int i = 0; i < 10; ++i)
            {
                flow(ctx, [&]() {
                    do_text(ctx, 1, GRAY, 10 + i * 6, lorem_ipsum);
                });
                do_text(ctx, 1, GRAY, 16 + i * 4, "lorum");
                do_text(ctx, 1, GRAY, 20 + i * 2, "ipsum");
            }
        });
    });
}

void
mixed_flow_demo(context& ctx)
{
    with_padding(ctx, 10, [&] {
        flow(ctx, [&]() {
            for (int i = 0; i < 10; ++i)
            {
                float x = 0.0f;
                for (int j = 0; j < 10; ++j)
                {
                    float f = fmod(x, 1.0f);
                    do_rect(
                        ctx,
                        0,
                        make_fake_element_id(ctx),
                        {72, 72},
                        color{f, 0.1f, 1.0f - f, 1},
                        CENTER);
                    x += 0.1f;
                }

                panel(
                    ctx,
                    1,
                    color{0.05f, 0.05f, 0.06f, 1},
                    min_size({0, 0})
                        | margins(
                            {.left = 10,
                             .right = 10,
                             .top = 10,
                             .bottom = 10}),
                    [&] {
                        do_text(ctx, 2, GRAY, 12 + i * 6, "panel", BASELINE_Y);
                    });
                do_text(ctx, 1, GRAY, 12 + i * 4, lorem_ipsum, BASELINE_Y);
            }
        });
    });
}

void
layout_demo_flow(context& ctx)
{
    float x = 0.0f;
    hyperflow(ctx, [&]() {
        for (int i = 0; i < 600; ++i)
        {
            float intensity = ((i / 4) % 3) * 0.01f;
            inset(
                ctx,
                {.left = 10, .right = 10, .top = 10, .bottom = 10},
                [&]() {
                    concrete_panel(
                        ctx,
                        0,
                        color{
                            0.03f + intensity,
                            0.03f + intensity,
                            0.04f + intensity,
                            1},
                        NO_FLAGS,
                        [&]() {
                            min_size_constraint(ctx, {0, 200}, [&]() {
                                row(ctx, [&]() {
                                    float f = fmod(x, 1.0f);
                                    do_rect(
                                        ctx,
                                        1,
                                        make_fake_element_id(ctx),
                                        {24, float((i & 7) * 12 + 12)},
                                        color{f, 0.1f, 1.0f - f, 1},
                                        layout_flag_set(
                                            (i & 3)
                                            << ALIA_CROSS_ALIGNMENT_BIT_OFFSET));
                                    x += 0.01f;
                                });
                            });
                        });
                });
        }
    });
}

void
layout_growth_demo(context& ctx)
{
    float x = 0.0f;
    row(ctx, [&]() {
        for (int i = 0; i < 12; ++i)
        {
            float f = fmod(x, 1.0f);
            growth_override(ctx, i * 1.0f, [&]() {
                do_rect(
                    ctx,
                    0,
                    make_fake_element_id(ctx),
                    {6, 12},
                    color{f, 0.1f, 1.0f - f, 1},
                    FILL | (i & 1 ? GROW : NO_FLAGS));
            });
            x += 0.08f;
        }
    });
}

void
do_animated_rect(
    context& ctx,
    alia_z_index z_index,
    bool& initialized,
    alia_vec2f& offset,
    alia_vec2f size,
    alia_rgba color,
    layout_flag_set flags)
{
    placement_hook(ctx, FILL, [&](auto outer_placement) {
        alignment_override(ctx, flags, [&]() {
            placement_hook(ctx, FILL, [&](auto inner_placement) {
                alia_vec2f inner_pos
                    = inner_placement.box.min - outer_placement.box.min;
                if (get_event_type(ctx) == ALIA_EVENT_DRAW)
                {
                    if (!initialized)
                    {
                        offset = inner_pos;
                        initialized = true;
                    }
                    offset += (inner_pos - offset) * 0.1f;
                }
                do_rect_with_offset(
                    ctx, z_index, size, color, FILL, offset - inner_pos);
            });
        });
    });
}

void
alignment_override_demo(context& ctx)
{
    static bool invert = false;
    static bool initialized[12] = {false};
    static alia_vec2f offsets[12] = {0};
    float x = 0.0f;
    row(ctx, [&]() {
        concrete_panel(ctx, 0, color{0.03f, 0.03f, 0.04f, 1}, CENTER, [&]() {
            inset(ctx, {.left = 4, .right = 4, .top = 4, .bottom = 4}, [&]() {
                if (do_rect(
                        ctx,
                        1,
                        make_fake_element_id(ctx),
                        {24, 24},
                        invert ? color{1, 1, 1, 1} : color{0, 0, 0, 0},
                        CENTER))
                {
                    invert = !invert;
                }
            });
        });
        for (int i = 0; i < 12; ++i)
        {
            float f = fmod(x, 1.0f);
            inset(
                ctx,
                {.left = 10, .right = 10, .top = 10, .bottom = 10},
                [&]() {
                    min_size_constraint(ctx, {0, 200}, [&]() {
                        concrete_panel(
                            ctx,
                            0,
                            color{
                                0.03f + 0.02f * f,
                                0.03f + 0.02f * f,
                                0.04f + 0.02f * f,
                                1},
                            NO_FLAGS,
                            [&]() {
                                do_animated_rect(
                                    ctx,
                                    1,
                                    initialized[i],
                                    offsets[i],
                                    {24, 24},
                                    color{f, 0.1f, 1.0f - f, 1},
                                    (i & 1) == (invert ? 0 : 1)
                                        ? ALIGN_TOP
                                        : ALIGN_BOTTOM);
                            });
                    });
                });
            x += 0.08f;
        }
    });
}

void
layout_mods_demo(context& ctx)
{
    float x = 0.0f;
    flow(ctx, [&]() {
        for (int i = 0; i < 36; ++i)
        {
            float f = fmod(x, 1.0f);
            panel(
                ctx,
                0,
                color{
                    0.03f + 0.02f * f,
                    0.03f + 0.02f * f,
                    0.04f + 0.02f * f,
                    1},
                min_size({120, 120})
                    | margins(
                        {.left = 10, .right = 10, .top = 10, .bottom = 10}),
                [&]() {
                    do_rect(
                        ctx,
                        1,
                        make_fake_element_id(ctx),
                        {float(4 * i), float(4 * i)},
                        color{f, 0.1f, 1.0f - f, 1},
                        align_right | center_y);
                });
            x += 0.2f;
        }
    });
}

void
grid_demo(context& ctx)
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    grid(ctx, [&](auto* grid) {
        float x = 0.0f;
        for (int i = 0; i < 40; ++i)
        {
            grid_row(ctx, grid, [&]() {
                for (int j = 0; j < 40; ++j)
                {
                    float const f = fmod(x, 1.0f);
                    float const size = std::pow(dist(rng), 12.0f) * 80 + 20;
                    do_rect(
                        ctx,
                        0,
                        make_fake_element_id(ctx),
                        {size, size},
                        color{f, 0.1f, 1.0f - f, 1},
                        CENTER);
                    x += 0.2f;
                }
            });
        }
    });
}

void
layout_demo(context& ctx)
{
    with_padding(ctx, 10, [&] {
        inset(ctx, {.left = 40, .right = 40, .top = 40, .bottom = 40}, [&]() {
            row(ctx, [&]() {
                float x = 0.0f;
                inset(
                    ctx,
                    {.left = 0, .right = 12, .top = 0, .bottom = 0},
                    [&]() {
                        column(ctx, [&]() {
                            for (int j = 0; j < 80; ++j)
                            {
                                float f = fmod(x, 1.0f);
                                do_rect(
                                    ctx,
                                    0,
                                    make_fake_element_id(ctx),
                                    {float((j & 7) * 12 + 12), 24},
                                    color{f, 0.1f, 1.0f - f, 1},
                                    layout_flag_set(
                                        (j & 3)
                                        << ALIA_X_ALIGNMENT_BIT_OFFSET));
                                x += 0.02f;
                            }
                        });
                    });
                column(ctx, GROW, [&]() {
                    do_text(ctx, 1, GRAY, 24, "layout_growth_demo");
                    layout_growth_demo(ctx);
                    do_text(ctx, 1, GRAY, 24, "alignment_override_demo");
                    alignment_override_demo(ctx);
                    do_text(ctx, 1, GRAY, 24, "layout_mods_demo");
                    layout_mods_demo(ctx);
                    do_text(ctx, 1, GRAY, 24, "mixed_flow_demo");
                    mixed_flow_demo(ctx);
                    do_text(ctx, 1, GRAY, 24, "layout_demo_flow");
                    layout_demo_flow(ctx);
                });
            });
        });
    });
}

void
show_color_ramp(context& ctx, color_ramp ramp)
{
    row(ctx, [&]() {
        for (int i = 0; i != color_ramp_step_count; ++i)
        {
            do_rect(
                ctx,
                1,
                make_fake_element_id(ctx),
                {36, 36},
                alia_rgba_from_rgb_alpha(
                    alia_rgb_from_srgb8(ramp[i].rgb), 1.0f),
                CENTER);
        }
    });
}

void
show_contrasting_color_pair(context& ctx, contrasting_color_pair pair)
{
    concrete_panel(
        ctx,
        0,
        alia_rgba_from_rgb_alpha(alia_rgb_from_srgb8(pair.main), 1.0f),
        CENTER,
        [&]() {
            with_padding(ctx, 20, [&]() {
                do_rect(
                    ctx,
                    1,
                    make_fake_element_id(ctx),
                    {10, 10},
                    alia_rgba_from_rgb_alpha(
                        alia_rgb_from_srgb8(pair.contrasting), 1.0f),
                    CENTER);
            });
        });
}

void
show_color_swatch(context& ctx, color_swatch swatch)
{
    row(ctx, [&]() {
        show_contrasting_color_pair(ctx, swatch.weaker[0]);
        show_contrasting_color_pair(ctx, swatch.base);
        show_contrasting_color_pair(ctx, swatch.stronger[0]);
        show_contrasting_color_pair(ctx, swatch.stronger[1]);
    });
}

void
color_ramp(context& ctx, alia_srgb8 seed)
{
    column(ctx, [&]() {
        alia_oklch oklch = alia_oklch_from_oklab(
            alia_oklab_from_rgb(alia_rgb_from_srgb8(seed)));
        for (int i = 0; i < 11; ++i)
        {
            oklch.l = i * 0.1f;
            alia_rgb rgb = alia_rgb_from_oklab(alia_oklab_from_oklch(oklch));
            do_rect(
                ctx,
                1,
                make_fake_element_id(ctx),
                {36, 36},
                alia_rgba_from_rgb_alpha(rgb, 1.0f),
                CENTER);
        }
    });
}

void
color_transition(context& ctx, alia_oklch start, alia_oklch end)
{
    column(ctx, [&]() {
        alia_oklch oklch = start;
        for (int i = 0; i < 101; ++i)
        {
            oklch.l = start.l + (end.l - start.l) * i * 0.01f;
            oklch.c = start.c + (end.c - start.c) * i * 0.01f;
            oklch.h = start.h + (end.h - start.h) * i * 0.01f;
            alia_rgb rgb = alia_rgb_from_oklab(alia_oklab_from_oklch(oklch));
            do_rect(
                ctx,
                1,
                make_fake_element_id(ctx),
                {36, 36},
                alia_rgba_from_rgb_alpha(rgb, 1.0f),
                CENTER);
        }
    });
}

void
color_demo(context& ctx)
{
    hyperflow(ctx, [&]() {
        alia_oklch oklch = {.l = 0.7f, .c = 0.2f, .h = 0.0f};
        for (int i = 0; i < 101; ++i)
        {
            oklch.h = i * 0.01f * 2 * 3.14159f;
            alia_rgb rgb = alia_rgb_from_oklab(alia_oklab_from_oklch(oklch));
            with_padding(ctx, 5, [&] {
                do_rect(
                    ctx,
                    1,
                    make_fake_element_id(ctx),
                    {24, 24},
                    alia_rgba_from_rgb_alpha(rgb, 1.0f),
                    CENTER);
            });
        }
    });
    // row(ctx, [&]() {
    //     color_ramp(ctx, alia_srgb8{0x8B, 0x43, 0x67});
    //     color_ramp(ctx, alia_srgb8{0xff, 0x64, 0x64});
    // });
}

void
theme_demo(context& ctx)
{
    column(ctx, [&]() {
        show_color_ramp(ctx, the_palette.primary);
        show_color_ramp(ctx, the_palette.secondary);
        show_color_ramp(ctx, the_palette.tertiary);
        show_color_ramp(ctx, the_palette.neutral);
        show_color_ramp(ctx, the_palette.warning);
        show_color_ramp(ctx, the_palette.danger);

        show_color_swatch(ctx, the_theme.primary);
        show_color_swatch(ctx, the_theme.secondary);
        show_color_swatch(ctx, the_theme.tertiary);
        show_color_swatch(ctx, the_theme.background);
        show_color_swatch(ctx, the_theme.foreground);
        show_color_swatch(ctx, the_theme.structural);
        show_color_swatch(ctx, the_theme.accent);
        show_color_swatch(ctx, the_theme.warning);
        show_color_swatch(ctx, the_theme.danger);
    });
}

struct pass_aborted
{
};

void
abort_pass()
{
    throw pass_aborted();
}

void
the_demo(context& ctx)
{
    the_element_counter = 1;
    try
    {
        static int active_demo = 0;
        int const demo_count = 6;
        with_padding(ctx, 0, [&] {
            row(ctx, [&]() {
                column(ctx, [&]() {
                    for (int i = 0; i < demo_count; ++i)
                    {
                        if (do_rect(
                                ctx,
                                1,
                                make_fake_element_id(ctx),
                                {40, 40},
                                (active_demo == i) ? color{1, 1, 1, 1}
                                                   : color{0, 0, 0, 0},
                                FILL))
                        {
                            active_demo = i;
                            abort_pass();
                        }
                        do_rect(
                            ctx,
                            1,
                            make_fake_element_id(ctx),
                            {1, 1},
                            color{0.4f, 0.4f, 0.4f, 1},
                            FILL);
                    }
                });
                column(ctx, GROW, [&]() {
                    inset(
                        ctx,
                        {.left = 40, .right = 40, .top = 40, .bottom = 40},
                        [&]() {
                            with_padding(ctx, 10, [&] {
                                switch (active_demo)
                                {
                                    case 0:
                                        mixed_flow_demo(ctx);
                                        break;
                                    case 1:
                                        layout_demo(ctx);
                                        break;
                                    case 2:
                                        color_demo(ctx);
                                        break;
                                    case 3:
                                        theme_demo(ctx);
                                        break;
                                    case 4:
                                        grid_demo(ctx);
                                        break;
                                    case 5:
                                        simple_text_demo(ctx);
                                        break;
                                }
                            });
                        });
                });
            });
        });
    }
    catch (pass_aborted)
    {
    }
}

alia_kmods_t
to_alia_kmods_t(int mods)
{
    alia_kmods_t result = 0;
    if (mods & GLFW_MOD_SHIFT)
        result |= ALIA_KMOD_SHIFT;
    if (mods & GLFW_MOD_CONTROL)
        result |= ALIA_KMOD_CTRL;
    if (mods & GLFW_MOD_ALT)
        result |= ALIA_KMOD_ALT;
    // TODO: Finish this.
    return result;
}

void
mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    float internal_x
        = (static_cast<float>(x) * framebuffer_width
           / window_width); // / the_ui_scale.x;
    float internal_y
        = (static_cast<float>(y) * framebuffer_height
           / window_height); // / the_ui_scale.y;

    switch (action)
    {
        case GLFW_PRESS: {
            process_mouse_press(
                the_system,
                {internal_x, internal_y},
                button,
                to_alia_kmods_t(mods));
            break;
        }
        case GLFW_RELEASE: {
            process_mouse_release(
                the_system,
                {internal_x, internal_y},
                button,
                to_alia_kmods_t(mods));
            break;
        }
    }
}
void
cursor_position_callback(GLFWwindow* window, double x, double y)
{
    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    float internal_x
        = (static_cast<float>(x) * framebuffer_width
           / window_width); // / the_ui_scale.x;
    float internal_y
        = (static_cast<float>(y) * framebuffer_height
           / window_height); // / the_ui_scale.y;

    process_mouse_motion(the_system, {internal_x, internal_y});
}

// alloc_probe.h
#ifdef _MSC_VER
#include <atomic>
#include <crtdbg.h>
#include <functional>
#include <windows.h>
#endif

struct AllocProbeResult
{
    bool any; // did any allocations occur?
    long first_req = -1; // first global request number seen
    long last_req = -1; // last global request number seen
    unsigned count = 0; // number of alloc/realloc events seen
};

#if 0

#ifdef _MSC_VER
namespace detail {
// Per-thread tracking (the hook is process-wide, so we filter by thread)
static thread_local bool g_active = false;
static thread_local DWORD g_tid = 0;
static thread_local long g_first = -1;
static thread_local long g_last = -1;
static thread_local unsigned g_count = 0;

// Keep previous hook so we can restore it
static _CRT_ALLOC_HOOK g_prevHook = nullptr;

// Our hook: called by the Debug CRT on each alloc/free/realloc
static int __cdecl Hook(
    int allocType,
    void* /*userData*/,
    size_t /*size*/,
    int /*blockType*/,
    long requestNumber,
    const unsigned char* /*filename*/,
    int /*line*/)
{
    if (g_active && GetCurrentThreadId() == g_tid)
    {
        if (allocType == _HOOK_ALLOC || allocType == _HOOK_REALLOC)
        {
            if (g_first < 0)
                g_first = requestNumber; // <- global order id
            g_last = requestNumber;
            ++g_count;
        }
    }
    return g_prevHook
             ? g_prevHook(allocType, nullptr, 0, 0, requestNumber, nullptr, 0)
             : 1;
}

struct Activator
{
    Activator()
    {
        // ensure debug heap is on so hooks fire
        int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        _CrtSetDbgFlag(flags | _CRTDBG_ALLOC_MEM_DF);
        g_prevHook = _CrtSetAllocHook(&Hook);
    }
    ~Activator()
    {
        _CrtSetAllocHook(g_prevHook);
    }
};
} // namespace detail

#endif // _MSC_VER

template<class F>
AllocProbeResult
probe_allocations(F&& f)
{
#ifdef _MSC_VER
    detail::g_active = true;
    detail::g_tid = GetCurrentThreadId();
    detail::g_first = -1;
    detail::g_last = -1;
    detail::g_count = 0;
    detail::Activator guard;

    // Run the code under test
    std::forward<F>(f)();

    AllocProbeResult r;
    r.any = detail::g_count > 0;
    r.first_req = detail::g_first;
    r.last_req = detail::g_last;
    r.count = detail::g_count;

    detail::g_active = false;
    return r;
#else
    // Non-MSVC / Release: can’t use Debug CRT hook; report no info.
    return AllocProbeResult{false, -1, -1, 0};
#endif
}

#else

template<class F>
AllocProbeResult
probe_allocations(F&& f)
{
    std::forward<F>(f)();
    return AllocProbeResult{false, -1, -1, 0};
}

#endif

void
update()
{
    the_time = glfwGetTime();

    static std::chrono::time_point<std::chrono::high_resolution_clock>
        last_frame_time = std::chrono::high_resolution_clock::now();
    auto const start_time = std::chrono::high_resolution_clock::now();

    std::chrono::time_point<std::chrono::high_resolution_clock>
        refresh_finished_time;
    std::chrono::time_point<std::chrono::high_resolution_clock>
        layout_finished_time;

    AllocProbeResult result = probe_allocations([&]() {
        refresh_system(the_system);
        refresh_finished_time = std::chrono::high_resolution_clock::now();

        alia_layout_system_resolve(
            &the_system.layout, the_system.surface_size);

        layout_finished_time = std::chrono::high_resolution_clock::now();

        update(the_system);

        update_glfw_window_info(the_system, the_window);

        // glfwMakeContextCurrent(the_window);
        glViewport(0, 0, the_system.surface_size.x, the_system.surface_size.y);

        alia_rgb c = alia_rgb_from_srgb8(alia_srgb8{0x1f, 0x21, 0x2a});
        glClearColor(c.r, c.g, c.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
            printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

        alia_draw_bucket_table bucket_table = {
            .buckets = {},
            .keys = {},
        };
        alia_draw_context draw_context = {
            .system = &the_draw_system,
            .buckets = &bucket_table,
            .arena = {},
        };
        alia_bump_allocator_init(&draw_context.arena, &the_display_list_arena);

        auto draw_event = alia_make_draw_event({.context = &draw_context});
        dispatch_event(the_system, draw_event);

        while ((err = glGetError()) != GL_NO_ERROR)
            printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

        std::sort(bucket_table.keys.begin(), bucket_table.keys.end());
        for (auto const key : bucket_table.keys)
        {
            alia_draw_material_id material_id = key & 0xffff'ffff;
            alia_z_index z_index = key >> 32;
            alia_draw_bucket* bucket = &bucket_table.buckets[key];
            alia_draw_material* material
                = &the_draw_system.materials[material_id];
            material->vtable.draw_bucket(material->user, bucket);
        }

        while ((err = glGetError()) != GL_NO_ERROR)
            printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    });

    auto const end_time = std::chrono::high_resolution_clock::now();
    auto const refresh_time = std::chrono::duration_cast<
        std::chrono::duration<int64_t, std::micro>>(
        refresh_finished_time - start_time);
    auto const layout_time = std::chrono::duration_cast<
        std::chrono::duration<int64_t, std::micro>>(
        layout_finished_time - refresh_finished_time);
    auto const render_time = std::chrono::duration_cast<
        std::chrono::duration<int64_t, std::micro>>(
        end_time - layout_finished_time);
    auto const frame_time = std::chrono::duration_cast<
        std::chrono::duration<int64_t, std::micro>>(end_time - start_time);

    auto const external_frame_time = std::chrono::duration_cast<
        std::chrono::duration<int64_t, std::micro>>(
        start_time - last_frame_time);

    if (external_frame_time > std::chrono::milliseconds(20))
    {
        std::cout << "FRAME_TIME_SLOW" << std::endl;
    }

    std::cout
        << "frame_time: " // << std::setw(6) << external_frame_time << ": "
        << std::setw(6) << frame_time << ": " << std::setw(6) << refresh_time
        << " / " << std::setw(6) << layout_time << " / " << std::setw(6)
        << render_time << std::endl;

    // std::cout << "allocation count: " << result.count << std::endl;

    last_frame_time = start_time;

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glfwSwapBuffers(the_window);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);
}

void
framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    the_system.surface_size = {float(width), float(height)};
    the_draw_system.surface_size = {float(width), float(height)};
    update();
}

void
check_and_update_resolution()
{
#ifdef __EMSCRIPTEN__
    // 1. Get the current size of the HTML element (in CSS pixels)
    //    This is the size the "window" takes up on the webpage.
    double css_w, css_h;
    emscripten_get_element_css_size("#canvas", &css_w, &css_h);

    // 2. Get the Device Pixel Ratio (e.g., 2.0 for Retina/High-DPI)
    double dpr = emscripten_get_device_pixel_ratio();

    // 3. Calculate the required Buffer Size (Physical Pixels)
    int target_w = (int) (css_w * dpr);
    int target_h = (int) (css_h * dpr);

    // 4. Check what GLFW thinks the size is
    int current_w, current_h;
    glfwGetWindowSize(the_window, &current_w, &current_h);

    // 5. If they mismatch, Resize!
    if (current_w != target_w || current_h != target_h)
    {
        // This resizes the WebGL Backbuffer
        glfwSetWindowSize(the_window, target_w, target_h);

        // Update your Alia System / Renderer Viewport
        // (Assuming you do this in your render code, but if you cache it,
        // update it here)
        the_system.surface_size = {float(target_w), float(target_h)};

        // Helpful log to prove it's working
        std::cout << "Resized to: " << target_w << "x" << target_h
                  << " (DPR: " << dpr << ")\n";
    }
#endif
}

void
main_loop_step()
{
    // Check if we need to close (mostly for desktop)
    if (glfwWindowShouldClose(the_window))
    {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop(); // Stop the loop
#endif
        return;
    }

    glfwPollEvents();

    check_and_update_resolution();

    update();

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);
}

int
main()
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
        the_theme = theme;

        theme_update_needed = false;
    }

    // auto const& theme = get_system(ctx).theme;

#if 0
    // Enable debug heap reports
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    HANDLE hCurrentThread = GetCurrentThread();
    if (SetThreadPriority(hCurrentThread, THREAD_PRIORITY_HIGHEST))
    {
        std::cout << "Main thread priority set to HIGHEST." << std::endl;
    }
    else
    {
        std::cerr << "Failed to set main thread priority." << std::endl;
    }
#endif

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

#ifndef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
    // Emscripten handles this via linker flags (-s USE_WEBGL2=1)
    // But explicitly asking for ES 3.0 doesn't hurt:
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    the_window
        = glfwCreateWindow(1200, 1600, "Alia Renderer", nullptr, nullptr);
    if (!the_window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwSetMouseButtonCallback(the_window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(the_window, framebuffer_size_callback);
    glfwSetCursorPosCallback(the_window, cursor_position_callback);

#ifndef __EMSCRIPTEN__
    glfwMakeContextCurrent(the_window);
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }
#endif

    initialize_ui_system(&the_system, alia_vec2f{1200, 1600});
    the_system.controller = the_demo;

#ifndef __EMSCRIPTEN__
    glEnable(GL_FRAMEBUFFER_SRGB);
#endif

    the_draw_system.next_material_id = ALIA_BUILTIN_MATERIAL_COUNT;

    init_gl_renderer(&the_draw_system, &the_renderer);
    alia_material_register(
        &the_draw_system,
        ALIA_BOX_MATERIAL_ID,
        alia_material_vtable{
            .draw_bucket = render_box_command_list,
        },
        &the_renderer);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    the_draw_system.surface_size = the_system.surface_size;

    // TODO
    the_msdf_text_engine = create_msdf_text_engine(
        &the_draw_system,
        msdf_font_description{
            .metrics = {
                .em_size = 1,
                .line_height = 1.171875,
                .ascender = 0.927734375,
                .descender = -0.244140625,
                .underline_y = -0.09765625,
                .underline_thickness = 0.048828125,
            },
            .atlas = {
                .distance_range = 4,
                .distance_range_middle = 0,
                .font_size = 48,
                .width = 320,
                .height = 320,
            },
            .glyphs = roboto_glyphs,
            .glyph_count = roboto_glyph_count,
            .kerning_pairs = roboto_kerning_pairs,
            .kerning_pair_count = roboto_kerning_pair_count,
        },
        "assets/roboto-msdf.png");

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    initialize_lazy_commit_arena(&the_display_list_arena);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop_step, 0, 1);
#else
    while (!glfwWindowShouldClose(the_window))
        main_loop_step();
#endif

    glfwTerminate();
    return 0;
}
