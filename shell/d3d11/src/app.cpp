#include <alia/shell/d3d11/app.h>

#include <alia/abi/base/object.h>
#include <alia/abi/prelude.h>
#include <alia/host/host.h>
#include <alia/platforms/win32/host.h>
#include <alia/renderers/d3d11/renderer.h>
#include <alia/shell/d3d11/shell.h>

#include <cstring>

namespace {

bool
alia_d3d11_app_bootstrap_host(
    alia_d3d11_app_config const& config, alia_d3d11_app* app)
{
    ALIA_ASSERT(app);
    ALIA_ASSERT(app->ui);

    app->host = alia_host_create();
    if (!app->host)
        return false;

    alia_host_open_config const open_config = {
        .title = config.title,
        .window_state = config.window_state,
        .window_options = config.window_options,
        .canvas_selector = nullptr,
    };
    if (!alia_host_open(app->host, &open_config))
    {
        alia_host_destroy(app->host);
        app->host = nullptr;
        return false;
    }

    alia_host_sync_surface(app->host, app->ui);
    return true;
}

} // namespace

extern "C" {

int
alia_d3d11_app_init(alia_d3d11_app_config const* config, alia_d3d11_app* app)
{
    ALIA_ASSERT(config);
    ALIA_ASSERT(app);
    ALIA_ASSERT(config->inner.fn);

    std::memset(app, 0, sizeof(*app));

    app->shell = alia_d3d11_shell_create();
    if (!app->shell)
        return 1;

    alia_d3d11_shell_set_controller(app->shell, config->inner, config->shell);

    alia_struct_spec const ui_spec = alia_ui_system_object_spec();
    app->ui_storage = alia_object_alloc(ui_spec);
    if (!app->ui_storage)
    {
        alia_d3d11_app_destroy(app);
        return 1;
    }

    app->ui = alia_ui_system_init(
        app->ui_storage, alia_d3d11_shell_ui_controller(app->shell), {0, 0});
    if (!app->ui)
    {
        alia_d3d11_app_destroy(app);
        return 1;
    }

    if (!alia_d3d11_app_bootstrap_host(*config, app))
    {
        alia_d3d11_app_destroy(app);
        return 1;
    }

    alia_win32_host* win32 = alia_host_as_win32(app->host);
    if (!win32)
    {
        alia_d3d11_app_destroy(app);
        return 1;
    }

    alia_struct_spec const renderer_spec = alia_d3d11_renderer_object_spec();
    app->renderer_storage = alia_object_alloc(renderer_spec);
    if (!app->renderer_storage)
    {
        alia_d3d11_app_destroy(app);
        return 1;
    }

    app->renderer = alia_d3d11_renderer_init(app->renderer_storage);
    alia_d3d11_renderer_attach(
        app->renderer,
        app->ui,
        alia_win32_host_device(win32),
        alia_win32_host_context(win32));

    alia_d3d11_shell_initial_refresh(app->ui);
    return 0;
}

void
alia_d3d11_app_run_loop(
    alia_d3d11_app_config const* config, alia_d3d11_app* app)
{
    ALIA_ASSERT(config);
    ALIA_ASSERT(app);
    ALIA_ASSERT(app->ui);
    ALIA_ASSERT(app->host);
    ALIA_ASSERT(config->frame.fn);

    alia_host_run_config const run_config = {
        .ui = app->ui,
        .frame = config->frame,
        .continuous = config->continuous,
        .on_window_state_changed = config->on_window_state_changed,
    };
    alia_host_run(app->host, &run_config);
}

int
alia_d3d11_app_run(alia_d3d11_app_config const* config, alia_d3d11_app* app)
{
    if (alia_d3d11_app_init(config, app) != 0)
        return 1;

    alia_d3d11_app_run_loop(config, app);
    alia_d3d11_app_destroy(app);
    return 0;
}

void
alia_d3d11_app_destroy(alia_d3d11_app* app)
{
    if (!app)
        return;

    if (app->renderer)
        alia_d3d11_renderer_destroy(app->renderer);

    if (app->renderer_storage)
    {
        alia_object_free(app->renderer_storage);
        app->renderer_storage = nullptr;
    }
    app->renderer = nullptr;

    if (app->shell && app->ui)
        alia_d3d11_shell_teardown_text(app->shell, app->ui);

    if (app->shell)
    {
        alia_d3d11_shell_destroy(app->shell);
        app->shell = nullptr;
    }

    if (app->ui_storage)
    {
        alia_object_free(app->ui_storage);
        app->ui_storage = nullptr;
    }
    app->ui = nullptr;

    if (app->host)
    {
        alia_host_destroy(app->host);
        app->host = nullptr;
    }
}

} // extern "C"
