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

#include <alia/indie/layout/system.hpp>
#include <alia/indie/system/object.hpp>
#include <alia/indie/widget.hpp>

#include <chrono>

namespace alia { namespace indie {

struct glfw_window_impl
{
    GLFWwindow* glfw_window_ = nullptr;

    // TODO: Bundle this up into a Skia structure?
    std::unique_ptr<GrDirectContext> skia_graphics_context_;
    std::unique_ptr<SkSurface> skia_surface_;

    // TODO: Does this go here?
    indie::system system;
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

void
cursor_position_callback(GLFWwindow*, double, double)
{
}

void
mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        mouse_click_event e;
        dispatch_event(get_impl(window).system, e);
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

    impl.skia_surface_.reset();
    auto surface = SkSurface::MakeFromBackendRenderTarget(
        impl.skia_graphics_context_.get(),
        backend_render_target,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        nullptr,
        nullptr);
    if (!surface.get())
        throw alia::exception("Skia surface creation failed");
    impl.skia_surface_.reset(surface.release());
}

void
init_skia(glfw_window_impl& impl, vector<2, unsigned> size)
{
    auto interface = GrGLMakeNativeInterface();
    impl.skia_graphics_context_.reset(
        GrDirectContext::MakeGL(interface).release());
    reset_skia(impl, size);
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
    glfwSetCursorPosCallback(window, cursor_position_callback);

    // TODO: Do this elsewhere?
    glfwMakeContextCurrent(window);

    impl.glfw_window_ = window;
}

void
destroy_window(glfw_window_impl& impl)
{
    glfwDestroyWindow(impl.glfw_window_);
}

} // namespace

glfw_window::glfw_window(
    std::string const& title,
    vector<2, unsigned> size,
    std::function<void(indie::context)> controller)
    : impl_(new glfw_window_impl)
{
    initialize(impl_->system, controller);

    init_window(*impl_, title, size);

    init_skia(*impl_, size);

    // Perform the initial update.
    refresh_system(impl_->system);
}

void*
glfw_window::handle() const
{
    return impl_->glfw_window_;
}

void
glfw_window::do_main_loop()
{
    while (!glfwWindowShouldClose(impl_->glfw_window_))
    {
        // TODO: Track this ourselves.
        int width, height;
        glfwGetWindowSize(impl_->glfw_window_, &width, &height);

        std::chrono::steady_clock::time_point begin
            = std::chrono::steady_clock::now();

        refresh_system(impl_->system);

        {
            std::chrono::steady_clock::time_point end
                = std::chrono::steady_clock::now();
            std::cout << "refresh: "
                      << std::chrono::duration_cast<std::chrono::microseconds>(
                             end - begin)
                             .count()
                      << "[us]\n";
            begin = end;
        }

        resolve_layout(impl_->system.layout, make_vector(width, height));

        {
            std::chrono::steady_clock::time_point end
                = std::chrono::steady_clock::now();
            std::cout << "layout: "
                      << std::chrono::duration_cast<std::chrono::microseconds>(
                             end - begin)
                             .count()
                      << "[us]\n";
            begin = end;
        }

        auto& canvas = *impl_->skia_surface_->getCanvas();
        canvas.clipRect(SkRect::MakeWH(SkScalar(width), SkScalar(height)));

        // TODO: Don't clear automatically.
        {
            SkPaint paint;
            paint.setColor(SK_ColorWHITE);
            canvas.drawPaint(paint);
        }

        if (impl_->system.root_widget)
        {
            impl_->system.root_widget->render(
                *impl_->skia_surface_->getCanvas());
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
            begin = end;
        }

        impl_->skia_graphics_context_->flush();
        glfwSwapBuffers(impl_->glfw_window_);

        glfwPollEvents();
    }
}

glfw_window::~glfw_window()
{
    if (impl_)
    {
        impl_->skia_surface_.reset();
        impl_->skia_graphics_context_.reset();

        destroy_window(*impl_);
    }
}

}} // namespace alia::indie
