#ifndef ALIA_SHELL_D3D11_SHELL_H
#define ALIA_SHELL_D3D11_SHELL_H

#include <alia/abi/base/geometry/edge_offsets.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/msdf.h>
#include <alia/abi/ui/system/api.h>

#include <stdbool.h>
#include <stddef.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_d3d11_shell alia_d3d11_shell;
typedef struct alia_d3d11_renderer alia_d3d11_renderer;

typedef struct alia_d3d11_shell_config
{
    // When true, fill the surface with the palette foundation background on
    // the draw pass before the application controller runs.
    bool draw_foundation_underlay;
    // Insets applied around the application controller via layout edge
    // offsets.
    alia_edge_offsets surface_padding;
} alia_d3d11_shell_config;

alia_d3d11_shell*
alia_d3d11_shell_create(void);

void
alia_d3d11_shell_destroy(alia_d3d11_shell* shell);

void
alia_d3d11_shell_set_controller(
    alia_d3d11_shell* shell,
    alia_ui_controller inner,
    alia_d3d11_shell_config config);

alia_ui_controller
alia_d3d11_shell_ui_controller(alia_d3d11_shell* shell);

alia_msdf_text_engine*
alia_d3d11_shell_text_engine(alia_d3d11_shell* shell);

bool
alia_d3d11_shell_setup_text(
    alia_d3d11_shell* shell,
    alia_ui_system* ui,
    alia_d3d11_renderer* renderer,
    alia_msdf_atlas_rle const* atlas_rle,
    alia_msdf_font_description const* font_descriptions,
    size_t font_count);

void
alia_d3d11_shell_teardown_text(alia_d3d11_shell* shell, alia_ui_system* ui);

void
alia_d3d11_shell_frame(alia_ui_system* ui);

void
alia_d3d11_shell_draw(alia_ui_system* ui);

void
alia_d3d11_shell_initial_refresh(alia_ui_system* ui);

ALIA_EXTERN_C_END

#endif /* ALIA_SHELL_D3D11_SHELL_H */
