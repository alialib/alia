#include <alia/ui/backends/emscripten.hpp>

#include <emscripten.h>
#include <emscripten/html5.h>

#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkShader.h"
#include "include/core/SkSurface.h"

#include "include/gpu/GrBackendSurface.h"
#include "include/gpu/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/gl/GrGLInterface.h"
#include "include/gpu/gl/GrGLTypes.h"

#include <GLES2/gl2.h>

#include <optional>

#if defined(_WIN32) && defined(SK_USING_THIRD_PARTY_ICU)
bool
SkLoadICU();
#else
inline bool
SkLoadICU()
{
    return true;
}
#endif // defined(_WIN32) && defined(SK_USING_THIRD_PARTY_ICU)

#include "modules/skparagraph/include/FontCollection.h"

#include <alia/ui/events.hpp>
#include <alia/ui/layout/system.hpp>
#include <alia/ui/system/api.hpp>
#include <alia/ui/system/input_processing.hpp>
#include <alia/ui/system/object.hpp>
#include <alia/ui/system/os_interface.hpp>
#include <alia/ui/system/window_interface.hpp>
#include <alia/ui/utilities/rendering.hpp>

#include <chrono>

namespace alia {

struct emscripten_canvas_impl
{
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webgl_context;

    // TODO: Bundle this up into a Skia structure?
    std::unique_ptr<GrDirectContext> skia_graphics_context;
    std::unique_ptr<SkSurface> skia_surface;

    // TODO: Does this go here?
    ui_system system;
};

void
update_ui(emscripten_canvas_impl& impl);

struct emscripten_os_interface : os_interface
{
    emscripten_os_interface(emscripten_canvas_impl& impl) : impl_(impl)
    {
    }

    void
    set_clipboard_text(std::string text) override
    {
        // TODO
    }

    std::optional<std::string>
    get_clipboard_text() override
    {
        return std::nullopt;
    }

    emscripten_canvas_impl& impl_;
};

struct emscripten_canvas_interface : window_interface
{
    emscripten_canvas_interface(emscripten_canvas_impl& impl) : impl_(impl)
    {
    }

    void
    set_mouse_cursor(mouse_cursor cursor) override
    {
        // TODO
    }

    emscripten_canvas_impl& impl_;
};

void
error_callback(int /*error*/, const char* description)
{
    std::cout << description << std::endl;
    throw alia::exception(description);
}

// void
// key_event_callback(
//     GLFWwindow* window, int key, int /*scancode*/, int action, int mods)
// {
//     if (action == GLFW_PRESS || action == GLFW_REPEAT)
//     {
//         switch (key)
//         {
//             case GLFW_KEY_TAB:
//                 if (mods == GLFW_MOD_SHIFT)
//                     regress_focus(get_system(window));
//                 else if (mods == 0)
//                     advance_focus(get_system(window));
//                 break;
//             default:
//                 process_key_press(
//                     get_system(window),
//                     modded_key{key_code(key), key_modifiers(mods)});
//         }
//     }
//     else if (action == GLFW_RELEASE)
//     {
//         process_key_release(
//             get_system(window),
//             modded_key{key_code(key), key_modifiers(mods)});
//     }
// }

emscripten_canvas_impl&
get_impl(void* user_data)
{
    return *reinterpret_cast<emscripten_canvas_impl*>(user_data);
}

bool
mouse_event_callback(
    int event_type, EmscriptenMouseEvent const* event, void* user_data)
{
    switch (event_type)
    {
        case EMSCRIPTEN_EVENT_MOUSEMOVE:
            process_mouse_motion(
                get_impl(user_data).system,
                make_vector<double>(event->targetX, event->targetY));
            update_ui(get_impl(user_data));
            return true;
        case EMSCRIPTEN_EVENT_MOUSEDOWN:
            process_mouse_motion(
                get_impl(user_data).system,
                make_vector<double>(event->targetX, event->targetY));
            process_mouse_press(
                get_impl(user_data).system, mouse_button(event->button));
            update_ui(get_impl(user_data));
            break;
        case EMSCRIPTEN_EVENT_MOUSEUP:
            process_mouse_release(
                get_impl(user_data).system, mouse_button(event->button));
            update_ui(get_impl(user_data));
            break;
        case EMSCRIPTEN_EVENT_DBLCLICK:
            // TODO
            break;
    }
    return false;
}

// void
// mouse_button_callback(GLFWwindow* window, int button, int action, int
// /*mods*/)
// {
//     switch (action)
//     {
//     }
// }

// void
// scroll_callback(GLFWwindow* window, double x, double y)
// {
//     process_scroll(get_impl(window).system, make_vector(x, y));
// }

void
reset_skia(emscripten_canvas_impl& impl, vector<2, unsigned> size)
{
    GrGLFramebufferInfo framebuffer_info;
    framebuffer_info.fFBOID = 0;
    framebuffer_info.fFormat = 0x8058;

    auto backend_render_target = GrBackendRenderTargets::MakeGL(
        size[0], size[1], 0, 0, framebuffer_info);

    impl.skia_surface.reset();
    auto surface = SkSurfaces::WrapBackendRenderTarget(
        impl.skia_graphics_context.get(),
        backend_render_target,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        nullptr,
        nullptr);
    if (!surface.get())
    {
        std::cout << "Skia surface creation failed" << std::endl;
        throw alia::exception("Skia surface creation failed");
    }
    impl.skia_surface.reset(surface.release());
}

void
init_skia(emscripten_canvas_impl& impl, vector<2, unsigned> size)
{
    // static bool globally_initialized = false;
    // if (!globally_initialized)
    // {
    //     if (!SkLoadICU())
    //     {
    //         std::cout << "SkLoadICU failed" << std::endl;
    //         throw alia::exception("SkLoadICU failed");
    //     }
    // }

    // impl.font_collection = sk_make_sp<skia::textlayout::FontCollection>();
    // impl.font_collection->setDefaultFontManager(SkFontMgr::RefDefault());

    impl.skia_graphics_context.reset(
        GrDirectContexts::MakeGL(nullptr, GrContextOptions()).release());
    reset_skia(impl, size);
}

void
render_ui(emscripten_canvas_impl& impl)
{
    // TODO: Track this ourselves.
    int width, height;
    emscripten_get_canvas_element_size("canvas", &width, &height);

    std::chrono::steady_clock::time_point begin
        = std::chrono::steady_clock::now();

    auto& canvas = *impl.skia_surface->getCanvas();
    canvas.resetMatrix();
    canvas.clipRect(SkRect::MakeWH(SkScalar(width), SkScalar(height)));

    // TODO: Don't clear automatically.
    {
        SkPaint paint;
        // paint.setColor(SK_ColorWHITE);
        paint.setColor(SkColorSetRGB(0x31, 0x38, 0x44));
        canvas.drawPaint(paint);
    }

    {
        render_event event;
        event.canvas = impl.skia_surface->getCanvas();
        dispatch_event(impl.system, event, RENDER_EVENT);
    }

    {
        std::chrono::steady_clock::time_point end
            = std::chrono::steady_clock::now();
        auto render_time
            = std::chrono::duration_cast<std::chrono::microseconds>(
                  end - begin)
                  .count();
        static long long max_render_time = 0;
        max_render_time = (std::max)(render_time, max_render_time);
        // std::cout << "render: " << render_time << "[us]\n";
        // std::cout << "max_render_time: " << max_render_time << "[us]\n";
    }

    impl.skia_graphics_context->flush();
    // glfwSwapBuffers(impl.webgl_context);
}

void
update_ui(emscripten_canvas_impl& impl)
{
    // TODO: Track this ourselves.
    int width, height;
    emscripten_get_canvas_element_size("canvas", &width, &height);

    refresh_system(impl.system);
    update(impl.system);

    invoke_ready_callbacks(
        impl.system.scheduler,
        impl.system.tick_count,
        [&](std::function<void()> const& callback, millisecond_count) {
            callback();
            refresh_system(impl.system);
            update(impl.system);
        });

    std::chrono::steady_clock::time_point begin
        = std::chrono::steady_clock::now();

    resolve_layout(
        impl.system.layout, make_vector(float(width), float(height)));

    long long layout_time;
    {
        std::chrono::steady_clock::time_point end
            = std::chrono::steady_clock::now();
        layout_time = std::chrono::duration_cast<std::chrono::microseconds>(
                          end - begin)
                          .count();
    }

    static long long max_layout_time = 0;
    max_layout_time = (std::max)(layout_time, max_layout_time);
    // std::cout << "layout: " << layout_time << "[us]\n";
    // std::cout << "max_layout_time: " << max_layout_time << "[us]\n";

    render_ui(impl);

    // int resource_count;
    // size_t resource_bytes;
    // impl.skia_graphics_context->getResourceCacheUsage(
    //     &resource_count, &resource_bytes);
    // std::cout << "resource_count: " << resource_count << "\n";
    // std::cout << "resource_bytes: " << resource_bytes << "\n";
}

bool
framebuffer_size_callback(
    int event_type, EmscriptenUiEvent const* uiEvent, void* user_data)
{
    std::cout << "framebuffer_size_callback" << std::endl;
    auto& impl = get_impl(user_data);
    int width, height;
    emscripten_get_canvas_element_size("canvas", &width, &height);
    impl.system.surface_size = make_vector<unsigned>(width, height);
    reset_skia(impl, impl.system.surface_size);
    update_ui(impl);
    return true;
}

bool
fullscreen_change_callback(
    int event_type,
    EmscriptenFullscreenChangeEvent const* uiEvent,
    void* user_data)
{
    std::cout << "fullscreen_change_callback" << std::endl;
    auto& impl = get_impl(user_data);
    int width, height;
    emscripten_get_canvas_element_size("canvas", &width, &height);
    impl.system.surface_size = make_vector<unsigned>(width, height);
    reset_skia(impl, impl.system.surface_size);
    update_ui(impl);
    return true;
}

void
init_canvas(emscripten_canvas_impl& impl)
{
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = 0;
    attrs.majorVersion = 2;
    impl.webgl_context = emscripten_webgl_create_context("canvas", &attrs);
    emscripten_webgl_make_context_current(impl.webgl_context);
}

emscripten_canvas::emscripten_canvas(
    std::function<void(ui_context)> controller)
    : impl_(new emscripten_canvas_impl)
{
    emscripten_set_canvas_element_size("canvas", 1200, 1600);

    initialize(
        impl_->system,
        controller,
        std::make_shared<emscripten_os_interface>(*impl_),
        std::make_shared<emscripten_canvas_interface>(*impl_));

    init_canvas(*impl_);

    // TODO: Do this in a better way.
    int width, height;
    emscripten_get_canvas_element_size("canvas", &width, &height);
    impl_->system.surface_size = make_vector<unsigned>(width, height);

    init_skia(*impl_, impl_->system.surface_size);

    // TODO: Not this.
    // impl_->system.font_collection = impl_->font_collection;

    emscripten_set_resize_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,
        impl_.get(),
        false,
        framebuffer_size_callback);
    emscripten_set_fullscreenchange_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,
        impl_.get(),
        false,
        fullscreen_change_callback);
    emscripten_set_mousemove_callback(
        "canvas", impl_.get(), false, mouse_event_callback);
    emscripten_set_mousedown_callback(
        "canvas", impl_.get(), false, mouse_event_callback);
    emscripten_set_mouseup_callback(
        "canvas", impl_.get(), false, mouse_event_callback);

    // Perform the initial update.
    update_ui(*impl_);
}

bool
do_main_loop_iteration(emscripten_canvas_impl& impl)
{
    update_ui(impl);

    // SDL_Event event;
    // while (SDL_PollEvent(&event))
    // {
    //     switch (event.type)
    //     {
    //         case SDL_WINDOWEVENT:
    //             switch (event.window.event)
    //             {
    //                 case SDL_WINDOWEVENT_CLOSE:
    //                     return false;

    //                 default:
    //                     break;
    //             }
    //             break;

    //         case SDL_MOUSEMOTION:
    //             process_mouse_motion(
    //                 impl.system,
    //                 make_vector<double>(event.motion.x, event.motion.y));
    //             break;

    //         case SDL_MOUSEBUTTONDOWN:
    //             process_mouse_motion(
    //                 impl.system,
    //                 make_vector<double>(event.button.x, event.button.y));
    //             process_mouse_press(
    //                 impl.system, decode_button(event.button.button));
    //             break;
    //         case SDL_MOUSEBUTTONUP:
    //             process_mouse_motion(
    //                 impl.system,
    //                 make_vector<double>(event.button.x, event.button.y));
    //             process_mouse_release(
    //                 impl.system, decode_button(event.button.button));
    //             break;

    //         default:

    //             break;
    //     }
    // }

    return true;
}

void
main_loop_callback(void* impl)
{
    if (!do_main_loop_iteration(
            *reinterpret_cast<emscripten_canvas_impl*>(impl)))
    {
        emscripten_cancel_main_loop();
    }
}

void
emscripten_canvas::do_main_loop()
{
    emscripten_set_main_loop_arg(main_loop_callback, impl_.get(), 0, 1);
}

emscripten_canvas::~emscripten_canvas()
{
    if (impl_)
    {
        impl_->skia_surface.reset();
        impl_->skia_graphics_context.reset();
    }
}

} // namespace alia
