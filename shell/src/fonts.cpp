#include <alia/shell/fonts.h>

#include "alia_fonts.h"

extern "C" {

bool
alia_shell_setup_stock_text(alia_shell* shell, alia_ui_system* ui)
{
    alia_msdf_atlas_rle const atlas_rle = alia_stock_msdf_atlas_rle();
    return alia_shell_setup_text(
        shell, ui, &atlas_rle, alia_font_descriptions, alia_font_count);
}

} // extern "C"
