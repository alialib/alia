#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <chrono>
#include <iostream>

#include <alia/foundation/arena.hpp>
#include <alia/platforms/glfw/window.hpp>
#include <alia/renderers/gl/renderer.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/display_list.hpp>
#include <alia/ui/drawing.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/geometry.hpp>
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

void
rectangle_demo(Context ctx)
{
    static bool is_red = false;

    // box 1: dynamic color
    if (ctx.pass.type == PassType::Draw)
    {
        draw_box(ctx.pass.display_list, {10, 10, 96, 96}, is_red ? RED : BLUE);
    }

    // box 2: button
    if (ctx.pass.type == PassType::Event)
    {
        if (detect_click(ctx.pass.event, 10, 120, 96, 96))
            is_red = !is_red;
    }
    if (ctx.pass.type == PassType::Draw)
    {
        draw_box(ctx.pass.display_list, {10, 120, 96, 96}, GRAY);
    }
}

System the_system;

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
        rectangle_demo(
            Context{Pass{PassType::Event, nullptr, &event}, &the_system});
    }
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

    GLFWwindow* window
        = glfwCreateWindow(800, 600, "Alia Renderer", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    GlRenderer renderer;
    init_gl_renderer(&renderer);

    ArenaAllocator alloc{default_alloc, default_dealloc, nullptr};
    Arena* display_list_arena = create_arena(alloc, sizeof(DrawCommand) * 256);
    DisplayList display_list = create_display_list(display_list_arena);

    while (!glfwWindowShouldClose(window))
    {
        auto const start_time = std::chrono::high_resolution_clock::now();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        update_glfw_window_info(the_system, window);

        reset_display_list(&display_list);
        rectangle_demo(Context{
            Pass{PassType::Draw, &display_list, nullptr}, &the_system});

        render_display_list(&renderer, the_system, display_list);

        auto const end_time = std::chrono::high_resolution_clock::now();
        auto const delta_time = std::chrono::duration_cast<
            std::chrono::duration<int64_t, std::micro>>(end_time - start_time);

        std::cout << "frame_time: " << delta_time.count() << "us" << std::endl;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
