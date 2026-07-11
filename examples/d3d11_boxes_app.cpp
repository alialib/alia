// Win32 + D3D11 interactive smoke test: waitable present, slider, MSDF text.

#include <alia/platforms/win32/host.h>
#include <alia/renderers/d3d11/renderer.h>

#include <alia/abi/base/geometry.h>
#include <alia/abi/base/object.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/events.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/library.h>
#include <alia/abi/ui/msdf.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/system/api.h>
#include <alia/ui/system/internal_api.h>

#include "alia_fonts.h"

#include <d3d11.h>

#include <cstdio>
#include <cstring>
#include <vector>

using namespace alia;

namespace {

alia_ui_system* g_ui = nullptr;
alia_win32_host* g_host = nullptr;
alia_d3d11_renderer* g_renderer = nullptr;
alia_msdf_text_engine* g_text_engine = nullptr;
float g_value = 0.35f;

bool
setup_stock_text()
{
    alia_msdf_atlas_rle const atlas_rle = alia_stock_msdf_atlas_rle();
    std::vector<uint8_t> atlas_rgb(atlas_rle.decompressed_size);
    alia_msdf_decompress_atlas_rle(
        &atlas_rle, atlas_rgb.data(), atlas_rgb.size());

    alia_msdf_atlas_image const atlas_image = {
        .rgb = atlas_rgb.data(),
        .width = atlas_rle.width,
        .height = atlas_rle.height,
    };
    alia_d3d11_renderer_upload_msdf_atlas(g_renderer, &atlas_image);

    g_text_engine
        = alia_msdf_create_text_engine(alia_font_descriptions, alia_font_count);
    if (!g_text_engine)
        return false;
    alia_ui_bind_msdf_text_engine(g_ui, g_text_engine);
    return true;
}

void
teardown_stock_text()
{
    if (!g_ui)
        return;
    alia_ui_unbind_msdf_text_engine(g_ui);
    if (g_text_engine)
    {
        alia_msdf_destroy_text_engine(g_text_engine);
        g_text_engine = nullptr;
    }
}

void
boxes_controller(void* /*user*/, alia_context* ctx)
{
    if (ctx->events && ctx->events->event
        && ctx->events->event->type == ALIA_EVENT_DRAW)
    {
        alia_vec2i const size = alia_ui_surface_get_size(ctx->system);
        alia_srgba8 const bg = alia_srgba8_from_srgb8(
            ctx->palette->foundation.background.base);
        alia_draw_box(
            ctx,
            0,
            {{0.f, 0.f}, {float(size.x), float(size.y)}},
            {.fill_color = bg,
             .corner_radius = 0.f,
             .border_width = 0.f,
             .border_color = bg});

        if (g_text_engine)
        {
            char const* const label = "Hello D3D11";
            alia_srgba8 const text_color = alia_srgba8_from_srgb8(
                ctx->palette->foundation.text.base);
            alia_msdf_draw_text(
                g_text_engine,
                ctx,
                2,
                label,
                std::strlen(label),
                28.f,
                {40.f, 48.f},
                text_color,
                alia_font_roboto_regular_index);
        }

        // Value swatch — color tracks the slider.
        alia_draw_rounded_box(
            ctx,
            1,
            {{40.f, 120.f}, {280.f, 120.f}},
            {uint8_t(70 + g_value * 185),
             140,
             uint8_t(255 - g_value * 120),
             255},
            16.f);
    }

    alia_layout_edge_offsets_begin(
        ctx, alia_edge_offsets_make_uniform(32.f), 0);
    alia_layout_column_begin(ctx, ALIA_GROW | ALIA_FILL, 24.f);
    alia_do_slider_f(
        ctx, &g_value, 0.f, 1.f, 0.001f, ALIA_FILL_X, false, nullptr);
    alia_layout_column_end(ctx);
    alia_layout_edge_offsets_end(ctx);
}

void
frame(void* /*user*/)
{
    alia_win32_host_sync_surface(g_host, g_ui);

    ID3D11DeviceContext* context = alia_win32_host_context(g_host);
    ID3D11RenderTargetView* rtv = alia_win32_host_rtv(g_host);
    if (!context || !rtv)
        return;

    float clear[4] = {0.06f, 0.07f, 0.10f, 1.f};
    context->ClearRenderTargetView(rtv, clear);

    alia_ui_system_update(g_ui);
    alia_ui_execute_draw_pass(g_ui);
}

} // namespace

int
main()
{
    g_host = alia_win32_host_create();
    if (!g_host)
        return 1;

    alia_win32_host_open_config const open = {
        .title = "Alia D3D11 Boxes",
        .window_state = alia_window_state_make(900, 600),
        .resizable = true,
        .vsync = true,
    };
    if (!alia_win32_host_open(g_host, &open))
    {
        alia_win32_host_destroy(g_host);
        return 1;
    }

    alia_struct_spec const ui_spec = alia_ui_system_object_spec();
    void* ui_storage = alia_object_alloc(ui_spec);
    if (!ui_storage)
    {
        alia_win32_host_destroy(g_host);
        return 1;
    }

    g_ui = alia_ui_system_init(
        ui_storage, {boxes_controller, nullptr}, {900, 600});
    if (!g_ui)
    {
        alia_object_free(ui_storage);
        alia_win32_host_destroy(g_host);
        return 1;
    }

    {
        alia_theme_accent accent;
        alia_theme_accent_from_color(&accent, {70, 140, 255});
        alia_theme_context theme_ctx = alia_theme_context_default(true);
        alia_palette_from_accent(
            &g_ui->palette,
            &accent,
            &theme_ctx,
            nullptr,
            nullptr,
            ALIA_LITERAL_FIXED_SPECTRUM);
    }

    alia_struct_spec const renderer_spec = alia_d3d11_renderer_object_spec();
    void* renderer_storage = alia_object_alloc(renderer_spec);
    if (!renderer_storage)
    {
        alia_object_free(ui_storage);
        alia_win32_host_destroy(g_host);
        return 1;
    }

    g_renderer = alia_d3d11_renderer_init(renderer_storage);
    alia_d3d11_renderer_attach(
        g_renderer,
        g_ui,
        alia_win32_host_device(g_host),
        alia_win32_host_context(g_host));

    if (!setup_stock_text())
    {
        std::fprintf(stderr, "[alia d3d11 boxes] MSDF text setup failed\n");
        alia_d3d11_renderer_destroy(g_renderer);
        alia_object_free(renderer_storage);
        alia_object_free(ui_storage);
        alia_win32_host_destroy(g_host);
        return 1;
    }

    alia_win32_host_install(g_host, g_ui);
    alia_win32_host_sync_surface(g_host, g_ui);
    refresh_system(*g_ui);
    g_ui->ui_dirty = true;

    alia_win32_host_run_config const run = {
        .ui = g_ui,
        .frame = {frame, nullptr},
        .continuous = true,
        .probe_clear = false,
    };
    alia_win32_host_run(g_host, &run);

    teardown_stock_text();
    alia_d3d11_renderer_destroy(g_renderer);
    alia_object_free(renderer_storage);
    alia_object_free(ui_storage);
    alia_win32_host_destroy(g_host);
    return 0;
}
