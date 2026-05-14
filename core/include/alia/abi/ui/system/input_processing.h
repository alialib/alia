#ifndef ALIA_UI_SYSTEM_INPUT_PROCESSING_H
#define ALIA_UI_SYSTEM_INPUT_PROCESSING_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/input/constants.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// Enqueue mouse movement within (or into) the window.
void
alia_ui_enqueue_mouse_motion(alia_ui_system* ui, alia_vec2f position);

// Enqueue the window's loss of the mouse.
void
alia_ui_enqueue_mouse_loss(alia_ui_system* ui);

// Enqueue a mouse button press.
void
alia_ui_enqueue_mouse_press(
    alia_ui_system* ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods = 0);

// Enqueue a double click.
void
alia_ui_enqueue_double_click(
    alia_ui_system* ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods = 0);

// Enqueue a mouse button release.
void
alia_ui_enqueue_mouse_release(
    alia_ui_system* ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods = 0);

// Enqueue scroll inputs (e.g., the mouse wheel).
void
alia_ui_enqueue_scroll(alia_ui_system* ui, alia_vec2f delta);

// Enqueue a key press.
bool
alia_ui_enqueue_key_press(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods);

// Enqueue a key release.
bool
alia_ui_enqueue_key_release(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods);

// `enqueue_key_press` calls both of the following.
// `enqueue_focused_key_press` will only pass the key to the widget with the
// keyboard focus (if any).
// `enqueue_global_key_press` will pass it to any widget that's listening.
bool
alia_ui_enqueue_focused_key_press(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods);
bool
alia_ui_enqueue_global_key_press(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods);

bool
alia_ui_enqueue_focused_key_release(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods);
bool
alia_ui_enqueue_global_key_release(
    alia_ui_system* ui, alia_key_code_t key, alia_kmods_t mods);

// TODO: Implement text input.
// Enqueue text input.
// bool
// alia_ui_enqueue_text_input(
//     alia_ui_system* ui, alia_utf8_string const& text);

void
alia_ui_enqueue_focus_loss(alia_ui_system* ui);

void
alia_ui_enqueue_focus_gain(alia_ui_system* ui);

ALIA_EXTERN_C_END

#endif
