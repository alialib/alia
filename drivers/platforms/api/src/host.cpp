#include <alia/host/host.h>

#include <alia/abi/prelude.h>

#ifndef __EMSCRIPTEN__
#include <alia/platforms/glfw/host.h>
#else
#include <alia/platforms/web/host.h>
#endif

#include <new>

struct alia_host
{
#ifndef __EMSCRIPTEN__
    alia_glfw_host* glfw = nullptr;
    bool glfw_platform_initialized = false;
#else
    alia_web_host* web = nullptr;
    char const* canvas_selector = nullptr;
#endif
};

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
#endif

    return true;
}

void
alia_host_sync_surface(alia_host* host, alia_ui_system* ui)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(ui);

#ifndef __EMSCRIPTEN__
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

} // extern "C"
