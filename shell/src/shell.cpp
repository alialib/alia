#include <alia/shell/shell.h>

#include <alia/abi/base/geometry/vec2.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/events.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/msdf.h>
#include <alia/impl/events.hpp>
#include <alia/ui/system/internal_api.h>
#include <alia/ui/system/object.h>

#include <vector>

using namespace alia;

struct alia_shell
{
    alia_ui_controller inner{};
    alia_shell_config config{};
    alia_msdf_text_engine* text_engine = nullptr;
    std::vector<std::uint8_t> atlas_rgb;
};

namespace {

bool
edge_offsets_is_zero(alia_edge_offsets offsets)
{
    return offsets.left == 0.f && offsets.right == 0.f && offsets.top == 0.f
        && offsets.bottom == 0.f;
}

void
shell_draw_foundation_underlay(alia_context* ctx)
{
    alia_ui_system* ui = ctx->system;
    alia_box const surface_box
        = {{0.f, 0.f}, alia_vec2i_to_vec2f(ui->surface_size)};
    alia_srgba8 const fill
        = alia_srgba8_from_srgb8(ui->palette.foundation.background.base);
    alia_draw_box(
        ctx,
        0,
        surface_box,
        {.fill_color = fill,
         .corner_radius = 0.f,
         .border_width = 0.f,
         .border_color = fill});
}

void
shell_controller(void* user_data, alia_context* ctx)
{
    auto* shell = static_cast<alia_shell*>(user_data);
    ALIA_ASSERT(shell);
    ALIA_ASSERT(shell->inner.fn);

    if (get_event_type(*ctx) == ALIA_EVENT_DRAW
        && shell->config.draw_foundation_underlay)
    {
        shell_draw_foundation_underlay(ctx);
    }

    if (edge_offsets_is_zero(shell->config.surface_padding))
    {
        shell->inner.fn(shell->inner.user_data, ctx);
        return;
    }

    alia_layout_edge_offsets_begin(ctx, shell->config.surface_padding, 0);
    shell->inner.fn(shell->inner.user_data, ctx);
    alia_layout_edge_offsets_end(ctx);
}

} // namespace

extern "C" {

alia_shell*
alia_shell_create(void)
{
    return new alia_shell{};
}

void
alia_shell_destroy(alia_shell* shell)
{
    ALIA_ASSERT(shell);
    if (shell->text_engine)
    {
        alia_msdf_destroy_text_engine(shell->text_engine);
        shell->text_engine = nullptr;
    }
    shell->atlas_rgb.clear();
    delete shell;
}

void
alia_shell_set_controller(
    alia_shell* shell, alia_ui_controller inner, alia_shell_config config)
{
    ALIA_ASSERT(shell);
    ALIA_ASSERT(inner.fn);
    shell->inner = inner;
    shell->config = config;
}

alia_ui_controller
alia_shell_ui_controller(alia_shell* shell)
{
    ALIA_ASSERT(shell);
    return alia_ui_controller{shell_controller, shell};
}

alia_msdf_text_engine*
alia_shell_text_engine(alia_shell* shell)
{
    ALIA_ASSERT(shell);
    return shell->text_engine;
}

bool
alia_shell_setup_text(
    alia_shell* shell,
    alia_ui_system* ui,
    alia_msdf_atlas_rle const* atlas_rle,
    alia_msdf_font_description const* font_descriptions,
    size_t font_count)
{
    ALIA_ASSERT(shell);
    ALIA_ASSERT(ui);
    ALIA_ASSERT(atlas_rle);
    ALIA_ASSERT(font_descriptions);
    ALIA_ASSERT(font_count > 0);
    ALIA_ASSERT(ui->renderer.upload_msdf_atlas);

    alia_shell_teardown_text(shell, ui);

    shell->atlas_rgb.resize(atlas_rle->decompressed_size);
    alia_msdf_decompress_atlas_rle(
        atlas_rle, shell->atlas_rgb.data(), shell->atlas_rgb.size());

    alia_msdf_atlas_image const atlas_image = {
        .rgb = shell->atlas_rgb.data(),
        .width = atlas_rle->width,
        .height = atlas_rle->height,
    };
    ui->renderer.upload_msdf_atlas(ui->renderer.user, &atlas_image);

    shell->text_engine
        = alia_msdf_create_text_engine(font_descriptions, font_count);
    alia_ui_bind_msdf_text_engine(ui, shell->text_engine);
    return shell->text_engine != nullptr;
}

void
alia_shell_teardown_text(alia_shell* shell, alia_ui_system* ui)
{
    ALIA_ASSERT(shell);
    ALIA_ASSERT(ui);

    alia_ui_unbind_msdf_text_engine(ui);
    if (shell->text_engine)
    {
        alia_msdf_destroy_text_engine(shell->text_engine);
        shell->text_engine = nullptr;
    }
    shell->atlas_rgb.clear();
}

void
alia_shell_draw(alia_ui_system* ui)
{
    ALIA_ASSERT(ui);
    alia_ui_execute_draw_pass(ui);
}

void
alia_shell_frame(alia_ui_system* ui)
{
    ALIA_ASSERT(ui);
    alia_ui_system_update(ui);
    alia_shell_draw(ui);
}

void
alia_shell_initial_refresh(alia_ui_system* ui)
{
    ALIA_ASSERT(ui);
    refresh_system(*ui);
    // Mark the UI as needing a frame to be drawn.
    ui->ui_dirty = true;
}

} // extern "C"
