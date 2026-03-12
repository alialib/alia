#pragma once

#include <alia/context.h>
#include <functional>

extern "C" {
// TODO: This isn't technically correct.
typedef struct alia_ui_system alia_ui_system;
} // extern "C"

// TODO: Make this C-friendly!

namespace alia {

struct os_interface;
struct window_interface;

alia_struct_spec
alia_ui_system_object_spec(void);

// Initialize the UI system.
alia_ui_system*
alia_ui_system_init(
    void* object_storage,
    std::function<void(context&)> controller,
    alia_vec2i surface_size
    /*external_interface* external,
    std::shared_ptr<os_interface> os,
    std::shared_ptr<window_interface> window*/);

// TODO: cleanup function

// Update the UI system.
// This detects changes in the UI contents and updates the layout of the UI.
// It also resolves what's under the mouse cursor and updates the UI
// accordingly.
void
alia_ui_system_update(alia_ui_system* ui);

// Set the size of the surface.
void
alia_ui_surface_set_size(alia_ui_system* ui, alia_vec2i new_size);

alia_vec2i
alia_ui_surface_get_size(alia_ui_system* ui);

// Set the DPI of the UI system.
void
alia_ui_surface_set_dpi(alia_ui_system* ui, float dpi);

// Get the DPI of the UI system.
float
alia_ui_surface_get_dpi(alia_ui_system* ui);

} // namespace alia
