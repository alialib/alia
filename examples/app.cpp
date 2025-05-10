#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <chrono>
#include <iostream>

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

float atlas_w;
float atlas_h;

GLuint the_vao, the_vbo;

struct Vertex
{
    float x, y, u, v;
};

float
get_kerning(uint32_t left, uint32_t right)
{
    for (size_t i = 0; i < g_kerning_pair_count; ++i)
    {
        if (g_kerning_pairs[i].left == left
            && g_kerning_pairs[i].right == right)
        {
            return g_kerning_pairs[i].advance_adjustment;
        }
    }
    return 0.0f;
}

void
render_text(char const* text, float x, float y)
{
    for (char const* c = text; *c; ++c)
    {
        const Glyph& glyph = g_glyphs[*c - 32];
        assert(glyph.codepoint == *c);

        float vx0 = x + glyph.plane_left * 60;
        float vy0 = y - glyph.plane_bottom * 60;
        float vx1 = x + glyph.plane_right * 60;
        float vy1 = y - glyph.plane_top * 60;

        float uvx0 = glyph.atlas_left / atlas_w;
        float uvy0 = 1 - (glyph.atlas_bottom / atlas_h);
        float uvx1 = glyph.atlas_right / atlas_w;
        float uvy1 = 1 - (glyph.atlas_top / atlas_h);

        Vertex quad[6] = {
            {vx0, vy0, uvx0, uvy0},
            {vx1, vy0, uvx1, uvy0},
            {vx1, vy1, uvx1, uvy1},
            {vx0, vy0, uvx0, uvy0},
            {vx1, vy1, uvx1, uvy1},
            {vx0, vy1, uvx0, uvy1},
        };

        // Upload 6-vertex quad (two triangles)

        glBindVertexArray(the_vao);
        glBindBuffer(GL_ARRAY_BUFFER, the_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += glyph.advance * 60;

        x += get_kerning(*c, *(c + 1)) * 60;
    }
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
    rectangle_demo(draw_ctx);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    render_display_list(&the_renderer, the_system, the_display_list);

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    // glUseProgram(the_renderer.msdf_shader_program);
    glBindTexture(GL_TEXTURE_2D, the_renderer.msdf_texture);

    glUniform4f(the_renderer.msdf_color_location, 1, 1, 1, 1); // white

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

    glBindVertexArray(the_vao);
    glBindBuffer(GL_ARRAY_BUFFER, the_vbo);

    Vertex quad[6] = {
        {0, 0, 0, 0},
        {1, 0, 1, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 1, 0, 1},
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 6, quad, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Enable and set up vertex attributes
    // Attribute 0 = vec2 position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0);

    // Attribute 1 = vec2 UV
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (2 * sizeof(float)));

    glBindVertexArray(the_vao);
    render_text("The quick brown fox jumps over the lazy dog!", 200, 200);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    auto const end_time = std::chrono::high_resolution_clock::now();
    auto const delta_time = std::chrono::duration_cast<
        std::chrono::duration<int64_t, std::micro>>(end_time - start_time);

    std::cout << "frame_time: " << delta_time.count() << "us" << std::endl;

    while ((err = glGetError()) != GL_NO_ERROR)
        printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);

    glfwSwapBuffers(the_window);
    glfwPollEvents();

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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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

    glGenVertexArrays(1, &the_vao);
    glGenBuffers(1, &the_vbo);

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

    atlas_w = tex_width;
    atlas_h = tex_height;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(the_window))
    {
        update();

        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
            printf("GL ERROR: %x @ %s:%d\n", err, __FILE__, __LINE__);
    }

    glfwTerminate();
    return 0;
}
