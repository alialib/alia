#ifndef ALIA_UI_SYSTEM_INPUT_PROCESSING_HPP
#define ALIA_UI_SYSTEM_INPUT_PROCESSING_HPP

#include <alia/ui/geometry.hpp>
#include <alia/ui/system/input_constants.hpp>

namespace alia {

struct ui_system;

// Process mouse movement within (or into) the window.
void
process_mouse_motion(ui_system& ui, vector<2, double> const& position);

// Process the window's loss of the mouse.
void
process_mouse_loss(ui_system& ui);

// Process a mouse button press.
void
process_mouse_press(
    ui_system& ui, mouse_button button, key_modifiers mods = NO_FLAGS);

// Process a mouse button release.
void
process_mouse_release(ui_system& ui, mouse_button button);

// Process scroll inputs (e.g., the mouse wheel).
void
process_scroll(ui_system& ui, vector<2, double> const& delta);

// Process a key press.
bool
process_key_press(ui_system& ui, modded_key const& info);

// Process a key release.
bool
process_key_release(ui_system& ui, modded_key const& info);

} // namespace alia

#endif
