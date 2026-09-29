#ifndef ALIA_ABI_UI_ICONS_H
#define ALIA_ABI_UI_ICONS_H

#include <alia/abi/prelude.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// semantic icons used by widgets
enum alia_ui_icon_id
{
    ALIA_UI_ICON_CHECK = 0,
    ALIA_UI_ICON_COUNT
};

typedef uint32_t alia_ui_icon_id_t;

// glyph binding for a semantic icon
typedef struct alia_ui_icon
{
    // font index in the UI's bound MSDF text engine
    size_t font_index;
    uint32_t codepoint;
} alia_ui_icon;

// Set the glyph for `id`.
void
alia_ui_icon_set(
    alia_ui_system* ui,
    alia_ui_icon_id_t id,
    size_t font_index,
    uint32_t codepoint);

// Clear the glyph for `id`.
void
alia_ui_icon_clear(alia_ui_system* ui, alia_ui_icon_id_t id);

// Write the registered icon into `out`. Returns false if unset or invalid.
bool
alia_ui_icon_get(
    alia_ui_system const* ui, alia_ui_icon_id_t id, alia_ui_icon* out);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_ICONS_H */
