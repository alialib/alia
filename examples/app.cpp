#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <chrono>
#include <functional> // For std::hash
#include <iostream>
#include <unordered_map>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "roboto-msdf.h"

#include <alia/foundation/arena.hpp>
#include <alia/platforms/glfw/window.hpp>
#include <alia/renderers/gl/renderer.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/display_list.hpp>
#include <alia/ui/drawing.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout.hpp>
#include <alia/ui/system.hpp>

using namespace alia;

// TODO: Move this to foundation.

static void*
default_alloc(void*, size_t size, size_t alignment)
{
    void* ptr = nullptr;
#ifdef _MSC_VER
    ptr = _aligned_malloc(size, alignment);
#else
    posix_memalign(&ptr, alignment, size);
#endif
    return ptr;
}

static void
default_dealloc(void*, void* ptr)
{
#ifdef _MSC_VER
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

bool
detect_click(Event* event, float x, float y, float width, float height)
{
    return event->type == EventType::Click && event->click.x >= x
        && event->click.x <= x + width && event->click.y >= y
        && event->click.y <= y + height;
}

bool
do_rect(Context& ctx, Vec2 size, Color color)
{
    switch (ctx.pass.type)
    {
        case PassType::Refresh: {
            ctx.pass.layout_emission.specs[ctx.pass.layout_emission.count++]
                = LayoutSpec{.size = size, .margin = {4, 4}};
            break;
        }
        case PassType::Draw: {
            auto const& placement
                = ctx.pass.layout_consumption
                      .placements[ctx.pass.layout_consumption.index++];
            Box box = {.pos = placement.position, .size = placement.size};
            draw_box(ctx.pass.display_list, box, color);
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
}

void
rectangle_demo(Context& ctx)
{
    static bool invert = false;

    // box 1: dynamic color
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

    // box 2: button
    for (int i = 0; i < 400; ++i)
    {
        if (do_rect(ctx, {24, 24}, GRAY))
        {
            invert = !invert;
        }
    }
}

System the_system;
LayoutSpec the_layout_specs[4096];
LayoutPlacement the_layout_placements[4096];
GLFWwindow* the_window;
GlRenderer the_renderer;
DisplayList the_display_list;

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
        Context event_ctx
            = {Pass{
                   PassType::Event,
                   {the_layout_specs, 0},
                   {the_layout_placements, 0},
                   nullptr,
                   &event},
               &the_system};
        rectangle_demo(event_ctx);
    }
}

GLuint the_vao, the_vbo;

struct Vertex
{
    float x, y, u, v;
};

Vertex* the_vertex_storage;
Vertex* the_vertices = nullptr;
GLuint the_vertex_count = 0;

struct pair_hash
{
    std::size_t
    operator()(const std::pair<uint32_t, uint32_t>& pair) const
    {
        return (pair.first << 4) ^ pair.second;
    }
};

std::unordered_map<std::pair<uint32_t, uint32_t>, float, pair_hash>
    the_kerning_map;

float
get_kerning(uint32_t left, uint32_t right)
{
    auto it = the_kerning_map.find(std::make_pair(left, right));
    if (it != the_kerning_map.end())
    {
        return it->second;
    }
    return 0.0f;
}

float
render_text(
    char const* text,
    size_t start,
    size_t end,
    size_t buffer_length,
    float scale,
    float x,
    float y)
{
    if (x > the_system.framebuffer_size.x + scale * 2
        || y > the_system.framebuffer_size.y + scale * 2)
    {
        return x;
    }

    for (size_t i = start; i < end; ++i)
    {
        char const c = text[i];
        if (c < 32)
            continue;

        const Glyph& glyph = g_glyphs[c - 32];
        assert(glyph.codepoint == c);

        float vx0 = x + glyph.plane_left * scale;
        float vy0 = y - glyph.plane_bottom * scale;
        float vx1 = x + glyph.plane_right * scale;
        float vy1 = y - glyph.plane_top * scale;

        float uvx0 = glyph.atlas_left;
        float uvy0 = 1 - glyph.atlas_bottom;
        float uvx1 = glyph.atlas_right;
        float uvy1 = 1 - glyph.atlas_top;

        the_vertices[the_vertex_count] = {vx0, vy0, uvx0, uvy0};
        the_vertices[the_vertex_count + 1] = {vx1, vy0, uvx1, uvy0};
        the_vertices[the_vertex_count + 2] = {vx1, vy1, uvx1, uvy1};
        the_vertices[the_vertex_count + 3] = {vx0, vy0, uvx0, uvy0};
        the_vertices[the_vertex_count + 4] = {vx1, vy1, uvx1, uvy1};
        the_vertices[the_vertex_count + 5] = {vx0, vy1, uvx0, uvy1};

        the_vertex_count += 6;

        x += glyph.advance * scale;

        // if (i + 1 < buffer_length)
        x += get_kerning(c, text[i + 1]) * scale;
    }
    return x;
}

size_t
break_text(
    char const* text,
    size_t start,
    size_t end,
    size_t buffer_length,
    float scale,
    float width)
{
    size_t last_space = 0;
    float x = 0;
    for (size_t i = start; i < end; ++i)
    {
        char const c = text[i];

        switch (c)
        {
            case '\r':
                continue;
            case '\n':
                return i + 1;
            case ' ':
                last_space = i;
                break;
        }

        const Glyph& glyph = g_glyphs[c - 32];
        assert(glyph.codepoint == c);

        x += (glyph.advance + get_kerning(c, text[i + 1])) * scale;
        if (x > width)
        {
            return last_space == start ? i : last_space + 1;
        }
    }
    return end;
}

float
render_wrapped_text(
    char const* text, float scale, float x, float y, float width)
{
    size_t length = strlen(text);
    size_t start = 0;
    while (start < length)
    {
        size_t end = break_text(text, start, length, length, scale, width);
        render_text(text, start, end, length, scale, x, y);
        y += scale * 1.2f;
        start = end;
    }
    return y;
}

void
update()
{
    auto const start_time = std::chrono::high_resolution_clock::now();

    Context refresh_ctx
        = {Pass{
               PassType::Refresh,
               {the_layout_specs, 0},
               {the_layout_placements, 0},
               nullptr,
               nullptr},
           &the_system};
    rectangle_demo(refresh_ctx);

    update_glfw_window_info(the_system, the_window);

    // glfwMakeContextCurrent(the_window);
    glViewport(
        0, 0, the_system.framebuffer_size.x, the_system.framebuffer_size.y);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    layout(
        the_layout_specs,
        the_layout_placements,
        refresh_ctx.pass.layout_emission.count,
        the_system.framebuffer_size);

    reset_display_list(&the_display_list);
    Context draw_ctx
        = {Pass{
               PassType::Draw,
               {the_layout_specs, 0},
               {the_layout_placements, 0},
               &the_display_list,
               nullptr},
           &the_system};
    // rectangle_demo(draw_ctx);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    render_display_list(&the_renderer, the_system, the_display_list);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    // glUseProgram(the_renderer.msdf_shader_program);
    glBindTexture(GL_TEXTURE_2D, the_renderer.msdf_texture);

    glUniform4f(the_renderer.msdf_color_location, 0.9, 0.9, 0.9, 1);

    // RectInstance rect_instance{Vec2{0, 0}, Vec2{1, 1}};

    // glBindBuffer(GL_ARRAY_BUFFER, the_renderer.instance_vbo);
    // glBufferData(
    //     GL_ARRAY_BUFFER, sizeof(RectInstance), &rect_instance,
    //     GL_STATIC_DRAW);

    // glBindVertexArray(the_renderer.vao);
    // glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);

    // glUseProgram(the_renderer.msdf_shader_program);
    // glBindTexture(GL_TEXTURE_2D, the_renderer.msdf_texture);
    // glUniform4f(the_renderer.msdf_color_location, 1, 1, 1, 1); // white

    // float quad_vertices[] = {0.0f, 0.0f, 1.0f, 0.0f,
    // 0.0f, 1.0f, 1.0f, 1.0f};

    // glBindVertexArray(the_renderer.vao);
    // glDrawArrays(GL_TRIANGLES, 0, 6);

    the_vertices = the_vertex_storage;
    the_vertex_count = 0;

    float next_line = 64;

    for (int i = 0; i != 4; ++i)
    {
        next_line = render_wrapped_text(
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

    glBindVertexArray(the_vao);
    glBindBuffer(GL_ARRAY_BUFFER, the_vbo);
    // glBufferSubData(
    //     GL_ARRAY_BUFFER, 0, sizeof(Vertex) * the_vertex_count,
    //     the_vertices);
    glDrawArrays(GL_TRIANGLES, 0, the_vertex_count);

    printf("the_vertex_count: %d\n", the_vertex_count);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    auto const end_time = std::chrono::high_resolution_clock::now();
    auto const delta_time = std::chrono::duration_cast<
        std::chrono::duration<int64_t, std::micro>>(end_time - start_time);

    std::cout << "frame_time: " << delta_time.count() << "us" << std::endl;

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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    the_window = glfwCreateWindow(800, 600, "Alia Renderer", nullptr, nullptr);
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

    init_gl_renderer(&the_renderer);

    ArenaAllocator alloc{default_alloc, default_dealloc, nullptr};
    Arena* display_list_arena
        = create_arena(alloc, sizeof(DrawCommand) * 4096);
    the_display_list = create_display_list(display_list_arena);

    int tex_width, tex_height, channels;
    unsigned char* msdf_data
        = stbi_load("roboto-msdf.png", &tex_width, &tex_height, &channels, 3);
    if (!msdf_data)
    {
        std::cerr << "load failed: " << stbi_failure_reason() << std::endl;
        exit(1);
    }

    // the_vertex_storage = (Vertex*) malloc(sizeof(Vertex) * 240'000);

    glGenVertexArrays(1, &the_vao);
    glGenBuffers(1, &the_vbo);

    glBindVertexArray(the_vao);
    glBindBuffer(GL_ARRAY_BUFFER, the_vbo);

    const GLsizeiptr buffer_size = sizeof(Vertex) * 240'000;

    const GLbitfield flags
        = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

    glBufferStorage(GL_ARRAY_BUFFER, buffer_size, nullptr, flags);

    the_vertex_storage
        = (Vertex*) glMapBufferRange(GL_ARRAY_BUFFER, 0, buffer_size, flags);

    // Enable and set up vertex attributes
    // Attribute 0 = vec2 position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0);

    // Attribute 1 = vec2 UV
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (2 * sizeof(float)));

    // glBufferData(
    //     GL_ARRAY_BUFFER,
    //     sizeof(Vertex) * 240'000,
    //     nullptr, // calloc(240'000, sizeof(Vertex)),
    //     GL_DYNAMIC_DRAW);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glGenTextures(1, &the_renderer.msdf_texture);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glBindTexture(GL_TEXTURE_2D, the_renderer.msdf_texture);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB8,
        tex_width,
        tex_height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        msdf_data);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(msdf_data);

    for (size_t i = 0; i < g_glyph_count; ++i)
    {
        g_glyphs[i].atlas_left /= float(tex_height);
        g_glyphs[i].atlas_right /= float(tex_height);
        g_glyphs[i].atlas_top /= float(tex_height);
        g_glyphs[i].atlas_bottom /= float(tex_height);
    }

    for (size_t i = 0; i < g_kerning_pair_count; ++i)
    {
        the_kerning_map[std::make_pair(
            g_kerning_pairs[i].left, g_kerning_pairs[i].right)]
            = g_kerning_pairs[i].advance_adjustment;
    }

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
