#ifndef ALIA_INDIE_SYSTEM_INPUT_PROCESSING_HPP
#define ALIA_INDIE_SYSTEM_INPUT_PROCESSING_HPP

#include <alia/indie/system/input_constants.hpp>

namespace alia { namespace indie {

// Process mouse movement within (or into) the window.
void
process_mouse_motion(system& ui, vector<2, double> const& position);

// Process the window's loss of the mouse.
void
process_mouse_loss(system& ui);

// Process a mouse button press.
void
process_mouse_press(
    system& ui, mouse_button button, key_modifiers mods = NO_FLAGS);

// Process a mouse button release.
void
process_mouse_release(system& ui, mouse_button button);

}} // namespace alia::indie

#endif
