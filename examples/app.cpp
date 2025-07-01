#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <chrono>
#include <functional> // For std::hash
#include <iostream>
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
#include <alia/ui/layout/resolution.hpp>
#include <alia/ui/system.hpp>

using namespace alia;

System the_system;
LayoutSpecArena the_layout_spec_arena;
LayoutScratchArena the_scratch_arena;
LayoutContainer the_layout_root = {.first_child = nullptr, .child_count = 0};
LayoutPlacementArena the_layout_placement_arena;
GLFWwindow* the_window;
GlRenderer the_renderer;
DisplayListArena the_display_list_arena;
BoxCommandList the_box_commands;
MsdfTextEngine* the_msdf_text_engine;
CommandList<MsdfDrawCommand> the_msdf_commands;

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

bool
do_rect(Context& ctx, Vec2 size, Color color)
{
    switch (ctx.pass.type)
    {
        case PassType::Refresh: {
            auto& layout = ctx.pass.layout_emission;
            LayoutNode* new_node
                = allocate_spec_node<LayoutNode>(the_layout_spec_arena);
            *layout.next_ptr = new_node;
            layout.next_ptr = &new_node->next_sibling;
            *new_node = LayoutNode{
                .type = LayoutNodeType::Leaf,
                .next_sibling = 0,
                .size = size,
                .margin = {4, 4},
                .leaf = {}};
            ++layout.active_container->child_count;
            break;
        }
        case PassType::Draw: {
            auto const& placement
                = *ctx.pass.layout_consumption.next_placement;
            ctx.pass.layout_consumption.next_placement = placement.next;
            Box box = {.pos = placement.position, .size = placement.size};
            draw_box(
                *ctx.pass.display_list_arena,
                *ctx.pass.box_command_list,
                box,
                color);
            break;
        }
        case PassType::Event: {
            auto const& placement
                = *ctx.pass.layout_consumption.next_placement;
            ctx.pass.layout_consumption.next_placement = placement.next;
            Box box = {.pos = placement.position, .size = placement.size};
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

/*bool
do_text(
    Context& ctx, Vec2 size, Color color, float scale, std::string_view text)
{
    switch (ctx.pass.type)
    {
        case PassType::Refresh: {
            auto& layout = ctx.pass.layout_emission;
            *layout.next = layout.count;
            LayoutNode* new_node = &layout.nodes[layout.count];
            *new_node = LayoutNode{
                .type = LayoutNodeType::Leaf,
                .next_sibling = 0,
                .size = size,
                .margin = {4, 4},
                .leaf = {}};
            layout.next = &new_node->next_sibling;
            ++layout.active_container->container.child_count;
            ++layout.count;
            break;
        }
        case PassType::Draw: {
            auto const& placement
                = ctx.pass.layout_consumption
                      .placements[ctx.pass.layout_consumption.index++];
            draw_text(
                the_msdf_text_engine,
                *ctx.pass.display_list_arena,
                the_msdf_commands,
                text.data(),
                text.size(),
                scale,
                placement.position,
                color);
            break;
        }
        case PassType::Event: {
            auto const& placement
                = ctx.pass.layout_consumption
                      .placements[ctx.pass.layout_consumption.index++];
            Box box = {.pos = placement.position, .size = placement.size};
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
}*/

void
rectangle_demo(Context& ctx)
{
    static bool invert = false;

    flow(ctx, [&]() {
        float x = 0.0f;
        for (int i = 0; i < 2000; ++i)
        {
            do_rect(
                ctx,
                {24, 24},
                invert ? Color{x, 0.1f, 1.0f - x, 1}
                       : Color{1.0f - x, 0.1f, x, 1});
            x += 0.0005f;
        }

        /*hbox(ctx, [&]() {
            for (int i = 0; i < 4; ++i)
            {
                if (do_text(ctx, {200, 30}, GRAY, 24, "HELLO!"))
                {
                    invert = !invert;
                }
            }
        });*/
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
                {nullptr},
                nullptr,
                nullptr,
                &event},
            &the_system};
        rectangle_demo(event_ctx);
    }
}

#if 0

void
do_text_demo()
{
    draw_text(the_msdf_text_engine, "abcdef - hello", 0, 14, 14, 256, 64, 320);

    float next_line = 400;

    for (int i = 0; i < 18; ++i)
    {
        next_line = draw_wrapped_text(
            the_msdf_text_engine,
            R"---(
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur rutrum nunc non ligula volutpat, quis ultricies enim viverra. Cras at pellentesque orci, eget aliquam sapien. Donec fringilla dui orci, vitae sodales purus blandit quis. Aenean porttitor varius erat, pulvinar tristique nibh faucibus at. In maximus, ex non fermentum pulvinar, lacus diam iaculis mi, ac sagittis neque nulla in risus. Mauris rutrum nibh vitae eros iaculis maximus. Curabitur nec lorem ac massa elementum suscipit. Fusce vel sagittis ipsum.
)---",
            40 - i * 2,
            12,
            next_line,
            the_system.framebuffer_size.x - 24);
    }

    for (int i = 0; i < 0; ++i)
    {
        next_line = draw_wrapped_text(
            the_msdf_text_engine,
            R"---(
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur rutrum nunc non ligula volutpat, quis ultricies enim viverra. Cras at pellentesque orci, eget aliquam sapien. Donec fringilla dui orci, vitae sodales purus blandit quis. Aenean porttitor varius erat, pulvinar tristique nibh faucibus at. In maximus, ex non fermentum pulvinar, lacus diam iaculis mi, ac sagittis neque nulla in risus. Mauris rutrum nibh vitae eros iaculis maximus. Curabitur nec lorem ac massa elementum suscipit. Fusce vel sagittis ipsum. Suspendisse porta imperdiet nisi at vehicula. Nulla a pharetra tortor. Ut non velit sollicitudin, aliquam mauris at, interdum tortor. Vestibulum et est aliquet, consequat massa nec, ullamcorper est. Etiam euismod felis leo. Praesent cursus sed risus eu eleifend. Quisque pharetra maximus gravida. Ut non ullamcorper arcu. Sed vulputate ullamcorper metus, sit amet tempor nulla consequat sit amet. Nulla facilisi. Nam quis purus vel tortor fringilla pellentesque ut sit amet metus.

Nam facilisis volutpat eros, euismod finibus erat ornare vitae. Nulla dictum arcu at nisl pretium, non hendrerit justo pulvinar. Donec pretium mi ornare odio iaculis semper. Curabitur ex odio, lacinia nec nulla vitae, porta posuere lacus. Quisque iaculis elementum ante, vel maximus ipsum. Sed eleifend auctor turpis sed porttitor. Donec dignissim luctus velit. Aenean vehicula purus et nisl viverra finibus. Phasellus molestie lacus massa, faucibus laoreet purus molestie quis. Phasellus nec maximus augue. Duis est lectus, pretium non lacus in, hendrerit sagittis elit. Vestibulum volutpat cursus orci, rhoncus vestibulum nisi tempor interdum. Curabitur sapien magna, luctus quis dictum a, pulvinar eu nisl. Mauris ut felis lorem. Nullam ullamcorper scelerisque ipsum eu tincidunt. Aenean malesuada eros nec tincidunt ultrices. Cras sed ipsum vel nulla ultrices vehicula. Ut dapibus, dolor convallis fringilla varius, ante ex placerat lacus, eget viverra orci purus ac ipsum. In ullamcorper commodo libero, et auctor leo lobortis quis. Suspendisse vitae rutrum neque. Etiam pharetra turpis nec elementum dapibus. Nunc tincidunt fermentum accumsan. Duis vestibulum enim arcu, eu rutrum magna cursus nec. Sed quis viverra enim. Morbi arcu dui, tristique ac imperdiet non, bibendum in nisl. Donec et neque porta, maximus urna vitae, dictum risus. Curabitur vel dapibus justo, eget gravida est. Fusce facilisis convallis tortor. Donec nulla massa, dignissim at metus quis, malesuada fermentum tortor. Nulla efficitur, purus et pellentesque tristique, nibh sem ultrices sapien, eget tincidunt dui erat nec nulla. Curabitur auctor metus eros, sit amet maximus arcu sodales eu. Suspendisse potenti. Mauris vitae quam volutpat, consequat massa vitae, iaculis enim. Phasellus at scelerisque mauris. Quisque placerat nibh in justo auctor, nec accumsan sem fermentum. Morbi pretium ante in eros ullamcorper condimentum. Nunc tincidunt fermentum accumsan. Duis vestibulum enim arcu, eu rutrum magna cursus nec. Sed quis viverra enim. Morbi arcu dui, tristique ac imperdiet non, bibendum in nisl. Donec et neque porta, maximus urna vitae, dictum risus. Curabitur vel dapibus justo, eget gravida est. Fusce facilisis convallis tortor. Donec nulla massa, dignissim at metus quis, malesuada fermentum tortor. Nulla efficitur, purus et pellentesque tristique, nibh sem ultrices sapien, eget tincidunt dui erat nec nulla. Curabitur auctor metus eros, sit amet maximus arcu sodales eu. Suspendisse potenti. Mauris vitae quam volutpat, consequat massa vitae, iaculis enim. Phasellus at scelerisque mauris. Quisque placerat nibh in justo auctor, nec accumsan sem fermentum. Morbi pretium ante in eros ullamcorper condimentum.

Phasellus molestie lacus massa, faucibus laoreet purus molestie quis. Phasellus nec maximus augue. Duis est lectus, pretium non lacus in, hendrerit sagittis elit. Vestibulum volutpat cursus orci, rhoncus vestibulum nisi tempor interdum. Curabitur sapien magna, luctus quis dictum a, pulvinar eu nisl. Mauris ut felis lorem. Nullam ullamcorper scelerisque ipsum eu tincidunt. Aenean malesuada eros nec tincidunt ultrices. Cras sed ipsum vel nulla ultrices vehicula. Ut dapibus, dolor convallis fringilla varius, ante ex placerat lacus, eget viverra orci purus ac ipsum. In ullamcorper commodo libero, et auctor leo lobortis quis. Suspendisse vitae rutrum neque. Etiam pharetra turpis nec elementum dapibus. Nunc tincidunt fermentum accumsan. Duis vestibulum enim arcu, eu rutrum magna cursus nec. Sed quis viverra enim. Morbi arcu dui, tristique ac imperdiet non, bibendum in nisl. Donec et neque porta, maximus urna vitae, dictum risus. Curabitur vel dapibus justo, eget gravida est. Fusce facilisis convallis tortor. Donec nulla massa, dignissim at metus quis, malesuada fermentum tortor. Nulla efficitur, purus et pellentesque tristique, nibh sem ultrices sapien, eget tincidunt dui erat nec nulla. Curabitur auctor metus eros, sit amet maximus arcu sodales eu. Suspendisse potenti. Mauris vitae quam volutpat, consequat massa vitae, iaculis enim. Phasellus at scelerisque mauris. Quisque placerat nibh in justo auctor, nec accumsan sem fermentum. Morbi pretium ante in eros ullamcorper condimentum. Nunc tincidunt fermentum accumsan. Duis vestibulum enim arcu, eu rutrum magna cursus nec. Sed quis viverra enim. Morbi arcu dui, tristique ac imperdiet non, bibendum in nisl. Donec et neque porta, maximus urna vitae, dictum risus. Curabitur vel dapibus justo, eget gravida est. Fusce facilisis convallis tortor. Donec nulla massa, dignissim at metus quis, malesuada fermentum tortor. Nulla efficitur, purus et pellentesque tristique, nibh sem ultrices sapien, eget tincidunt dui erat nec nulla. Curabitur auctor metus eros, sit amet maximus arcu sodales eu. Suspendisse potenti. Mauris vitae quam volutpat, consequat massa vitae, iaculis enim. Phasellus at scelerisque mauris. Quisque placerat nibh in justo auctor, nec accumsan sem fermentum. Morbi pretium ante in eros ullamcorper condimentum.

Etiam tempus fermentum dolor, ac sollicitudin leo posuere nec. Nunc euismod scelerisque ligula, ut ullamcorper lacus bibendum quis. Nullam vel pharetra ligula. Aenean hendrerit, eros et commodo mollis, elit libero blandit nisi, a porttitor tortor enim congue nunc. Maecenas viverra lacus tellus, quis pharetra quam malesuada ac. Quisque dapibus sollicitudin aliquet. Etiam a diam nec risus fringilla blandit. Ut eu pretium nibh. Nam facilisis volutpat eros, euismod finibus erat ornare vitae. Nulla dictum arcu at nisl pretium, non hendrerit justo pulvinar. Donec pretium mi ornare odio iaculis semper. Curabitur ex odio, lacinia nec nulla vitae, porta posuere lacus. Quisque iaculis elementum ante, vel maximus ipsum. Sed eleifend auctor turpis sed porttitor. Donec dignissim luctus velit. Aenean vehicula purus et nisl viverra finibus. Etiam tempus fermentum dolor, ac sollicitudin leo posuere nec. Nunc euismod scelerisque ligula, ut ullamcorper lacus bibendum quis. Nullam vel pharetra ligula. Aenean hendrerit, eros et commodo mollis, elit libero blandit nisi, a porttitor tortor enim congue nunc. Maecenas viverra lacus tellus, quis pharetra quam malesuada ac. Quisque dapibus sollicitudin aliquet. Etiam a diam nec risus fringilla blandit. Ut eu pretium nibh.

Nunc tincidunt fermentum accumsan. Duis vestibulum enim arcu, eu rutrum magna cursus nec. Sed quis viverra enim. Morbi arcu dui, tristique ac imperdiet non, bibendum in nisl. Donec et neque porta, maximus urna vitae, dictum risus. Curabitur vel dapibus justo, eget gravida est. Fusce facilisis convallis tortor. Donec nulla massa, dignissim at metus quis, malesuada fermentum tortor. Nulla efficitur, purus et pellentesque tristique, nibh sem ultrices sapien, eget tincidunt dui erat nec nulla. Curabitur auctor metus eros, sit amet maximus arcu sodales eu. Suspendisse potenti. Mauris vitae quam volutpat, consequat massa vitae, iaculis enim. Phasellus at scelerisque mauris. Quisque placerat nibh in justo auctor, nec accumsan sem fermentum. Morbi pretium ante in eros ullamcorper condimentum. Nunc tincidunt fermentum accumsan. Duis vestibulum enim arcu, eu rutrum magna cursus nec. Sed quis viverra enim. Morbi arcu dui, tristique ac imperdiet non, bibendum in nisl. Donec et neque porta, maximus urna vitae, dictum risus. Curabitur vel dapibus justo, eget gravida est. Fusce facilisis convallis tortor. Donec nulla massa, dignissim at metus quis, malesuada fermentum tortor. Nulla efficitur, purus et pellentesque tristique, nibh sem ultrices sapien, eget tincidunt dui erat nec nulla. Curabitur auctor metus eros, sit amet maximus arcu sodales eu. Suspendisse potenti. Mauris vitae quam volutpat, consequat massa vitae, iaculis enim. Phasellus at scelerisque mauris. Quisque placerat nibh in justo auctor, nec accumsan sem fermentum. Morbi pretium ante in eros ullamcorper condimentum. Nam facilisis volutpat eros, euismod finibus erat ornare vitae. Nulla dictum arcu at nisl pretium, non hendrerit justo pulvinar. Donec pretium mi ornare odio iaculis semper. Curabitur ex odio, lacinia nec nulla vitae, porta posuere lacus. Quisque iaculis elementum ante, vel maximus ipsum. Sed eleifend auctor turpis sed porttitor. Donec dignissim luctus velit. Aenean vehicula purus et nisl viverra finibus. Etiam tempus fermentum dolor, ac sollicitudin leo posuere nec. Nunc euismod scelerisque ligula, ut ullamcorper lacus bibendum quis. Nullam vel pharetra ligula. Aenean hendrerit, eros et commodo mollis, elit libero blandit nisi, a porttitor tortor enim congue nunc. Maecenas viverra lacus tellus, quis pharetra quam malesuada ac. Quisque dapibus sollicitudin aliquet. Etiam a diam nec risus fringilla blandit. Ut eu pretium nibh. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur rutrum nunc non ligula volutpat, quis ultricies enim viverra. Cras at pellentesque orci, eget aliquam sapien. Donec fringilla dui orci, vitae sodales purus blandit quis. Aenean porttitor varius erat, pulvinar tristique nibh faucibus at. In maximus, ex non fermentum pulvinar, lacus diam iaculis mi, ac sagittis neque nulla in risus. Mauris rutrum nibh vitae eros iaculis maximus. Curabitur nec lorem ac massa elementum suscipit. Fusce vel sagittis ipsum. Suspendisse porta imperdiet nisi at vehicula. Nulla a pharetra tortor. Ut non velit sollicitudin, aliquam mauris at, interdum tortor. Vestibulum et est aliquet, consequat massa nec, ullamcorper est. Etiam euismod felis leo. Praesent cursus sed risus eu eleifend. Quisque pharetra maximus gravida. Ut non ullamcorper arcu. Sed vulputate ullamcorper metus, sit amet tempor nulla consequat sit amet. Nulla facilisi. Nam quis purus vel tortor fringilla pellentesque ut sit amet metus.

Nam facilisis volutpat eros, euismod finibus erat ornare vitae. Nulla dictum arcu at nisl pretium, non hendrerit justo pulvinar. Donec pretium mi ornare odio iaculis semper. Curabitur ex odio, lacinia nec nulla vitae, porta posuere lacus. Quisque iaculis elementum ante, vel maximus ipsum. Sed eleifend auctor turpis sed porttitor. Donec dignissim luctus velit. Aenean vehicula purus et nisl viverra finibus. Etiam tempus fermentum dolor, ac sollicitudin leo posuere nec. Nunc euismod scelerisque ligula, ut ullamcorper lacus bibendum quis. Nullam vel pharetra ligula. Aenean hendrerit, eros et commodo mollis, elit libero blandit nisi, a porttitor tortor enim congue nunc. Maecenas viverra lacus tellus, quis pharetra quam malesuada ac. Quisque dapibus sollicitudin aliquet. Etiam a diam nec risus fringilla blandit. Ut eu pretium nibh. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur rutrum nunc non ligula volutpat, quis ultricies enim viverra. Cras at pellentesque orci, eget aliquam sapien. Donec fringilla dui orci, vitae sodales purus blandit quis. Aenean porttitor varius erat, pulvinar tristique nibh faucibus at. In maximus, ex non fermentum pulvinar, lacus diam iaculis mi, ac sagittis neque nulla in risus. Mauris rutrum nibh vitae eros iaculis maximus. Curabitur nec lorem ac massa elementum suscipit. Fusce vel sagittis ipsum. Suspendisse porta imperdiet nisi at vehicula. Nulla a pharetra tortor. Ut non velit sollicitudin, aliquam mauris at, interdum tortor. Vestibulum et est aliquet, consequat massa nec, ullamcorper est. Etiam euismod felis leo. Praesent cursus sed risus eu eleifend. Quisque pharetra maximus gravida. Ut non ullamcorper arcu. Sed vulputate ullamcorper metus, sit amet tempor nulla consequat sit amet. Nulla facilisi. Nam quis purus vel tortor fringilla pellentesque ut sit amet metus.

Phasellus molestie lacus massa, faucibus laoreet purus molestie quis. Phasellus nec maximus augue. Duis est lectus, pretium non lacus in, hendrerit sagittis elit. Vestibulum volutpat cursus orci, rhoncus vestibulum nisi tempor interdum. Curabitur sapien magna, luctus quis dictum a, pulvinar eu nisl. Mauris ut felis lorem. Nullam ullamcorper scelerisque ipsum eu tincidunt. Aenean malesuada eros nec tincidunt ultrices. Cras sed ipsum vel nulla ultrices vehicula. Ut dapibus, dolor convallis fringilla varius, ante ex placerat lacus, eget viverra orci purus ac ipsum. In ullamcorper commodo libero, et auctor leo lobortis quis. Suspendisse vitae rutrum neque. Etiam pharetra turpis nec elementum dapibus. Etiam tempus fermentum dolor, ac sollicitudin leo posuere nec. Nunc euismod scelerisque ligula, ut ullamcorper lacus bibendum quis. Nullam vel pharetra ligula. Aenean hendrerit, eros et commodo mollis, elit libero blandit nisi, a porttitor tortor enim congue nunc. Maecenas viverra lacus tellus, quis pharetra quam malesuada ac. Quisque dapibus sollicitudin aliquet. Etiam a diam nec risus fringilla blandit. Ut eu pretium nibh.

Etiam tempus fermentum dolor, ac sollicitudin leo posuere nec. Nunc euismod scelerisque ligula, ut ullamcorper lacus bibendum quis. Nullam vel pharetra ligula. Aenean hendrerit, eros et commodo mollis, elit libero blandit nisi, a porttitor tortor enim congue nunc. Maecenas viverra lacus tellus, quis pharetra quam malesuada ac. Quisque dapibus sollicitudin aliquet. Etiam a diam nec risus fringilla blandit. Ut eu pretium nibh. Phasellus molestie lacus massa, faucibus laoreet purus molestie quis. Phasellus nec maximus augue. Duis est lectus, pretium non lacus in, hendrerit sagittis elit. Vestibulum volutpat cursus orci, rhoncus vestibulum nisi tempor interdum. Curabitur sapien magna, luctus quis dictum a, pulvinar eu nisl. Mauris ut felis lorem. Nullam ullamcorper scelerisque ipsum eu tincidunt. Aenean malesuada eros nec tincidunt ultrices. Cras sed ipsum vel nulla ultrices vehicula. Ut dapibus, dolor convallis fringilla varius, ante ex placerat lacus, eget viverra orci purus ac ipsum. In ullamcorper commodo libero, et auctor leo lobortis quis. Suspendisse vitae rutrum neque. Etiam pharetra turpis nec elementum dapibus. Etiam tempus fermentum dolor, ac sollicitudin leo posuere nec. Nunc euismod scelerisque ligula, ut ullamcorper lacus bibendum quis. Nullam vel pharetra ligula. Aenean hendrerit, eros et commodo mollis, elit libero blandit nisi, a porttitor tortor enim congue nunc.

Nunc tincidunt fermentum accumsan. Duis vestibulum enim arcu, eu rutrum magna cursus nec. Sed quis viverra enim. Morbi arcu dui, tristique ac imperdiet non, bibendum in nisl. Donec et neque porta, maximus urna vitae, dictum risus. Curabitur vel dapibus justo, eget gravida est. Fusce facilisis convallis tortor. Donec nulla massa, dignissim at metus quis, malesuada fermentum tortor. Nulla efficitur, purus et pellentesque tristique, nibh sem ultrices sapien, eget tincidunt dui erat nec nulla. Curabitur auctor metus eros, sit amet maximus arcu sodales eu. Suspendisse potenti. Mauris vitae quam volutpat, consequat massa vitae, iaculis enim. Phasellus at scelerisque mauris. Quisque placerat nibh in justo auctor, nec accumsan sem fermentum. Morbi pretium ante in eros ullamcorper condimentum.)---",
            24,
            12,
            next_line,
            the_system.framebuffer_size.x - 24);
    }
}

#endif

void
update()
{
    static std::chrono::time_point<std::chrono::high_resolution_clock>
        last_frame_time = std::chrono::high_resolution_clock::now();
    auto const start_time = std::chrono::high_resolution_clock::now();

    std::uint32_t root_index;
    // Add a placeholder to fill the reserved/invalid 0 index.
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
        &the_system};
    *refresh_ctx.pass.layout_emission.next_ptr = 0;
    rectangle_demo(refresh_ctx);

    update_glfw_window_info(the_system, the_window);

    LayoutPlacement* initial_layout_placement = resolve_layout(
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
            {initial_layout_placement},
            &the_display_list_arena,
            &the_box_commands,
            nullptr},
        &the_system};
    rectangle_demo(draw_ctx);

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
