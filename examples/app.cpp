#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <chrono>
#include <functional> // For std::hash
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "roboto-msdf.h"

#include <alia/flow/arena.hpp>
#include <alia/platforms/glfw/window.hpp>
#include <alia/renderers/gl/renderer.hpp>
#include <alia/text_engines/msdf/msdf.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/display_list.hpp>
#include <alia/ui/drawing.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/api.hpp>
#include <alia/ui/layout/leaf.hpp>
#include <alia/ui/layout/resolution.hpp>
#include <alia/ui/system.hpp>

using namespace alia;

System the_system;
LayoutSpecArena the_layout_spec_arena;
LayoutScratchArena the_scratch_arena;
LayoutContainer the_layout_root = {.child_count = 0, .first_child = nullptr};
LayoutPlacementArena the_layout_placement_arena;
GLFWwindow* the_window;
GlRenderer the_renderer;
DisplayListArena the_display_list_arena;
BoxCommandList the_box_commands;
MsdfTextEngine* the_msdf_text_engine;
CommandList<MsdfDrawCommand> the_msdf_commands;
LayoutPlacementNode* the_initial_layout_placement;
Style the_style = {.padding = 10.0f};

bool
detect_click(Event* event, float x, float y, float width, float height)
{
    return event->type == EventType::Click && event->click.x >= x
        && event->click.x <= x + width && event->click.y >= y
        && event->click.y <= y + height;
}

template<class T>
T*
allocate_spec_node(LayoutSpecArena& arena)
{
    static_assert(std::is_trivially_destructible_v<T>);
    return reinterpret_cast<T*>(arena.allocate(sizeof(T), alignof(T)));
}

template<typename Outer, typename Inner>
Outer*
downcast(Inner* inner_ptr)
{
    static_assert(
        std::is_same<decltype(Outer::base), Inner>::value,
        "Outer::base must be of type Inner");
    static_assert(
        offsetof(Outer, base) == 0, "Outer must embed `base` at offset 0");
    return reinterpret_cast<Outer*>(inner_ptr);
}

template<typename Outer, typename Inner>
Outer const*
downcast(Inner const* inner_ptr)
{
    static_assert(
        std::is_same<decltype(Outer::base), Inner>::value,
        "Outer::base must be of type Inner");
    static_assert(
        offsetof(Outer, base) == 0, "Outer must embed `base` at offset 0");
    return reinterpret_cast<Outer const*>(inner_ptr);
}

bool
do_rect(Context& ctx, Vec2 size, Color color)
{
    switch (ctx.pass.type)
    {
        case PassType::Refresh: {
            auto& layout = ctx.pass.layout_emission;
            LayoutLeafNode* new_node
                = allocate_spec_node<LayoutLeafNode>(the_layout_spec_arena);
            *layout.next_ptr = &new_node->base;
            layout.next_ptr = &new_node->base.next_sibling;
            *new_node = LayoutLeafNode{
                .base = {.vtable = &leaf_vtable, .next_sibling = 0},
                .props
                = {.x_alignment = LayoutAlignment::Start,
                   .y_alignment = LayoutAlignment::Start,
                   .growth_factor = 0},
                .padding = ctx.style->padding,
                .size = size};
            ++layout.active_container->child_count;
            break;
        }
        case PassType::Draw: {
            auto const* placement = ctx.pass.layout_consumption.next_placement;
            ctx.pass.layout_consumption.next_placement = placement->next;
            auto& leaf_placement = *downcast<LeafLayoutPlacement>(placement);
            Box box = {
                .pos = leaf_placement.position, .size = leaf_placement.size};
            draw_box(
                *ctx.pass.display_list_arena,
                *ctx.pass.box_command_list,
                box,
                color);
            break;
        }
        case PassType::Event: {
            auto const* placement = ctx.pass.layout_consumption.next_placement;
            ctx.pass.layout_consumption.next_placement = placement->next;
            auto& leaf_placement = *downcast<LeafLayoutPlacement>(placement);
            Box box = {
                .pos = leaf_placement.position, .size = leaf_placement.size};
            if (detect_click(
                    ctx.pass.event,
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

struct MsdfTextLayoutNode
{
    LayoutNode base;
    LayoutProperties props;
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
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    // TODO: Implement
}

VerticalRequirements
measure_text_vertical(
    MeasurementContext* ctx, LayoutNode* node, float assigned_width)
{
    auto& text = *reinterpret_cast<MsdfTextLayoutNode*>(node);
    auto const* metrics = get_msdf_font_metrics(text.engine);
    return VerticalRequirements{
        .min_size = metrics->line_height * text.font_size + text.padding * 2,
        .growth_factor = 0,
        .ascent = text.props.y_alignment == LayoutAlignment::Baseline
                    ? metrics->ascender * text.font_size + text.padding
                    : 0.0f,
        .descent = text.props.y_alignment == LayoutAlignment::Baseline
                     ? -metrics->descender * text.font_size + text.padding
                     : 0.0f};
}

struct TextLayoutPlacementHeader
{
    LayoutPlacementNode base;
    int fragment_count;
};

struct TextLayoutPlacementFragment
{
    LayoutPlacementNode base;
    Vec2 position;
    Vec2 size;
    char const* text;
    size_t length;
};

void
assign_text_boxes(
    PlacementContext* ctx, LayoutNode* node, Box box, float baseline)
{
    auto& text = *reinterpret_cast<MsdfTextLayoutNode*>(node);
    auto const* metrics = get_msdf_font_metrics(text.engine);

    // TODO: Don't repeatedly measure the text width.
    float width = measure_text_width(
        text.engine, text.text, strlen(text.text), text.font_size);

    auto const placement = resolve_assignment(
        text.props,
        box.size,
        baseline,
        Vec2{
            width + text.padding * 2,
            metrics->line_height * text.font_size + text.padding * 2},
        metrics->ascender * text.font_size + text.padding);

    TextLayoutPlacementHeader* header
        = reinterpret_cast<TextLayoutPlacementHeader*>(ctx->arena->allocate(
            sizeof(TextLayoutPlacementHeader),
            alignof(TextLayoutPlacementHeader)));
    header->fragment_count = 1;
    *ctx->next_ptr = &header->base;
    ctx->next_ptr = &header->base.next;

    TextLayoutPlacementFragment* fragment
        = reinterpret_cast<TextLayoutPlacementFragment*>(ctx->arena->allocate(
            sizeof(TextLayoutPlacementFragment),
            alignof(TextLayoutPlacementFragment)));
    fragment->position
        = box.pos + placement.pos + Vec2{text.padding, text.padding};
    fragment->size = placement.size - Vec2{text.padding * 2, text.padding * 2};
    fragment->text = text.text;
    fragment->length = strlen(text.text);
    *ctx->next_ptr = &fragment->base;
    ctx->next_ptr = &fragment->base.next;
}

HorizontalRequirements
measure_text_wrapped_horizontal(MeasurementContext* ctx, LayoutNode* node)
{
    return HorizontalRequirements{0, 0};
}

WrappingRequirements
measure_text_wrapped_vertical(
    MeasurementContext* ctx,
    LayoutNode* node,
    float current_x_offset,
    float line_width)
{
    auto& text = *reinterpret_cast<MsdfTextLayoutNode*>(node);
    auto const* metrics = get_msdf_font_metrics(text.engine);

    size_t length = strlen(text.text);

    auto break_result = break_text(
        text.engine,
        text.text,
        0,
        length,
        length,
        text.font_size,
        line_width - current_x_offset - text.padding * 2);
    bool wrapped_immediately = (break_result.first == 0);

    int wrap_count = 0;
    size_t index = break_result.first;
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
            line_width - text.padding * 2);
        index = break_result.first;
        new_x = break_result.second;
    }

    return WrappingRequirements{
        .line_height
        = metrics->line_height * text.font_size + text.padding * 2,
        .ascent = metrics->ascender * text.font_size + text.padding,
        .descent = -metrics->descender * text.font_size + text.padding,
        .wrap_count = wrap_count,
        .wrapped_immediately = wrapped_immediately,
        .new_x_offset = new_x};
}

void
assign_text_wrapped_boxes(
    PlacementContext* ctx,
    LayoutNode* node,
    WrappingAssignment const* assignment)
{
    auto& text = *reinterpret_cast<MsdfTextLayoutNode*>(node);
    auto const* metrics = get_msdf_font_metrics(text.engine);

    size_t length = strlen(text.text);

    TextLayoutPlacementHeader* header
        = reinterpret_cast<TextLayoutPlacementHeader*>(ctx->arena->allocate(
            sizeof(TextLayoutPlacementHeader),
            alignof(TextLayoutPlacementHeader)));
    header->fragment_count = 0;
    *ctx->next_ptr = &header->base;
    ctx->next_ptr = &header->base.next;

    float x = assignment->first_line_x_offset;
    float y = assignment->y_base + assignment->first_line.baseline_offset;
    float next_y = assignment->y_base + assignment->first_line.line_height
                 + assignment->middle_lines.baseline_offset;

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
            assignment->line_width - x - text.padding * 2);
        size_t const end_index = break_result.first;

        if (end_index == length)
        {
            y += assignment->last_line.baseline_offset
               - assignment->middle_lines.baseline_offset;
        }

        TextLayoutPlacementFragment* fragment
            = reinterpret_cast<TextLayoutPlacementFragment*>(
                ctx->arena->allocate(
                    sizeof(TextLayoutPlacementFragment),
                    alignof(TextLayoutPlacementFragment)));
        fragment->position
            = {x + assignment->x_base + text.padding,
               y - metrics->ascender * text.font_size};
        fragment->size
            = {assignment->line_width - x - text.padding * 2,
               metrics->line_height * text.font_size};
        fragment->text = text.text + index;
        fragment->length = end_index - index;
        *ctx->next_ptr = &fragment->base;
        ctx->next_ptr = &fragment->base.next;
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
do_text(Context& ctx, Color color, float scale, char const* text)
{
    bool result = false;
    switch (ctx.pass.type)
    {
        case PassType::Refresh: {
            auto& layout = ctx.pass.layout_emission;
            MsdfTextLayoutNode* new_node
                = allocate_spec_node<MsdfTextLayoutNode>(
                    the_layout_spec_arena);
            new_node->base.vtable = &text_layout_vtable;
            new_node->base.next_sibling = nullptr;
            new_node->props.growth_factor = 0;
            new_node->props.x_alignment = LayoutAlignment::Start;
            new_node->props.y_alignment = LayoutAlignment::Baseline;
            new_node->text = text;
            new_node->font_size = scale;
            new_node->engine = the_msdf_text_engine;
            new_node->padding = ctx.style->padding;
            *layout.next_ptr = &new_node->base;
            layout.next_ptr = &new_node->base.next_sibling;
            ++layout.active_container->child_count;
            break;
        }
        case PassType::Draw: {
            auto const* placement = ctx.pass.layout_consumption.next_placement;
            ctx.pass.layout_consumption.next_placement = placement->next;
            auto& text_placement
                = *downcast<TextLayoutPlacementHeader>(placement);
            for (int i = 0; i < text_placement.fragment_count; ++i)
            {
                auto const* fragment_placement
                    = ctx.pass.layout_consumption.next_placement;
                ctx.pass.layout_consumption.next_placement
                    = fragment_placement->next;
                auto& fragment = *downcast<TextLayoutPlacementFragment>(
                    fragment_placement);
                draw_text(
                    the_msdf_text_engine,
                    *ctx.pass.display_list_arena,
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
            auto const* placement = ctx.pass.layout_consumption.next_placement;
            ctx.pass.layout_consumption.next_placement = placement->next;
            auto& text_placement
                = *downcast<TextLayoutPlacementHeader>(placement);
            for (int i = 0; i < text_placement.fragment_count; ++i)
            {
                auto const* fragment_placement
                    = ctx.pass.layout_consumption.next_placement;
                ctx.pass.layout_consumption.next_placement
                    = fragment_placement->next;
                auto& fragment = *downcast<TextLayoutPlacementFragment>(
                    fragment_placement);
                Box box = {.pos = fragment.position, .size = fragment.size};
                if (detect_click(
                        ctx.pass.event,
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

void
rectangle_demo(Context& ctx)
{
    static bool invert = false;

    vbox(ctx, [&]() {
        flow(ctx, [&]() {
            float x = 0.0f;
            for (int i = 0; i < 10; ++i)
            {
                for (int j = 0; j < 500; ++j)
                {
                    float f = fmod(x, 1.0f);
                    if (do_rect(
                            ctx,
                            {24, 24},
                            invert ? Color{f, 0.1f, 1.0f - f, 1}
                                   : Color{1.0f - f, 0.1f, f, 1}))
                    {
                        invert = !invert;
                        return;
                    }
                    x += 0.0015f;
                }

                for (int j = 0; j < 1; ++j)
                {
                    if (do_text(
                            ctx,
                            GRAY,
                            20 + i * 12 + j * 4,
                            "Lorem ipsum dolor sit amet, consectetur "
                            "adipiscing "
                            "elit. Proin sed dictum massa. Maecenas et "
                            "euismod "
                            "lorem, ut dapibus eros. Nam maximus, purus vitae "
                            "mollis ornare, tortor justo posuere neque, at "
                            "lacinia ante metus eget diam. Aenean sit amet "
                            "posuere metus. In hac habitasse platea dictumst. "
                            "Nam "
                            "sed turpis ultricies tellus auctor egestas. Ut "
                            "laoreet nisi nisi, id posuere tortor tincidunt "
                            "a. "
                            "Pellentesque placerat vulputate massa at semper. "
                            "Fusce malesuada porttitor enim dignissim "
                            "viverra. In "
                            "aliquam, odio nec sagittis elementum, elit enim "
                            "auctor turpis, sit amet volutpat enim massa ac "
                            "orci. "
                            "Maecenas iaculis, ex at pulvinar volutpat, "
                            "ligula "
                            "nulla pellentesque tellus, vel aliquam nunc "
                            "dolor eu "
                            "risus."))
                    {
                        invert = !invert;
                        return;
                    }
                }
            }
        });

        hbox(ctx, [&]() {
            if (do_rect(ctx, {400, 24}, GRAY))
            {
                invert = !invert;
                return;
            }
        });
    });
}

void
text_demo(Context& ctx)
{
    static bool invert = false;

    vbox(ctx, [&]() {
        for (int i = 0; i < 10; ++i)
        {
            with_padding(ctx, 15, [&] {
                hbox(ctx, [&]() {
                    do_text(ctx, GRAY, 40, "test");
                    flow(ctx, 1.0f, [&]() {
                        do_text(
                            ctx,
                            GRAY,
                            10 + i * 6,
                            "Lorem ipsum dolor sit amet, consectetur "
                            "adipiscing elit. Proin sed dictum massa. "
                            "Maecenas et euismod lorem, ut dapibus eros. "
                            "Nam maximus, purus vitae mollis ornare, tortor "
                            "justo posuere neque, at lacinia ante metus eget "
                            "diam. Aenean sit amet posuere metus. In hac "
                            "habitasse platea dictumst. Nam sed turpis "
                            "ultricies tellus auctor egestas. Ut laoreet nisi "
                            "nisi, id posuere tortor tincidunt a. "
                            "Pellentesque placerat vulputate massa at semper. "
                            "Fusce malesuada porttitor enim dignissim "
                            "viverra. In aliquam, odio nec sagittis "
                            "elementum, elit enim auctor turpis, sit amet "
                            "volutpat enim massa ac orci. Maecenas iaculis, "
                            "ex at pulvinar volutpat, ligula nulla "
                            "pellentesque tellus, vel aliquam nunc dolor eu "
                            "risus.");
                    });
                });
            });
        }
    });
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
        Context event_ctx = {
            Pass{
                PassType::Event,
                {nullptr, nullptr, nullptr},
                {the_initial_layout_placement},
                nullptr,
                nullptr,
                &event},
            &the_style,
            &the_system};
        text_demo(event_ctx);
    }
}

void
update()
{
    static std::chrono::time_point<std::chrono::high_resolution_clock>
        last_frame_time = std::chrono::high_resolution_clock::now();
    auto const start_time = std::chrono::high_resolution_clock::now();

    std::uint32_t root_index;
    the_layout_spec_arena.reset();
    Context refresh_ctx = {
        Pass{
            PassType::Refresh,
            {&the_layout_spec_arena,
             &the_layout_root,
             &the_layout_root.first_child},
            {nullptr},
            nullptr,
            nullptr,
            nullptr},
        &the_style,
        &the_system};
    *refresh_ctx.pass.layout_emission.next_ptr = 0;
    text_demo(refresh_ctx);

    update_glfw_window_info(the_system, the_window);

    the_layout_placement_arena.reset();
    the_initial_layout_placement = resolve_layout(
        the_scratch_arena,
        the_layout_placement_arena,
        *the_layout_root.first_child,
        the_system.framebuffer_size);

    auto const layout_finished_time
        = std::chrono::high_resolution_clock::now();

    // glfwMakeContextCurrent(the_window);
    glViewport(
        0, 0, the_system.framebuffer_size.x, the_system.framebuffer_size.y);

    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    the_display_list_arena.reset();
    clear_command_list(the_box_commands);
    clear_command_list(the_msdf_commands);
    Context draw_ctx = {
        Pass{
            PassType::Draw,
            {nullptr, nullptr, nullptr},
            {the_initial_layout_placement},
            &the_display_list_arena,
            &the_box_commands,
            nullptr},
        &the_style,
        &the_system};
    text_demo(draw_ctx);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    render_box_command_list(&the_renderer, the_system, the_box_commands);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    // do_text_demo();

    render_command_list(
        the_msdf_text_engine, the_msdf_commands, the_system.framebuffer_size);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    auto const end_time = std::chrono::high_resolution_clock::now();
    auto const layout_time = std::chrono::duration_cast<
        std::chrono::duration<int64_t, std::micro>>(
        layout_finished_time - start_time);
    auto const render_time = std::chrono::duration_cast<
        std::chrono::duration<int64_t, std::micro>>(
        end_time - layout_finished_time);
    auto const frame_time = std::chrono::duration_cast<
        std::chrono::duration<int64_t, std::micro>>(end_time - start_time);

    // printf("the_glyph_count: %d\n", the_glyph_instance_count);
    auto const external_frame_time = std::chrono::duration_cast<
        std::chrono::duration<int64_t, std::micro>>(
        start_time - last_frame_time);

    if (external_frame_time > std::chrono::milliseconds(20))
    {
        std::cout << "FRAME_TIME_SLOW" << std::endl;
    }

    std::cout
        << "frame_time: " << std::setw(6) << external_frame_time << ": "
        << std::setw(6) << frame_time << ": " << std::setw(6) << layout_time
        << " / " << std::setw(6) << render_time << std::endl;

    last_frame_time = start_time;

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

    the_scratch_arena.initialize();
    the_layout_spec_arena.initialize();
    the_layout_placement_arena.initialize();

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
