// GLFW + OpenGL embed smoke test: own window/loop, Alia UI + GL renderer.

#include <alia/platforms/glfw/input_glue.h>
#include <alia/renderers/gl/renderer.h>

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

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstring>
#include <vector>

using namespace alia;

namespace {

alia_ui_system* g_ui = nullptr;
alia_gl_renderer* g_renderer = nullptr;
alia_msdf_text_engine* g_text_engine = nullptr;
alia_glfw_ui_binding g_binding{};
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
    alia_gl_renderer_upload_msdf_atlas(g_renderer, &atlas_image);

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
embed_controller(void* /*user*/, alia_context* ctx)
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
            char const* const label = "Hello GLFW+GL";
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

} // namespace

int
main()
{
    if (!glfwInit())
    {
        std::fprintf(stderr, "[alia glfw_gl_embed] glfwInit failed\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window
        = glfwCreateWindow(900, 600, "Alia GLFW+GL Embed", nullptr, nullptr);
    if (!window)
    {
        std::fprintf(stderr, "[alia glfw_gl_embed] glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::fprintf(stderr, "[alia glfw_gl_embed] gladLoadGLLoader failed\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    alia_struct_spec const ui_spec = alia_ui_system_object_spec();
    void* ui_storage = alia_object_alloc(ui_spec);
    if (!ui_storage)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    g_ui = alia_ui_system_init(
        ui_storage, {embed_controller, nullptr}, {900, 600});
    if (!g_ui)
    {
        alia_object_free(ui_storage);
        glfwDestroyWindow(window);
        glfwTerminate();
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

    alia_struct_spec const renderer_spec = alia_gl_renderer_object_spec();
    void* renderer_storage = alia_object_alloc(renderer_spec);
    if (!renderer_storage)
    {
        alia_object_free(ui_storage);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    g_renderer = alia_gl_renderer_init(renderer_storage);
    alia_gl_renderer_attach(g_renderer, g_ui);

    if (!setup_stock_text())
    {
        std::fprintf(stderr, "[alia glfw_gl_embed] MSDF text setup failed\n");
        alia_gl_renderer_destroy(g_renderer);
        alia_object_free(renderer_storage);
        alia_object_free(ui_storage);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    g_binding.ui = g_ui;
    alia_glfw_install_surface_callbacks(window, &g_binding);
    alia_glfw_install_default_input_callbacks(window, &g_binding);

    alia_glfw_sync_surface(window, g_ui);
    refresh_system(*g_ui);
    g_ui->ui_dirty = true;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        alia_glfw_sync_surface(window, g_ui);

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.06f, 0.07f, 0.10f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        alia_ui_system_update(g_ui);
        alia_ui_execute_draw_pass(g_ui);

        glfwSwapBuffers(window);
    }

    teardown_stock_text();
    alia_gl_renderer_destroy(g_renderer);
    alia_object_free(renderer_storage);
    alia_object_free(ui_storage);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
