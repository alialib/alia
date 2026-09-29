#ifndef ALIA_SHELL_FONTS_H
#define ALIA_SHELL_FONTS_H

#include <alia/shell/shell.h>

#include <stdbool.h>

ALIA_EXTERN_C_BEGIN

// Upload the merged stock atlas (UI icons from `shell/assets/ui_icons.yaml`
// plus typography from `shell/assets/fonts.yaml`), bind an MSDF text engine,
// register core icons on the UI, and regenerate style defaults so widgets pick
// them up.
bool
alia_shell_setup_stock_text(alia_shell* shell, alia_ui_system* ui);

ALIA_EXTERN_C_END

#endif /* ALIA_SHELL_FONTS_H */
