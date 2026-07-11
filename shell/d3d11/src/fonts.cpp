#include <alia/shell/d3d11/fonts.h>

#include "alia_fonts.h"

extern "C" {

bool
alia_d3d11_shell_setup_stock_text(
    alia_d3d11_shell* shell,
    alia_ui_system* ui,
    alia_d3d11_renderer* renderer)
{
    alia_msdf_atlas_rle const atlas_rle = alia_stock_msdf_atlas_rle();
    return alia_d3d11_shell_setup_text(
        shell,
        ui,
        renderer,
        &atlas_rle,
        alia_font_descriptions,
        alia_font_count);
}

} // extern "C"
