#include <alia/abi/ui/icons.h>

#include <alia/abi/prelude.h>
#include <alia/ui/system/object.h>

extern "C" {

void
alia_ui_icon_set(
    alia_ui_system* ui,
    alia_ui_icon_id_t id,
    size_t font_index,
    uint32_t codepoint)
{
    ALIA_ASSERT(ui);
    ALIA_ASSERT(id < ALIA_UI_ICON_COUNT);
    ui->icons[id] = alia_ui_icon{.font_index = font_index, .codepoint = codepoint};
}

void
alia_ui_icon_clear(alia_ui_system* ui, alia_ui_icon_id_t id)
{
    alia_ui_icon_set(ui, id, 0, 0);
}

bool
alia_ui_icon_get(
    alia_ui_system const* ui, alia_ui_icon_id_t id, alia_ui_icon* out)
{
    ALIA_ASSERT(ui);
    ALIA_ASSERT(out);
    if (id >= ALIA_UI_ICON_COUNT)
        return false;
    alia_ui_icon const& icon = ui->icons[id];
    if (icon.codepoint == 0)
        return false;
    *out = icon;
    return true;
}

} // extern "C"
