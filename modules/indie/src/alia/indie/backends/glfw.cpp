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

#include <alia/indie/system/object.hpp>

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

    impl.glfw_window_
        = glfwCreateWindow(size[0], size[1], title.c_str(), NULL, NULL);
    if (!impl.glfw_window_)
        throw alia::exception("GLFW window creation failed");
    glfwMakeContextCurrent(impl.glfw_window_);
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
    refresh_system(impl_->system.alia_system);
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
        render(impl_->system, *impl_->skia_surface_->getCanvas());
        glfwSwapBuffers(impl_->glfw_window_);
        glfwWaitEvents();
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
