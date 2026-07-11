#include <alia/host/host.h>

#include <alia/abi/prelude.h>

#ifndef __EMSCRIPTEN__
#include <alia/platforms/glfw/host.h>
#if defined(_WIN32)
#include <alia/platforms/win32/host.h>
#endif
#else
#include <alia/platforms/web/host.h>
#endif

#include <new>

struct alia_host
{
#ifndef __EMSCRIPTEN__
    alia_host_backend backend = ALIA_HOST_BACKEND_AUTO;
    alia_glfw_host* glfw = nullptr;
    bool glfw_platform_initialized = false;
#if defined(_WIN32)
    alia_win32_host* win32 = nullptr;
#endif
#else
    alia_web_host* web = nullptr;
    char const* canvas_selector = nullptr;
#endif
};

namespace {

#ifndef __EMSCRIPTEN__
alia_host_backend
resolve_backend(alia_host_backend requested)
{
    if (requested != ALIA_HOST_BACKEND_AUTO)
        return requested;
#if defined(_WIN32)
    return ALIA_HOST_BACKEND_WIN32;
#else
    return ALIA_HOST_BACKEND_GLFW;
#endif
}

bool
host_open_glfw(alia_host* host, alia_host_open_config const* config)
{
    if (!alia_glfw_platform_init())
        return false;
    host->glfw_platform_initialized = true;

    host->glfw = alia_glfw_host_create();
    if (!host->glfw)
    {
        alia_glfw_platform_shutdown();
        host->glfw_platform_initialized = false;
        return false;
    }

    alia_glfw_window_options glfw_options{};
    alia_glfw_window_options const* options = nullptr;
    if (config->window_options)
    {
        glfw_options.resizable = config->window_options->resizable;
        glfw_options.vsync = config->window_options->vsync;
        glfw_options.vulkan_present = config->window_options->vulkan_present;
        options = &glfw_options;
    }

    char const* const title = config->title ? config->title : "Alia";
    if (!alia_glfw_host_open(
            host->glfw, title, config->window_state, options))
    {
        alia_glfw_host_destroy(host->glfw);
        host->glfw = nullptr;
        alia_glfw_platform_shutdown();
        host->glfw_platform_initialized = false;
        return false;
    }
    return true;
}

#if defined(_WIN32)
bool
host_open_win32(alia_host* host, alia_host_open_config const* config)
{
    host->win32 = alia_win32_host_create();
    if (!host->win32)
        return false;

    alia_win32_host_open_config open{};
    open.title = config->title ? config->title : "Alia";
    open.window_state = config->window_state;
    open.resizable = true;
    open.vsync = true;
    if (config->window_options)
    {
        open.resizable = config->window_options->resizable;
        open.vsync = config->window_options->vsync;
    }

    if (!alia_win32_host_open(host->win32, &open))
    {
        alia_win32_host_destroy(host->win32);
        host->win32 = nullptr;
        return false;
    }
    return true;
}
#endif
#endif

} // namespace

extern "C" {

alia_host*
alia_host_create(void)
{
    return new (std::nothrow) alia_host{};
}

void
alia_host_destroy(alia_host* host)
{
    if (!host)
        return;

#ifndef __EMSCRIPTEN__
#if defined(_WIN32)
    if (host->win32)
    {
        alia_win32_host_destroy(host->win32);
        host->win32 = nullptr;
    }
#endif
    if (host->glfw)
    {
        alia_glfw_host_destroy(host->glfw);
        host->glfw = nullptr;
    }
    if (host->glfw_platform_initialized)
    {
        alia_glfw_platform_shutdown();
        host->glfw_platform_initialized = false;
    }
#else
    if (host->web)
    {
        alia_web_host_destroy(host->web);
        host->web = nullptr;
    }
#endif

    delete host;
}

bool
alia_host_open(alia_host* host, alia_host_open_config const* config)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(config);

#ifndef __EMSCRIPTEN__
    host->backend = resolve_backend(config->backend);
#if defined(_WIN32)
    if (host->backend == ALIA_HOST_BACKEND_WIN32)
        return host_open_win32(host, config);
#endif
    if (host->backend == ALIA_HOST_BACKEND_GLFW)
        return host_open_glfw(host, config);
    return false;
#else
    host->web = alia_web_host_create();
    if (!host->web)
        return false;

    host->canvas_selector = config->canvas_selector;
    if (!alia_web_host_init_webgl(host->web, config->canvas_selector))
    {
        alia_web_host_destroy(host->web);
        host->web = nullptr;
        return false;
    }
    return true;
#endif
}

void
alia_host_sync_surface(alia_host* host, alia_ui_system* ui)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(ui);

#ifndef __EMSCRIPTEN__
#if defined(_WIN32)
    if (host->win32)
    {
        alia_win32_host_sync_surface(host->win32, ui);
        return;
    }
#endif
    ALIA_ASSERT(host->glfw);
    alia_glfw_host_sync_surface(host->glfw, ui);
#else
    ALIA_ASSERT(host->web);
    alia_web_host_sync_surface(host->web, ui, host->canvas_selector);
#endif
}

void
alia_host_run(alia_host* host, alia_host_run_config const* config)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(config);
    ALIA_ASSERT(config->ui);
    ALIA_ASSERT(config->frame.fn);

#ifndef __EMSCRIPTEN__
#if defined(_WIN32)
    if (host->win32)
    {
        alia_win32_host_run_config const win32_config = {
            .ui = config->ui,
            .frame = config->frame,
            .continuous = config->continuous,
            .probe_clear = false,
            .on_window_state_changed = config->on_window_state_changed,
        };
        alia_win32_host_run(host->win32, &win32_config);
        return;
    }
#endif
    ALIA_ASSERT(host->glfw);
    alia_glfw_host_config const glfw_config = {
        .ui = config->ui,
        .frame = config->frame,
        .continuous = config->continuous,
        .on_window_state_changed = config->on_window_state_changed,
    };
    alia_glfw_host_run(host->glfw, &glfw_config);
#else
    ALIA_ASSERT(host->web);
    alia_web_host_config const web_config = {
        .ui = config->ui,
        .canvas_selector = host->canvas_selector,
        .frame = config->frame,
        .continuous = config->continuous,
    };
    alia_web_host_run(host->web, &web_config);
#endif
}

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
alia_win32_host*
alia_host_as_win32(alia_host* host)
{
    ALIA_ASSERT(host);
    return host->win32;
}
#endif

} // extern "C"
