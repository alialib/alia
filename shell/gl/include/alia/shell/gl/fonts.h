#ifndef ALIA_SHELL_GL_FONTS_H
#define ALIA_SHELL_GL_FONTS_H

#include <alia/renderers/gl/renderer.h>
#include <alia/shell/gl/shell.h>

#include <stdbool.h>

ALIA_EXTERN_C_BEGIN

bool
alia_gl_shell_setup_stock_text(
    alia_gl_shell* shell,
    alia_ui_system* ui,
    alia_gl_renderer* renderer);

ALIA_EXTERN_C_END

#endif /* ALIA_SHELL_GL_FONTS_H */
