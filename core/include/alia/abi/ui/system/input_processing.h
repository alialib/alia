#ifndef ALIA_UI_SYSTEM_INPUT_PROCESSING_H
#define ALIA_UI_SYSTEM_INPUT_PROCESSING_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/input/constants.h>

ALIA_EXTERN_C_BEGIN

// TODO: This isn't technically correct.
typedef struct alia_ui_system alia_ui_system;

// Process mouse movement within (or into) the window.
void
alia_ui_process_mouse_motion(alia_ui_system* ui, alia_vec2f position);

// Process the window's loss of the mouse.
void
alia_ui_process_mouse_loss(alia_ui_system* ui);

// Process a mouse button press.
void
alia_ui_process_mouse_press(
    alia_ui_system* ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods = 0);

// Process a double click.
void
alia_ui_process_double_click(
    alia_ui_system* ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods = 0);

// Process a mouse button release.
void
alia_ui_process_mouse_release(
    alia_ui_system* ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods = 0);

// Process scroll inputs (e.g., the mouse wheel).
void
alia_ui_process_scroll(alia_ui_system* ui, alia_vec2f delta);

// Process a key press.
bool
alia_ui_process_key_press(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods);

// Process a key release.
bool
alia_ui_process_key_release(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods);

// `process_key_press` calls both of the following.
// `process_focused_key_press` will only pass the key to the widget with the
// keyboard focus (if any).
// `process_background_key_press` will pass it to any widget that's listening.
bool
alia_ui_process_focused_key_press(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods);
bool
alia_ui_process_background_key_press(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods);

bool
alia_ui_process_focused_key_release(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods);
bool
alia_ui_process_background_key_release(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods);

// TODO: Implement text input.
// bool
// alia_ui_process_text_input(
//     alia_ui_system* ui, alia_utf8_string const& text);

void
alia_ui_process_focus_loss(alia_ui_system* ui);

void
alia_ui_process_focus_gain(alia_ui_system* ui);

ALIA_EXTERN_C_END

#endif
