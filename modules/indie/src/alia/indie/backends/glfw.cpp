#include <alia/indie/backends/glfw.hpp>

#include <GLFW/glfw3.h>

#pragma warning(push, 0)
// TODO: Prune these.
#include "include/core/SkCanvas.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkColorPriv.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkPath.h"
#include "include/core/SkRegion.h"
#include "include/core/SkShader.h"
#include "include/core/SkStream.h"
#include "include/core/SkSurface.h"
#include "include/core/SkTextBlob.h"
#include "include/core/SkTime.h"
#include "include/core/SkTypeface.h"
#include "include/effects/SkBlurMaskFilter.h"
#include "include/effects/SkGradientShader.h"
#include "include/gpu/GrBackendSurface.h"
#include "include/gpu/GrDirectContext.h"
#include "include/gpu/gl/GrGLInterface.h"
#include "include/utils/SkRandom.h"
#pragma warning(pop)

#include <alia/indie/events/input.hpp>
#include <alia/indie/layout/system.hpp>
#include <alia/indie/system/api.hpp>
#include <alia/indie/system/input_processing.hpp>
#include <alia/indie/system/object.hpp>
#include <alia/indie/system/os_interface.hpp>
#include <alia/indie/system/window_interface.hpp>
#include <alia/indie/widget.hpp>

#include <chrono>

namespace alia { namespace indie {

struct glfw_window_impl
{
    GLFWwindow* glfw_window = nullptr;

    // TODO: Bundle this up into a Skia structure?
    std::unique_ptr<GrDirectContext> skia_graphics_context;
    std::unique_ptr<SkSurface> skia_surface;

    // TODO: Does this go here?
    indie::system system;
};

struct glfw_os_interface : os_interface
{
    glfw_os_interface(glfw_window_impl& impl) : impl_(impl)
    {
    }

    void
    set_clipboard_text(std::string text) override
    {
        glfwSetClipboardString(impl_.glfw_window, text.c_str());
    }

    std::optional<std::string>
    get_clipboard_text() override
    {
        char const* text = glfwGetClipboardString(impl_.glfw_window);
        if (text)
            return std::string(text);
        else
            return std::nullopt;
    }

    glfw_window_impl& impl_;
};

struct glfw_window_interface : window_interface
{
    glfw_window_interface(glfw_window_impl& impl) : impl_(impl)
    {
    }

    void
    set_mouse_cursor(mouse_cursor cursor) override
    {
        GLFWcursor* glfw_cursor = nullptr;
        switch (cursor)
        {
            case mouse_cursor::CROSSHAIR:
                glfw_cursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
                break;
            case mouse_cursor::TEXT:
                glfw_cursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
                break;
            case mouse_cursor::POINTER:
                glfw_cursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
                break;
            case mouse_cursor::EW_RESIZE:
                glfw_cursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
                break;
            case mouse_cursor::NS_RESIZE:
                glfw_cursor = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
                break;
            default:
                break;
        }
        glfwSetCursor(impl_.glfw_window, glfw_cursor);
    }

    glfw_window_impl& impl_;
};

namespace {

void
error_callback(int /*error*/, const char* description)
{
    throw alia::exception(description);
}

struct glfw_global_init
{
    glfw_global_init()
    {
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
        {
            throw alia::exception("GLFW initialization failed");
        }
    }

    ~glfw_global_init()
    {
        glfwTerminate();
    }
};

glfw_window_impl&
get_impl(GLFWwindow* window)
{
    return *reinterpret_cast<glfw_window_impl*>(
        glfwGetWindowUserPointer(window));
}

system&
get_system(GLFWwindow* window)
{
    return get_impl(window).system;
}

void
key_event_callback(
    GLFWwindow* window, int key, int /*scancode*/, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key)
        {
            case GLFW_KEY_TAB:
                if (mods == GLFW_MOD_SHIFT)
                    regress_focus(get_system(window));
                else if (mods == 0)
                    advance_focus(get_system(window));
                break;
            default:
                process_key_press(
                    get_system(window),
                    modded_key{key_code(key), key_modifiers(mods)});
        }
    }
    else if (action == GLFW_RELEASE)
    {
        process_key_release(
            get_system(window),
            modded_key{key_code(key), key_modifiers(mods)});
    }
}

void
mouse_motion_callback(GLFWwindow* window, double x, double y)
{
    process_mouse_motion(get_system(window), make_vector(x, y));
}

void
mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/)
{
    switch (action)
    {
        case GLFW_PRESS:
            process_mouse_press(get_impl(window).system, mouse_button(button));
            break;
        case GLFW_RELEASE:
            process_mouse_release(
                get_impl(window).system, mouse_button(button));
            break;
    }
}

void
reset_skia(glfw_window_impl& impl, vector<2, unsigned> size)
{
    GrGLFramebufferInfo framebuffer_info;
    framebuffer_info.fFBOID = 0;
    framebuffer_info.fFormat = GL_RGBA8;

    GrBackendRenderTarget backend_render_target(
        size[0], size[1], 0, 0, framebuffer_info);

    impl.skia_surface.reset();
    auto surface = SkSurface::MakeFromBackendRenderTarget(
        impl.skia_graphics_context.get(),
        backend_render_target,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        nullptr,
        nullptr);
    if (!surface.get())
        throw alia::exception("Skia surface creation failed");
    impl.skia_surface.reset(surface.release());
}

void
init_skia(glfw_window_impl& impl, vector<2, unsigned> size)
{
    auto interface = GrGLMakeNativeInterface();
    impl.skia_graphics_context.reset(
        GrDirectContext::MakeGL(interface).release());
    reset_skia(impl, size);
}

void
render_ui(glfw_window_impl& impl)
{
    // TODO: Track this ourselves.
    int width, height;
    glfwGetFramebufferSize(impl.glfw_window, &width, &height);

    std::chrono::steady_clock::time_point begin
        = std::chrono::steady_clock::now();

    auto& canvas = *impl.skia_surface->getCanvas();
    canvas.clipRect(SkRect::MakeWH(SkScalar(width), SkScalar(height)));

    // TODO: Don't clear automatically.
    {
        SkPaint paint;
        paint.setColor(SK_ColorWHITE);
        canvas.drawPaint(paint);
    }

    if (impl.system.root_widget)
    {
        impl.system.root_widget->render(*impl.skia_surface->getCanvas());
    }
    else
    {
        // TODO: Clear the canvas?
    }

    {
        std::chrono::steady_clock::time_point end
            = std::chrono::steady_clock::now();
        std::cout << "render: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(
                         end - begin)
                         .count()
                  << "[us]\n";
    }

    impl.skia_graphics_context->flush();
    glfwSwapBuffers(impl.glfw_window);
}

void
update_ui(glfw_window_impl& impl)
{
    // TODO: Track this ourselves.
    int width, height;
    glfwGetFramebufferSize(impl.glfw_window, &width, &height);

    std::chrono::steady_clock::time_point begin
        = std::chrono::steady_clock::now();

    refresh_system(impl.system);
    update(impl.system);

    std::chrono::steady_clock::duration refresh_time;
    {
        std::chrono::steady_clock::time_point end
            = std::chrono::steady_clock::now();
        refresh_time = end - begin;
        begin = end;
    }

    resolve_layout(
        impl.system.layout, make_vector(float(width), float(height)));

    std::chrono::steady_clock::duration layout_time;
    {
        std::chrono::steady_clock::time_point end
            = std::chrono::steady_clock::now();
        layout_time = end - begin;
        begin = end;
    }

    std::cout << "refresh: "
              << std::chrono::duration_cast<std::chrono::microseconds>(
                     refresh_time)
                     .count()
              << "[us]\n";

    std::cout << "layout: "
              << std::chrono::duration_cast<std::chrono::microseconds>(
                     layout_time)
                     .count()
              << "[us]\n";

    render_ui(impl);
}

void
framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    auto& impl = get_impl(window);
    reset_skia(impl, make_vector<unsigned>(width, height));
    update_ui(impl);
}

void
init_window(
    glfw_window_impl& impl, std::string const& title, vector<2, unsigned> size)
{
    static glfw_global_init glfw_singleton;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);
    glfwWindowHint(GLFW_DEPTH_BITS, 0);

    GLFWwindow* window
        = glfwCreateWindow(size[0], size[1], title.c_str(), NULL, NULL);
    if (!window)
        throw alia::exception("GLFW window creation failed");
    glfwSetWindowUserPointer(window, &impl);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, mouse_motion_callback);

    glfwSetKeyCallback(window, key_event_callback);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // TODO: Do this elsewhere?
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    impl.glfw_window = window;
}

void
destroy_window(glfw_window_impl& impl)
{
    glfwDestroyWindow(impl.glfw_window);
}

} // namespace

glfw_window::glfw_window(
    std::string const& title,
    vector<2, unsigned> size,
    std::function<void(indie::context)> controller)
    : impl_(new glfw_window_impl)
{
    initialize(
        impl_->system,
        controller,
        std::make_shared<glfw_os_interface>(*impl_),
        std::make_shared<glfw_window_interface>(*impl_));

    init_window(*impl_, title, size);

    // TODO: Do this in a better way.
    int width, height;
    glfwGetFramebufferSize(impl_->glfw_window, &width, &height);
    impl_->system.surface_size[0] = width;
    impl_->system.surface_size[1] = height;

    init_skia(*impl_, size);

    // Perform the initial update.
    refresh_system(impl_->system);
}

void*
glfw_window::handle() const
{
    return impl_->glfw_window;
}

void
glfw_window::do_main_loop()
{
    while (!glfwWindowShouldClose(impl_->glfw_window))
    {
        update_ui(*impl_);

        glfwPollEvents();
    }
}

glfw_window::~glfw_window()
{
    if (impl_)
    {
        impl_->skia_surface.reset();
        impl_->skia_graphics_context.reset();

        destroy_window(*impl_);
    }
}

}} // namespace alia::indie
