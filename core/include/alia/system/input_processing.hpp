#ifndef ALIA_UI_SYSTEM_INPUT_PROCESSING_HPP
#define ALIA_UI_SYSTEM_INPUT_PROCESSING_HPP

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/input/constants.h>

// TODO: Use forward declarations once those are sorted out.
#include <alia/system/object.hpp>

namespace alia {

// Process mouse movement within (or into) the window.
void
process_mouse_motion(ui_system& ui, alia_vec2f position);

// Process the window's loss of the mouse.
void
process_mouse_loss(ui_system& ui);

// Process a mouse button press.
void
process_mouse_press(
    ui_system& ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods = 0);

// Process a double click.
void
process_double_click(
    ui_system& ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods = 0);

// Process a mouse button release.
void
process_mouse_release(
    ui_system& ui,
    alia_vec2f position,
    alia_button_t button,
    alia_kmods_t mods = 0);

// Process scroll inputs (e.g., the mouse wheel).
void
process_scroll(ui_system& ui, alia_vec2f delta);

// Process a key press.
bool
process_key_press(ui_system& ui, alia_key_code_t key, alia_kmods_t mods);

// Process a key release.
bool
process_key_release(ui_system& ui, alia_key_code_t key, alia_kmods_t mods);

// process_key_press calls both of the following.
// process_focused_key_press will only pass the key to the widget with the
// keyboard focus (if any).
// process_background_key_press will pass it to any widget that's listening.
bool
process_focused_key_press(
    ui_system& ui, alia_key_code_t key, alia_kmods_t mods);
bool
process_background_key_press(
    ui_system& ui, alia_key_code_t key, alia_kmods_t mods);

} // namespace alia

#endif
