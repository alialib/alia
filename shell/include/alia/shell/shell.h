#ifndef ALIA_SHELL_SHELL_H
#define ALIA_SHELL_SHELL_H

#include <alia/abi/base/geometry/edge_offsets.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/msdf.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/text.h>

#include <stdbool.h>
#include <stddef.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_shell alia_shell;

typedef struct alia_shell_config
{
    // palette foundation underlay drawn before the app controller
    bool draw_foundation_underlay;
    // insets around the application controller via layout edge offsets
    alia_edge_offsets surface_padding;
    // Ctrl+/-/0 UI magnification shortcuts
    bool enable_keyboard_zoom;
    // smooth animation when magnification changes via keyboard zoom
    bool enable_smooth_zoom;
} alia_shell_config;

// Return a shell config with no underlay, no padding, and zoom chrome on.
static inline alia_shell_config
alia_shell_config_defaults(void)
{
    alia_shell_config config;
    config.draw_foundation_underlay = false;
    config.surface_padding = alia_edge_offsets_make_uniform(0.f);
    config.enable_keyboard_zoom = true;
    config.enable_smooth_zoom = true;
    return config;
}

alia_shell*
alia_shell_create(void);

void
alia_shell_destroy(alia_shell* shell);

// Configure the inner controller and shell wrapper behavior.
void
alia_shell_set_controller(
    alia_shell* shell, alia_ui_controller inner, alia_shell_config config);

// Pass to `alia_ui_system_init` as the controller.
alia_ui_controller
alia_shell_ui_controller(alia_shell* shell);

// Non-owning pointer to the active MSDF text engine, if text setup succeeded.
alia_msdf_text_engine*
alia_shell_text_engine(alia_shell* shell);

// Number of stock typefaces registered during text setup (one per MSDF font).
size_t
alia_shell_typeface_count(alia_shell* shell);

// Typeface ID for stock font `index` (same order as the MSDF font table).
// Asserts if text is not set up or `index` is out of range.
alia_typeface_id
alia_shell_typeface(alia_shell* shell, size_t index);

// Decompress the atlas, upload via `ui->renderer.upload_msdf_atlas`, and bind
// an MSDF text engine. Requires renderer ops already installed on `ui`.
bool
alia_shell_setup_text(
    alia_shell* shell,
    alia_ui_system* ui,
    alia_msdf_atlas_rle const* atlas_rle,
    alia_msdf_font_description const* font_descriptions,
    size_t font_count);

void
alia_shell_teardown_text(alia_shell* shell, alia_ui_system* ui);

// Update and execute the draw pass.
void
alia_shell_frame(alia_ui_system* ui);

// Draw pass only. Caller must run `alia_ui_system_update` first if needed.
void
alia_shell_draw(alia_ui_system* ui);

// Layout refresh before entering the main loop.
void
alia_shell_initial_refresh(alia_ui_system* ui);

ALIA_EXTERN_C_END

#endif /* ALIA_SHELL_SHELL_H */
