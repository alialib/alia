#ifndef ALIA_ABI_UI_SYSTEM_HOST_WINDOW_H
#define ALIA_ABI_UI_SYSTEM_HOST_WINDOW_H

#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// Host-provided window actions (GLFW, Win32, Emscripten, etc.). The UI may
// invoke these from bindings; implementations are optional (null function =
// no-op).
typedef struct alia_host_window_ops
{
    void (*toggle_fullscreen)(void* user);
    void* user;
} alia_host_window_ops;

void
alia_ui_system_set_host_window_ops(
    alia_ui_system* ui, alia_host_window_ops const* ops);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_SYSTEM_HOST_WINDOW_H */
