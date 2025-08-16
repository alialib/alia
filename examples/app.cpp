#include <glad/glad.h>

#include <GLFW/glfw3.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <chrono>
#include <functional> // For std::hash
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "roboto-msdf.h"

#include <alia/base/arena.hpp>
#include <alia/platforms/glfw/window.hpp>
#include <alia/renderers/gl/renderer.hpp>
#include <alia/text_engines/msdf/msdf.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/display_list.hpp>
#include <alia/ui/drawing.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/compositors/column.hpp>
#include <alia/ui/layout/compositors/flow.hpp>
#include <alia/ui/layout/compositors/hyperflow.hpp>
#include <alia/ui/layout/compositors/row.hpp>
#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/leaf.hpp>
#include <alia/ui/layout/modifiers/alignment_override.hpp>
#include <alia/ui/layout/modifiers/growth_override.hpp>
#include <alia/ui/layout/modifiers/inset.hpp>
#include <alia/ui/layout/modifiers/min_size.hpp>
#include <alia/ui/layout/modifiers/placement_hook.hpp>
#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/system.hpp>

using namespace alia;

System the_system;
GLFWwindow* the_window;
GlRenderer the_renderer;
DisplayListArena the_display_list_arena;
BoxCommandList the_box_commands;
MsdfTextEngine* the_msdf_text_engine;
CommandList<MsdfDrawCommand> the_msdf_commands;
Style the_style = {.padding = 10.0f};

bool
detect_click(Event* event, float x, float y, float width, float height)
{
    return event->type == EventType::Click && event->click.x >= x
        && event->click.x <= x + width && event->click.y >= y
        && event->click.y <= y + height;
}

bool
do_rect(Context& ctx, Vec2 size, Color color, LayoutFlagSet flags)
{
    switch (ctx.pass.type)
    {
        case PassType::Refresh: {
            auto& layout = ctx.pass.refresh.layout_emission;
            LayoutLeafNode* new_node
                = arena_alloc<LayoutLeafNode>(ctx.system->layout.node_arena);
            *layout.next_ptr = &new_node->base;
            layout.next_ptr = &new_node->base.next_sibling;
            *new_node = LayoutLeafNode{
                .base = {.vtable = &leaf_vtable, .next_sibling = 0},
                .flags = flags,
                .padding = ctx.style->padding,
                .size = size};
            break;
        }
        case PassType::Draw: {
            auto& leaf_placement = *arena_alloc<LeafLayoutPlacement>(
                ctx.system->layout.placement_arena);
            Box box = {
                .pos = leaf_placement.position, .size = leaf_placement.size};
            draw_box(
                *ctx.pass.draw.display_list_arena,
                *ctx.pass.draw.box_command_list,
                box,
                color);
            break;
        }
        case PassType::Event: {
            auto& leaf_placement = *arena_alloc<LeafLayoutPlacement>(
                ctx.system->layout.placement_arena);
            Box box = {
                .pos = leaf_placement.position, .size = leaf_placement.size};
            if (detect_click(
                    ctx.pass.event.event,
                    box.pos.x,
                    box.pos.y,
                    box.size.x,
                    box.size.y))
                return true;
            break;
        }
    }
    return false;
}

bool
do_rect_with_offset(
    Context& ctx, Vec2 size, Color color, LayoutFlagSet flags, Vec2 offset)
{
    switch (ctx.pass.type)
    {
        case PassType::Refresh: {
            auto& layout = ctx.pass.refresh.layout_emission;
            LayoutLeafNode* new_node
                = arena_alloc<LayoutLeafNode>(ctx.system->layout.node_arena);
            *layout.next_ptr = &new_node->base;
            layout.next_ptr = &new_node->base.next_sibling;
            *new_node = LayoutLeafNode{
                .base = {.vtable = &leaf_vtable, .next_sibling = 0},
                .flags = flags,
                .padding = ctx.style->padding,
                .size = size};
            break;
        }
        case PassType::Draw: {
            auto& leaf_placement = *arena_alloc<LeafLayoutPlacement>(
                ctx.system->layout.placement_arena);
            Box box
                = {.pos = leaf_placement.position + offset,
                   .size = leaf_placement.size};
            draw_box(
                *ctx.pass.draw.display_list_arena,
                *ctx.pass.draw.box_command_list,
                box,
                color);
            break;
        }
        case PassType::Event: {
            auto& leaf_placement = *arena_alloc<LeafLayoutPlacement>(
                ctx.system->layout.placement_arena);
            Box box
                = {.pos = leaf_placement.position + offset,
                   .size = leaf_placement.size};
            if (detect_click(
                    ctx.pass.event.event,
                    box.pos.x,
                    box.pos.y,
                    box.size.x,
                    box.size.y))
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
    Vec2 size;
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
    Insets insets;
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
min_size(Vec2 size)
{
    return {size};
}
constexpr margins_t
margins(Insets insets)
{
    return {insets};
}

template<class LayoutMods>
void
do_rect(Context& ctx, Vec2 size, Color color, LayoutMods mods)
{
    apply_mods(ctx, mods, [&] { do_rect(ctx, size, color, FILL); });
}

template<class Content>
void
concrete_panel(
    Context& ctx, Color color, LayoutFlagSet flags, Content&& content)
{
    placement_hook(ctx, flags, [&](auto const& placement) {
        if (ctx.pass.type == PassType::Draw)
        {
            draw_box(
                *ctx.pass.draw.display_list_arena,
                *ctx.pass.draw.box_command_list,
                placement.box,
                color);
        }

        std::forward<Content>(content)();
    });
}

template<class Content, class LayoutMods>
void
panel(Context& ctx, Color color, LayoutMods mods, Content&& content)
{
    apply_mods(ctx, mods, [&] {
        concrete_panel(ctx, color, FILL, std::forward<Content>(content));
    });
}

/// ----

struct MsdfTextLayoutNode
{
    LayoutNode base;
    LayoutFlagSet flags;
    float padding;
    MsdfTextEngine* engine;
    char const* text;
    float font_size;
};

HorizontalRequirements
measure_text_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    auto& text = *reinterpret_cast<MsdfTextLayoutNode*>(node);
    float width = measure_text_width(
        text.engine, text.text, strlen(text.text), text.font_size);
    return HorizontalRequirements{
        .min_size = width + text.padding * 2, .growth_factor = 0};
}

void
assign_text_widths(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    float assigned_width)
{
    // TODO: Implement
}

VerticalRequirements
measure_text_vertical(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    float assigned_width)
{
    auto& text = *reinterpret_cast<MsdfTextLayoutNode*>(node);
    auto const* metrics = get_msdf_font_metrics(text.engine);
    return VerticalRequirements{
        .min_size = metrics->line_height * text.font_size + text.padding * 2,
        .growth_factor = 0,
        .ascent = (text.flags & Y_ALIGNMENT_MASK) == BASELINE_Y
                    ? metrics->ascender * text.font_size + text.padding
                    : 0.0f,
        .descent = (text.flags & Y_ALIGNMENT_MASK) == BASELINE_Y
                     ? -metrics->descender * text.font_size + text.padding
                     : 0.0f};
}

struct TextLayoutPlacementHeader
{
    int fragment_count;
};

struct TextLayoutPlacementFragment
{
    Vec2 position;
    Vec2 size;
    char const* text;
    size_t length;
};

void
assign_text_boxes(
    PlacementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    Box box,
    float baseline)
{
    auto& text = *reinterpret_cast<MsdfTextLayoutNode*>(node);
    auto const* metrics = get_msdf_font_metrics(text.engine);

    // TODO: Don't repeatedly measure the text width.
    float width = measure_text_width(
        text.engine, text.text, strlen(text.text), text.font_size);

    auto const placement = resolve_padded_assignment(
        adjust_flags_for_main_axis(text.flags, main_axis),
        box.size,
        baseline,
        Vec2{width, metrics->line_height * text.font_size},
        metrics->ascender * text.font_size,
        text.padding);

    TextLayoutPlacementHeader* header
        = arena_alloc<TextLayoutPlacementHeader>(*ctx->arena);
    header->fragment_count = 1;

    TextLayoutPlacementFragment* fragment
        = arena_alloc<TextLayoutPlacementFragment>(*ctx->arena);
    fragment->position = box.pos + placement.pos;
    fragment->size = placement.size;
    fragment->text = text.text;
    fragment->length = strlen(text.text);
}

HorizontalRequirements
measure_text_wrapped_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    return HorizontalRequirements{0, 0};
}

WrappingRequirements
measure_text_wrapped_vertical(
    MeasurementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    float current_x_offset,
    float line_width)
{
    auto& text = *reinterpret_cast<MsdfTextLayoutNode*>(node);
    auto const* metrics = get_msdf_font_metrics(text.engine);

    size_t length = strlen(text.text);

    WrappingRequirements requirements;

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
    PlacementContext* ctx,
    MainAxisIndex main_axis,
    LayoutNode* node,
    WrappingAssignment const* assignment)
{
    auto& text = *reinterpret_cast<MsdfTextLayoutNode*>(node);
    auto const* metrics = get_msdf_font_metrics(text.engine);

    size_t length = strlen(text.text);

    TextLayoutPlacementHeader* header
        = arena_alloc<TextLayoutPlacementHeader>(*ctx->arena);
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

        TextLayoutPlacementFragment* fragment
            = arena_alloc<TextLayoutPlacementFragment>(*ctx->arena);
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

LayoutNodeVtable text_layout_vtable
    = {measure_text_horizontal,
       assign_text_widths,
       measure_text_vertical,
       assign_text_boxes,
       measure_text_wrapped_horizontal,
       measure_text_wrapped_vertical,
       assign_text_wrapped_boxes};

template<class Content>
void
with_padding(Context& ctx, float padding, Content&& content)
{
    float old_padding = ctx.style->padding;
    ctx.style->padding = padding;
    content();
    ctx.style->padding = old_padding;
}

bool
do_text(
    Context& ctx,
    Color color,
    float scale,
    char const* text,
    LayoutFlagSet flags = NO_FLAGS)
{
    bool result = false;
    switch (ctx.pass.type)
    {
        case PassType::Refresh: {
            auto& layout = ctx.pass.refresh.layout_emission;
            MsdfTextLayoutNode* new_node = arena_alloc<MsdfTextLayoutNode>(
                ctx.system->layout.node_arena);
            new_node->base.vtable = &text_layout_vtable;
            new_node->base.next_sibling = nullptr;
            new_node->flags = flags;
            new_node->text = text;
            new_node->font_size = scale;
            new_node->engine = the_msdf_text_engine;
            new_node->padding = ctx.style->padding;
            *layout.next_ptr = &new_node->base;
            layout.next_ptr = &new_node->base.next_sibling;
            break;
        }
        case PassType::Draw: {
            auto& text_placement = *arena_alloc<TextLayoutPlacementHeader>(
                ctx.system->layout.placement_arena);
            for (int i = 0; i < text_placement.fragment_count; ++i)
            {
                auto& fragment = *arena_alloc<TextLayoutPlacementFragment>(
                    ctx.system->layout.placement_arena);
                draw_text(
                    the_msdf_text_engine,
                    *ctx.pass.draw.display_list_arena,
                    the_msdf_commands,
                    fragment.text,
                    fragment.length,
                    scale,
                    fragment.position,
                    color);
            }
            break;
        }
        case PassType::Event: {
            auto& text_placement = *arena_alloc<TextLayoutPlacementHeader>(
                ctx.system->layout.placement_arena);
            for (int i = 0; i < text_placement.fragment_count; ++i)
            {
                auto& fragment = *arena_alloc<TextLayoutPlacementFragment>(
                    ctx.system->layout.placement_arena);
                Box box = {.pos = fragment.position, .size = fragment.size};
                if (detect_click(
                        ctx.pass.event.event,
                        box.pos.x,
                        box.pos.y,
                        box.size.x,
                        box.size.y))
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

void
rectangle_demo(Context& ctx)
{
    static bool invert = false;

    with_padding(ctx, 0, [&] {
        column(ctx, [&]() {
            flow(ctx, [&]() {
                float x = 0.0f;
                for (int i = 0; i < 10; ++i)
                {
                    flow(ctx, [&]() {
                        for (int j = 0; j < 500; ++j)
                        {
                            float f = fmod(x, 1.0f);
                            if (do_rect(
                                    ctx,
                                    {24, 24},
                                    invert ? Color{f, 0.1f, 1.0f - f, 1}
                                           : Color{1.0f - f, 0.1f, f, 1},
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
                                    GRAY,
                                    24 + i * 6 + j * 4,
                                    "lorem ipsum");
                                if (do_text(
                                        ctx,
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
text_demo(Context& ctx)
{
    static bool invert = false;

    inset(ctx, {.left = 10, .right = 10, .top = 10, .bottom = 10}, [&]() {
        column(ctx, [&]() {
            for (int i = 0; i < 10; ++i)
            {
                with_padding(ctx, 8, [&] {
                    row(ctx, [&]() {
                        do_text(ctx, GRAY, 40, "test");
                        flow(ctx, GROW, [&]() {
                            for (int j = 0; j < 10; ++j)
                            {
                                flow(ctx, [&]() {
                                    do_text(
                                        ctx, GRAY, 10 + i * 6, lorem_ipsum);
                                });
                                do_text(ctx, GRAY, 16 + i * 4, "lorum");
                                do_text(ctx, GRAY, 20 + i * 2, "ipsum");
                            }
                        });
                    });
                });
            }
        });
    });
}

void
nested_flow_demo(Context& ctx)
{
    static bool invert = false;

    inset(ctx, {.left = 100, .right = 100, .top = 100, .bottom = 100}, [&]() {
        flow(ctx, [&]() {
            for (int i = 0; i < 10; ++i)
            {
                flow(ctx, [&]() {
                    do_text(ctx, GRAY, 10 + i * 6, lorem_ipsum);
                });
                do_text(ctx, GRAY, 16 + i * 4, "lorum");
                do_text(ctx, GRAY, 20 + i * 2, "ipsum");
            }
        });
    });
}

void
mixed_flow_demo(Context& ctx)
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
                        ctx, {24, 24}, Color{f, 0.1f, 1.0f - f, 1}, CENTER);
                    x += 0.1f;
                }

                do_text(ctx, GRAY, 24 + i * 6, "lorem ipsum", BASELINE_Y);
                do_text(ctx, GRAY, 12 + i * 6, lorem_ipsum, BASELINE_Y);
            }
        });
    });
}

void
layout_demo_flow(Context& ctx)
{
    float x = 0.0f;
    hyperflow(ctx, [&]() {
        for (int i = 0; i < 600; ++i)
        {
            float intensity = ((i / 4) % 3) * 0.02f;
            inset(
                ctx,
                {.left = 10, .right = 10, .top = 10, .bottom = 10},
                [&]() {
                    concrete_panel(
                        ctx,
                        Color{
                            0.14f + intensity,
                            0.14f + intensity,
                            0.18f + intensity,
                            1},
                        NO_FLAGS,
                        [&]() {
                            min_size_constraint(ctx, {0, 200}, [&]() {
                                row(ctx, [&]() {
                                    float f = fmod(x, 1.0f);
                                    do_rect(
                                        ctx,
                                        {24, float((i & 7) * 12 + 12)},
                                        Color{f, 0.1f, 1.0f - f, 1},
                                        LayoutFlagSet(
                                            (i & 3)
                                            << CROSS_ALIGNMENT_BIT_OFFSET));
                                    x += 0.01f;
                                });
                            });
                        });
                });
        }
    });
}

void
layout_growth_demo(Context& ctx)
{
    float x = 0.0f;
    row(ctx, [&]() {
        for (int i = 0; i < 12; ++i)
        {
            float f = fmod(x, 1.0f);
            growth_override(ctx, i * 1.0f, [&]() {
                do_rect(
                    ctx,
                    {6, 12},
                    Color{f, 0.1f, 1.0f - f, 1},
                    FILL | (i & 1 ? GROW : NO_FLAGS));
            });
            x += 0.08f;
        }
    });
}

void
do_animated_rect(
    Context& ctx,
    bool& initialized,
    Vec2& offset,
    Vec2 size,
    Color color,
    LayoutFlagSet flags)
{
    placement_hook(ctx, FILL, [&](auto outer_placement) {
        alignment_override(ctx, flags, [&]() {
            placement_hook(ctx, FILL, [&](auto inner_placement) {
                Vec2 inner_pos
                    = inner_placement.box.pos - outer_placement.box.pos;
                if (ctx.pass.type == PassType::Draw)
                {
                    if (!initialized)
                    {
                        offset = inner_pos;
                        initialized = true;
                    }
                    offset += (inner_pos - offset) * 0.1f;
                }
                do_rect_with_offset(
                    ctx, size, color, FILL, offset - inner_pos);
            });
        });
    });
}

void
alignment_override_demo(Context& ctx)
{
    static bool invert = false;
    static bool initialized[12] = {false};
    static Vec2 offsets[12] = {0};
    float x = 0.0f;
    row(ctx, [&]() {
        concrete_panel(ctx, Color{0.14f, 0.14f, 0.16f, 1}, CENTER, [&]() {
            inset(ctx, {.left = 4, .right = 4, .top = 4, .bottom = 4}, [&]() {
                if (do_rect(
                        ctx,
                        {24, 24},
                        invert ? Color{1, 1, 1, 1} : Color{0, 0, 0, 0},
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
                            Color{
                                0.14f + 0.06f * f,
                                0.14f + 0.06f * f,
                                0.16f + 0.08f * f,
                                1},
                            NO_FLAGS,
                            [&]() {
                                do_animated_rect(
                                    ctx,
                                    initialized[i],
                                    offsets[i],
                                    {24, 24},
                                    Color{f, 0.1f, 1.0f - f, 1},
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
layout_mods_demo(Context& ctx)
{
    float x = 0.0f;
    flow(ctx, [&]() {
        for (int i = 0; i < 36; ++i)
        {
            float f = fmod(x, 1.0f);
            panel(
                ctx,
                Color{
                    0.14f + 0.06f * f,
                    0.14f + 0.06f * f,
                    0.16f + 0.08f * f,
                    1},
                min_size({200, 200})
                    | margins(
                        {.left = 10, .right = 10, .top = 10, .bottom = 10}),
                [&]() {
                    do_rect(
                        ctx,
                        {float(4 * i), float(4 * i)},
                        Color{f, 0.1f, 1.0f - f, 1},
                        align_right | center_y);
                });
            x += 0.2f;
        }
    });
}

void
layout_demo(Context& ctx)
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
                                    {float((j & 7) * 12 + 12), 24},
                                    Color{f, 0.1f, 1.0f - f, 1},
                                    LayoutFlagSet(
                                        (j & 3) << X_ALIGNMENT_BIT_OFFSET));
                                x += 0.02f;
                            }
                        });
                    });
                column(ctx, GROW, [&]() {
                    do_text(ctx, GRAY, 24, "layout_growth_demo");
                    layout_growth_demo(ctx);
                    do_text(ctx, GRAY, 24, "alignment_override_demo");
                    alignment_override_demo(ctx);
                    do_text(ctx, GRAY, 24, "layout_mods_demo");
                    layout_mods_demo(ctx);
                    do_text(ctx, GRAY, 24, "mixed_flow_demo");
                    mixed_flow_demo(ctx);
                    // do_text(ctx, GRAY, 24, "layout_demo_flow");
                    // layout_demo_flow(ctx);
                });
            });
        });
    });
}

void
the_demo(Context& ctx)
{
    layout_demo(ctx);
}

void
mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        Event event;
        event.type = EventType::Click;

        int framebuffer_width, framebuffer_height;
        glfwGetFramebufferSize(
            window, &framebuffer_width, &framebuffer_height);

        int window_width, window_height;
        glfwGetWindowSize(window, &window_width, &window_height);

        event.click.x
            = (static_cast<float>(x) * framebuffer_width
               / window_width); // / the_ui_scale.x;
        event.click.y
            = (static_cast<float>(y) * framebuffer_height
               / window_height); // / the_ui_scale.y;
        std::uint32_t root_index;
        the_system.layout.placement_arena.reset();
        Context event_ctx
            = {.pass = {.type = PassType::Event, .event = {.event = &event}},
               .style = &the_style,
               .system = &the_system};
        the_demo(event_ctx);
    }
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
    static std::chrono::time_point<std::chrono::high_resolution_clock>
        last_frame_time = std::chrono::high_resolution_clock::now();
    auto const start_time = std::chrono::high_resolution_clock::now();

    std::chrono::time_point<std::chrono::high_resolution_clock>
        refresh_finished_time;
    std::chrono::time_point<std::chrono::high_resolution_clock>
        layout_finished_time;

    AllocProbeResult result = probe_allocations([&]() {
        the_system.layout.node_arena.reset();
        Context refresh_ctx
            = {{PassType::Refresh,
                {.refresh
                 = {.layout_emission
                    = {&the_system.layout.node_arena,
                       &the_system.layout.root.first_child}}}},
               &the_style,
               &the_system};
        the_demo(refresh_ctx);
        *refresh_ctx.pass.refresh.layout_emission.next_ptr = 0;

        refresh_finished_time = std::chrono::high_resolution_clock::now();

        the_system.layout.placement_arena.reset();
        resolve_layout(the_system.layout, the_system.framebuffer_size);

        layout_finished_time = std::chrono::high_resolution_clock::now();

        update_glfw_window_info(the_system, the_window);

        // glfwMakeContextCurrent(the_window);
        glViewport(
            0,
            0,
            the_system.framebuffer_size.x,
            the_system.framebuffer_size.y);

        glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
            printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

        the_display_list_arena.reset();
        clear_command_list(the_box_commands);
        clear_command_list(the_msdf_commands);
        the_system.layout.placement_arena.reset();
        Context draw_ctx
            = {.pass
               = {.type = PassType::Draw,
                  .draw
                  = {.display_list_arena = &the_display_list_arena,
                     .box_command_list = &the_box_commands}},
               .style = &the_style,
               .system = &the_system};
        the_demo(draw_ctx);

        while ((err = glGetError()) != GL_NO_ERROR)
            printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

        render_box_command_list(&the_renderer, the_system, the_box_commands);

        while ((err = glGetError()) != GL_NO_ERROR)
            printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

        render_command_list(
            the_msdf_text_engine,
            the_msdf_commands,
            the_system.framebuffer_size);

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

    std::cout << "frame_time: " << std::setw(6) << external_frame_time << ": "
              << std::setw(6) << frame_time << ": " << std::setw(6)
              << refresh_time << " / " << std::setw(6) << layout_time << " / "
              << std::setw(6) << render_time << std::endl;

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
    // Update the system's framebuffer size
    the_system.framebuffer_size = {float(width), float(height)};

    update();
}

int
main()
{
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

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    glfwMakeContextCurrent(the_window);
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    initialize(
        the_system,
        Vec2{1200, 1600}, // Initial framebuffer size
        Vec2{1.0f, 1.0f}); // Initial UI zoom

    init_gl_renderer(&the_renderer);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    // TODO
    the_msdf_text_engine = create_msdf_text_engine(
        MsdfFontDescription{
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
        "roboto-msdf.png");

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    the_display_list_arena.initialize();
    clear_command_list(the_box_commands);
    clear_command_list(the_msdf_commands);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(the_window))
    {
        update();

        glfwPollEvents();

        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
            printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);
    }

    glfwTerminate();
    return 0;
}
