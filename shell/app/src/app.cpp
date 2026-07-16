#include <alia/shell/app.h>

#include <alia/abi/base/object.h>
#include <alia/abi/prelude.h>
#include <alia/host/host.h>
#include <alia/shell/fonts.h>
#include <alia/shell/shell.h>

#include <cstring>

#if defined(ALIA_SHELL_BACKEND_D3D11)
#include <alia/platforms/win32/host.h>
#include <alia/renderers/d3d11/renderer.h>
#elif defined(ALIA_SHELL_BACKEND_GL)
#include <alia/renderers/gl/renderer.h>
#else
#error "alia_shell_app requires ALIA_SHELL_BACKEND_D3D11 or ALIA_SHELL_BACKEND_GL"
#endif

namespace {

struct alia_app_state
{
    alia_shell* shell = nullptr;
    alia_ui_system* ui = nullptr;
    void* ui_storage = nullptr;
#if defined(ALIA_SHELL_BACKEND_D3D11)
    alia_d3d11_renderer* renderer = nullptr;
#elif defined(ALIA_SHELL_BACKEND_GL)
    alia_gl_renderer* renderer = nullptr;
#endif
    void* renderer_storage = nullptr;
    alia_host* host = nullptr;
};

static_assert(
    sizeof(alia_app_state) <= sizeof(alia_app),
    "alia_app storage too small for app state");
static_assert(
    alignof(alia_app_state) <= alignof(alia_app),
    "alia_app alignment too small for app state");

alia_app_state*
as_state(alia_app* app)
{
    ALIA_ASSERT(app);
    return reinterpret_cast<alia_app_state*>(app->bytes);
}

bool
bootstrap_host(alia_app_config const& config, alia_app_state* state)
{
    ALIA_ASSERT(state);
    ALIA_ASSERT(state->ui);

    state->host = alia_host_create();
    if (!state->host)
        return false;

    alia_host_open_config const open_config = {
        .title = config.title,
        .window_state = config.window_state,
        .window_options = config.window_options,
#if defined(ALIA_SHELL_BACKEND_GL)
        .canvas_selector = config.canvas_selector,
#else
        .canvas_selector = nullptr,
#endif
    };
    if (!alia_host_open(state->host, &open_config))
    {
        alia_host_destroy(state->host);
        state->host = nullptr;
        return false;
    }

    alia_host_sync_surface(state->host, state->ui);
    return true;
}

bool
attach_renderer(alia_app_state* state)
{
    ALIA_ASSERT(state);
    ALIA_ASSERT(state->ui);
    ALIA_ASSERT(state->host);

#if defined(ALIA_SHELL_BACKEND_D3D11)
    alia_win32_host* win32 = alia_host_as_win32(state->host);
    if (!win32)
        return false;

    alia_struct_spec const renderer_spec = alia_d3d11_renderer_object_spec();
    state->renderer_storage = alia_object_alloc(renderer_spec);
    if (!state->renderer_storage)
        return false;

    state->renderer = alia_d3d11_renderer_init(state->renderer_storage);
    alia_d3d11_renderer_attach(
        state->renderer,
        state->ui,
        alia_win32_host_device(win32),
        alia_win32_host_context(win32));
#else
    alia_struct_spec const renderer_spec = alia_gl_renderer_object_spec();
    state->renderer_storage = alia_object_alloc(renderer_spec);
    if (!state->renderer_storage)
        return false;

    state->renderer = alia_gl_renderer_init(state->renderer_storage);
    alia_gl_renderer_attach(state->renderer, state->ui);
#endif
    return true;
}

} // namespace

extern "C" {

int
alia_app_init(alia_app_config const* config, alia_app* app)
{
    ALIA_ASSERT(config);
    ALIA_ASSERT(app);
    ALIA_ASSERT(config->inner.fn);

    std::memset(app, 0, sizeof(*app));
    alia_app_state* state = as_state(app);

    state->shell = alia_shell_create();
    if (!state->shell)
        return 1;

    alia_shell_set_controller(state->shell, config->inner, config->shell);

    alia_struct_spec const ui_spec = alia_ui_system_object_spec();
    state->ui_storage = alia_object_alloc(ui_spec);
    if (!state->ui_storage)
    {
        alia_app_destroy(app);
        return 1;
    }

    state->ui = alia_ui_system_init(
        state->ui_storage, alia_shell_ui_controller(state->shell), {0, 0});
    if (!state->ui)
    {
        alia_app_destroy(app);
        return 1;
    }

    if (!bootstrap_host(*config, state))
    {
        alia_app_destroy(app);
        return 1;
    }

    if (!attach_renderer(state))
    {
        alia_app_destroy(app);
        return 1;
    }

    alia_shell_initial_refresh(state->ui);
    return 0;
}

void
alia_app_run_loop(alia_app_config const* config, alia_app* app)
{
    ALIA_ASSERT(config);
    ALIA_ASSERT(app);
    ALIA_ASSERT(config->frame.fn);

    alia_app_state* state = as_state(app);
    ALIA_ASSERT(state->ui);
    ALIA_ASSERT(state->host);

    alia_host_run_config const run_config = {
        .ui = state->ui,
        .frame = config->frame,
        .continuous = config->continuous,
        .on_window_state_changed = config->on_window_state_changed,
    };
    alia_host_run(state->host, &run_config);
}

void
alia_app_destroy(alia_app* app)
{
    if (!app)
        return;

    alia_app_state* state = as_state(app);

    if (state->renderer)
    {
#if defined(ALIA_SHELL_BACKEND_D3D11)
        alia_d3d11_renderer_destroy(state->renderer);
#else
        alia_gl_renderer_destroy(state->renderer);
#endif
        state->renderer = nullptr;
    }

    if (state->renderer_storage)
    {
        alia_object_free(state->renderer_storage);
        state->renderer_storage = nullptr;
    }

    if (state->shell && state->ui)
        alia_shell_teardown_text(state->shell, state->ui);

    if (state->shell)
    {
        alia_shell_destroy(state->shell);
        state->shell = nullptr;
    }

    if (state->ui_storage)
    {
        alia_object_free(state->ui_storage);
        state->ui_storage = nullptr;
    }
    state->ui = nullptr;

    if (state->host)
    {
        alia_host_destroy(state->host);
        state->host = nullptr;
    }
}

int
alia_app_run(alia_app_config const* config, alia_app* app)
{
    if (alia_app_init(config, app) != 0)
        return 1;
    alia_app_run_loop(config, app);
#ifndef __EMSCRIPTEN__
    alia_app_destroy(app);
#endif
    return 0;
}

alia_ui_system*
alia_app_ui(alia_app* app)
{
    return as_state(app)->ui;
}

bool
alia_app_setup_stock_text(alia_app* app)
{
    alia_app_state* state = as_state(app);
    return alia_shell_setup_stock_text(state->shell, state->ui);
}

alia_msdf_text_engine*
alia_app_text_engine(alia_app* app)
{
    return alia_shell_text_engine(as_state(app)->shell);
}

void
alia_app_shell_frame(alia_ui_system* ui)
{
    alia_shell_frame(ui);
}

void
alia_app_shell_draw(alia_ui_system* ui)
{
    alia_shell_draw(ui);
}

#if defined(ALIA_SHELL_BACKEND_D3D11)
alia_d3d11_renderer*
alia_app_d3d11_renderer(alia_app* app)
{
    return as_state(app)->renderer;
}
#elif defined(ALIA_SHELL_BACKEND_GL)
alia_gl_renderer*
alia_app_gl_renderer(alia_app* app)
{
    return as_state(app)->renderer;
}
#endif

} // extern "C"
