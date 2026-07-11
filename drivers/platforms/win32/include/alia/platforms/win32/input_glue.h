#ifndef ALIA_PLATFORMS_WIN32_INPUT_GLUE_H
#define ALIA_PLATFORMS_WIN32_INPUT_GLUE_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/constants.h>
#include <alia/platforms/win32/ui_binding.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

ALIA_EXTERN_C_BEGIN

// Read current modifier keys from the Win32 key state.
ALIA_API alia_kmods_t
alia_win32_current_kmods(void);

// Map a WM_KEY* / WM_SYSKEY* message to Alia key identity.
ALIA_API alia_key_info
alia_win32_key_info_from_msg(WPARAM wParam, LPARAM lParam);

ALIA_API void
alia_win32_enqueue_mouse_motion(alia_ui_system* ui, float x, float y);

ALIA_API void
alia_win32_enqueue_mouse_button(
    alia_ui_system* ui,
    float x,
    float y,
    alia_button_t button,
    bool pressed);

ALIA_API void
alia_win32_enqueue_double_click(
    alia_ui_system* ui, float x, float y, alia_button_t button);

// `wheel_delta` is in Win32 WHEEL_DELTA units (typically +/-120 per notch).
// `horizontal` selects WM_MOUSEHWHEEL vs WM_MOUSEWHEEL.
ALIA_API void
alia_win32_enqueue_wheel(
    alia_ui_system* ui, short wheel_delta, bool horizontal);

ALIA_API void
alia_win32_enqueue_key(
    alia_ui_system* ui, WPARAM wParam, LPARAM lParam, bool pressed);

ALIA_EXTERN_C_END

#endif /* ALIA_PLATFORMS_WIN32_INPUT_GLUE_H */
