#ifndef ALIA_PLATFORMS_WIN32_UI_BINDING_H
#define ALIA_PLATFORMS_WIN32_UI_BINDING_H

#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;
typedef struct alia_win32_host alia_win32_host;

// POD shared by Win32 window procs when forwarding input/surface updates into
// the UI system.
typedef struct alia_win32_ui_binding
{
    alia_ui_system* ui;
    alia_win32_host* host;
} alia_win32_ui_binding;

ALIA_EXTERN_C_END

#endif /* ALIA_PLATFORMS_WIN32_UI_BINDING_H */
