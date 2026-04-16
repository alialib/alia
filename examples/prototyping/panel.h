#pragma once

template<class Content>
void
concrete_panel(
    context& ctx,
    alia_z_index z_index,
    alia_srgb8 color,
    layout_flag_set flags,
    Content&& content)
{
    alia_box panel_box;
    column(ctx, flags, &panel_box, [&]() {
        if (get_event_type(ctx) == ALIA_EVENT_DRAW)
        {
            alia_draw_rounded_box(
                &ctx, z_index, panel_box, alia_srgba8_from_srgb8(color), 0.0f);
        }

        std::forward<Content>(content)();
    });
}

template<class Content, class LayoutMods>
void
panel(
    context& ctx,
    alia_z_index z_index,
    alia_srgb8 color,
    LayoutMods mods,
    Content&& content)
{
    apply_mods(ctx, mods, [&] {
        concrete_panel(
            ctx, z_index, color, FILL, std::forward<Content>(content));
    });
}
