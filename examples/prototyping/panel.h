#pragma once

template<class Content>
void
concrete_panel(
    context& ctx,
    alia_z_index z_index,
    alia_rgba color,
    layout_flag_set flags,
    Content&& content)
{
    placement_hook(ctx, flags, [&](auto const& placement) {
        if (get_event_type(ctx) == ALIA_EVENT_DRAW)
        {
            alia_draw_rounded_box(&ctx, z_index, placement.box, color, 0.0f);
        }

        std::forward<Content>(content)();
    });
}

template<class Content, class LayoutMods>
void
panel(
    context& ctx,
    alia_z_index z_index,
    alia_rgba color,
    LayoutMods mods,
    Content&& content)
{
    apply_mods(ctx, mods, [&] {
        concrete_panel(
            ctx, z_index, color, FILL, std::forward<Content>(content));
    });
}
