#include <alia/host/host.h>

#include <alia/abi/prelude.h>

#ifndef __EMSCRIPTEN__
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
#if defined(_WIN32)
    alia_win32_host* win32 = nullptr;
#endif
#else
    alia_web_host* web = nullptr;
    char const* canvas_selector = nullptr;
#endif
};

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
namespace {

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

} // namespace
#endif

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
#if defined(_WIN32)
    return host_open_win32(host, config);
#else
    (void) host;
    (void) config;
    return false;
#endif
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
    ALIA_ASSERT(host->win32);
    alia_win32_host_sync_surface(host->win32, ui);
#endif
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
    ALIA_ASSERT(host->win32);
    alia_win32_host_run_config const win32_config = {
        .ui = config->ui,
        .frame = config->frame,
        .continuous = config->continuous,
        .probe_clear = false,
        .on_window_state_changed = config->on_window_state_changed,
    };
    alia_win32_host_run(host->win32, &win32_config);
#endif
#else
    ALIA_ASSERT(host->web);
    alia_web_host_config const web_config = {
        .ui = config->ui,
        .canvas_selector = host->canvas_selector,
        .frame = config->frame,
        .continuous = config->continuous,
    };
    // Schedules requestAnimationFrame and returns. Do not tear down the app
    // afterward — the Emscripten runtime stays alive for async callbacks
    // (EXIT_RUNTIME=0). Avoid emscripten_exit_with_live_runtime here; it
    // throws an unwind that fights -fwasm-exceptions.
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
