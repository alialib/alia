#include <alia/shell/fonts.h>

#include <alia/abi/ui/icons.h>
#include <alia/abi/ui/styling.h>

#include "alia_fonts.h"

extern "C" {

bool
alia_shell_setup_stock_text(alia_shell* shell, alia_ui_system* ui)
{
    alia_msdf_atlas_rle const atlas_rle = alia_stock_msdf_atlas_rle();
    if (!alia_shell_setup_text(
            shell,
            ui,
            &atlas_rle,
            alia_font_descriptions,
            alia_font_count,
            alia_font_roboto_regular_index))
    {
        return false;
    }

    // Register core UI icons, then refresh style defaults so widgets pick them
    // up.
    alia_ui_icon_set(
        ui,
        ALIA_UI_ICON_CHECK,
        alia_font_material_symbols_outlined_index,
        alia_font_material_symbols_outlined_icon_check);
    alia_style_generate_defaults(ui, nullptr);
    return true;
}

} // extern "C"
