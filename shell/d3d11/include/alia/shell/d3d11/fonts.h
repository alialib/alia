#ifndef ALIA_SHELL_D3D11_FONTS_H
#define ALIA_SHELL_D3D11_FONTS_H

#include <alia/renderers/d3d11/renderer.h>
#include <alia/shell/d3d11/shell.h>

#include <stdbool.h>

ALIA_EXTERN_C_BEGIN

bool
alia_d3d11_shell_setup_stock_text(
    alia_d3d11_shell* shell,
    alia_ui_system* ui,
    alia_d3d11_renderer* renderer);

ALIA_EXTERN_C_END

#endif /* ALIA_SHELL_D3D11_FONTS_H */
