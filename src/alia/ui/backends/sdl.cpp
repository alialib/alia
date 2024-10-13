#include <alia/ui/backends/sdl.hpp>

#include <SDL.h>
#include <SDL_mouse.h>
#include <SDL_opengl.h>

#if defined(_WIN32)
#pragma warning(push, 0)
#endif

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

// #include "icu/SkLoadICU.h"

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

#if defined(_WIN32)
#pragma warning(pop)
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

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

struct sdl_window_impl
{
    SDL_Window* sdl_window = nullptr;
    SDL_GLContext sdl_context;

    // TODO: Bundle this up into a Skia structure?
    std::unique_ptr<GrDirectContext> skia_graphics_context;
    std::unique_ptr<SkSurface> skia_surface;

    // TODO: Does this go here?
    ui_system system;
};

struct sdl_os_interface final : os_interface
{
    sdl_os_interface(sdl_window_impl& impl) : impl_(impl)
    {
    }

    void
    set_clipboard_text(std::string text) override
    {
        // TODO
        // glfwSetClipboardString(impl_.sdl_window, text.c_str());
    }

    std::optional<std::string>
    get_clipboard_text() override
    {
        // TODO
        // char const* text = glfwGetClipboardString(impl_.sdl_window);
        // if (text)
        //     return std::string(text);
        // else
        //     return std::nullopt;
        return std::nullopt;
    }

    sdl_window_impl& impl_;
};

struct sdl_window_interface final : window_interface
{
    sdl_window_interface(sdl_window_impl& impl) : impl_(impl)
    {
    }

    void
    set_mouse_cursor(mouse_cursor cursor) override
    {
        SDL_Cursor* sdl_cursor = nullptr;
        switch (cursor)
        {
            case mouse_cursor::CROSSHAIR:
                sdl_cursor
                    = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
                break;
            case mouse_cursor::TEXT:
                sdl_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
                break;
            case mouse_cursor::POINTER:
                sdl_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
                break;
            case mouse_cursor::EW_RESIZE:
                sdl_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
                break;
            case mouse_cursor::NS_RESIZE:
                sdl_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
                break;
            default:
                break;
        }
        SDL_SetCursor(sdl_cursor);
        SDL_FreeCursor(sdl_cursor);
    }

    sdl_window_impl& impl_;
};

namespace {

struct sdl_global_init
{
    sdl_global_init()
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            throw alia::exception("SDL initialization failed");
        }
    }

    ~sdl_global_init()
    {
        SDL_Quit();
    }
};

// sdl_window_impl&
// get_impl(SDL_Window* window)
// {
//     return *reinterpret_cast<sdl_window_impl*>(
//         SDL_GetWindowData(window, "alia"));
// }

// void
// key_event_callback(
//     SDL_Window* window, int key, int /*scancode*/, int action, int mods)
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

// void
// scroll_callback(SDL_Window* window, double x, double y)
// {
//     process_scroll(get_impl(window).system, make_vector(x, y));
// }

void
reset_skia(sdl_window_impl& impl, vector<2, unsigned> size)
{
    SDL_GL_MakeCurrent(impl.sdl_window, impl.sdl_context);

    GrGLFramebufferInfo framebuffer_info;
    framebuffer_info.fFBOID = 0;
    framebuffer_info.fFormat = GL_RGBA8;

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
        throw alia::exception("Skia surface creation failed");
    impl.skia_surface.reset(surface.release());
}

void
init_skia(sdl_window_impl& impl, vector<2, unsigned> size)
{
    static bool globally_initialized = false;
    if (!globally_initialized)
    {
        if (!SkLoadICU())
        {
            throw alia::exception("SkLoadICU failed");
        }
    }

    // impl.font_collection = sk_make_sp<skia::textlayout::FontCollection>();
    // impl.font_collection->setDefaultFontManager(SkFontMgr::RefDefault());

    SDL_GL_MakeCurrent(impl.sdl_window, impl.sdl_context);

    impl.skia_graphics_context.reset(
        GrDirectContexts::MakeGL(nullptr, GrContextOptions()).release());
    reset_skia(impl, size);
}

void
render_ui(sdl_window_impl& impl)
{
    SDL_GL_MakeCurrent(impl.sdl_window, impl.sdl_context);

    // TODO: Track this ourselves.
    int width, height;
    SDL_GL_GetDrawableSize(impl.sdl_window, &width, &height);
    std::chrono::steady_clock::time_point begin
        = std::chrono::steady_clock::now();

    auto& canvas = *impl.skia_surface->getCanvas();
    canvas.clipRect(SkRect::MakeWH(SkScalar(width), SkScalar(height)));

    // TODO: Don't clear automatically.
    {
        SkPaint paint;
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
    SDL_GL_SwapWindow(impl.sdl_window);
}

void
update_ui(sdl_window_impl& impl)
{
    // TODO: Track this ourselves.
    int width, height;
    SDL_GL_GetDrawableSize(impl.sdl_window, &width, &height);

    std::chrono::steady_clock::time_point begin
        = std::chrono::steady_clock::now();

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

    long long refresh_time;
    {
        std::chrono::steady_clock::time_point end
            = std::chrono::steady_clock::now();
        refresh_time = std::chrono::duration_cast<std::chrono::microseconds>(
                           end - begin)
                           .count();
        begin = end;
    }

    resolve_layout(
        impl.system.layout, make_vector(float(width), float(height)));

    // Increment the refresh counter immediately after resolving layout so
    // that any changes detected after this will be associated with the new
    // counter value and thus cause a recalculation.
    ++impl.system.refresh_counter;

    long long layout_time;
    {
        std::chrono::steady_clock::time_point end
            = std::chrono::steady_clock::now();
        layout_time = std::chrono::duration_cast<std::chrono::microseconds>(
                          end - begin)
                          .count();
        begin = end;
    }

    static long long max_refresh_time = 0;
    max_refresh_time = (std::max)(refresh_time, max_refresh_time);
    // std::cout << "refresh: " << refresh_time << "[us]\n";
    // std::cout << "max_refresh_time: " << max_refresh_time << "[us]\n";

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

// void
// framebuffer_size_callback(SDL_Window* window, int width, int height)
// {
//     auto& impl = get_impl(window);
//     reset_skia(impl, make_vector<unsigned>(width, height));
//     update_ui(impl);
// }

// #ifdef __EMSCRIPTEN__
// int
// emscripten_window_resized_callback(
//     int event_type, const void* reserved, void* user_data)
// {
//     auto& impl = *reinterpret_cast<sdl_window_impl*>(user_data);

//     int width, height;
//     SDL_GL_GetDrawableSize(impl.sdl_window, &width, &height);
//     reset_skia(impl, make_vector<unsigned>(width, height));
//     update_ui(impl);

//     return 0;
// }
// #endif

static int
resize_event_watcher(void* data, SDL_Event* event)
{
    if (event->type == SDL_WINDOWEVENT
        && event->window.event == SDL_WINDOWEVENT_RESIZED)
    {
        auto& impl = *reinterpret_cast<sdl_window_impl*>(data);
        int width, height;
        SDL_GL_GetDrawableSize(impl.sdl_window, &width, &height);
        reset_skia(impl, make_vector<unsigned>(width, height));
        update_ui(impl);
    }
    return 0;
}

void
init_window(
    sdl_window_impl& impl, std::string const& title, vector<2, unsigned> size)
{
    static sdl_global_init sdl_singleton;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    SDL_Window* window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        size[0],
        size[1],
#ifdef __EMSCRIPTEN__
        SDL_WINDOW_OPENGL
#else
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
            | SDL_WINDOW_RESIZABLE
#endif
    );
    if (!window)
        throw alia::exception("SDL window creation failed");
    SDL_SetWindowData(window, "alia", &impl);

    SDL_AddEventWatch(resize_event_watcher, &impl);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    SDL_GL_MakeCurrent(window, context);

    // #ifdef __EMSCRIPTEN__
    //     EmscriptenFullscreenStrategy strategy;
    //     strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
    //     strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
    //     strategy.canvasResizedCallback
    //         = nullptr; // emscripten_window_resized_callback;
    //     strategy.canvasResizedCallbackUserData = &impl;
    //     emscripten_enter_soft_fullscreen("canvas", &strategy);
    // #endif

    // glfwSetScrollCallback(window, scroll_callback);

    // glfwSetKeyCallback(window, key_event_callback);

    // glfwSwapInterval(0);

    impl.sdl_window = window;
    impl.sdl_context = context;
}

void
destroy_window(sdl_window_impl& impl)
{
    SDL_GL_DeleteContext(impl.sdl_context);
    SDL_DestroyWindow(impl.sdl_window);
}

} // namespace

sdl_window::sdl_window(
    std::string const& title,
    vector<2, unsigned> size,
    std::function<void(ui_context)> controller)
    : impl_(new sdl_window_impl)
{
    initialize(
        impl_->system,
        controller,
        std::make_shared<sdl_os_interface>(*impl_),
        std::make_shared<sdl_window_interface>(*impl_));

    init_window(*impl_, title, size);

    int width, height;
    SDL_GL_GetDrawableSize(impl_->sdl_window, &width, &height);
    impl_->system.surface_size = make_vector<unsigned>(width, height);

    init_skia(*impl_, size);

    // Perform the initial update.
    update_ui(*impl_);
}

void*
sdl_window::handle() const
{
    return impl_->sdl_window;
}

mouse_button
decode_button(Uint8 button)
{
    switch (button)
    {
        case SDL_BUTTON_LEFT:
            return mouse_button::LEFT;
        case SDL_BUTTON_RIGHT:
            return mouse_button::RIGHT;
        case SDL_BUTTON_MIDDLE:
            return mouse_button::MIDDLE;
        default:
            return mouse_button(button);
    }
}

bool
main_loop_iteration(sdl_window_impl& impl)
{
    update_ui(impl);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_WINDOWEVENT:
                switch (event.window.event)
                {
                    case SDL_WINDOWEVENT_CLOSE:
                        return false;

                    default:
                        break;
                }
                break;

            case SDL_MOUSEMOTION:
                process_mouse_motion(
                    impl.system,
                    make_vector<double>(event.motion.x, event.motion.y));
                break;

            case SDL_MOUSEBUTTONDOWN:
                process_mouse_motion(
                    impl.system,
                    make_vector<double>(event.button.x, event.button.y));
                process_mouse_press(
                    impl.system, decode_button(event.button.button));
                break;
            case SDL_MOUSEBUTTONUP:
                process_mouse_motion(
                    impl.system,
                    make_vector<double>(event.button.x, event.button.y));
                process_mouse_release(
                    impl.system, decode_button(event.button.button));
                break;

            default:

                break;
        }
    }

    return true;
}

#ifdef __EMSCRIPTEN__
void
emscripten_main_loop_iteration(void* impl)
{
    if (!main_loop_iteration(*reinterpret_cast<sdl_window_impl*>(impl)))
    {
        emscripten_cancel_main_loop();
    }
}
#endif

void
sdl_window::do_main_loop()
{
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(
        emscripten_main_loop_iteration, impl_.get(), 0, 1);
#else
    while (main_loop_iteration(*impl_))
    {
    }
#endif
}

sdl_window::~sdl_window()
{
    if (impl_)
    {
        impl_->skia_surface.reset();
        impl_->skia_graphics_context.reset();

        destroy_window(*impl_);
    }
}

} // namespace alia
