#pragma once

#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/context.h>
#include <alia/impl/events.hpp>
#include <alia/ui/layout/api.hpp>

#include <algorithm>

template<class Content>
void
do_flow_panel(
    context& ctx,
    alia_z_index z_index,
    alia_edge_offsets offsets,
    alia_srgb8 color,
    Content&& content)
{
    alia_layout_edge_offsets_begin(&ctx, offsets, ALIA_PROVIDE_BOX);

    if (!is_refresh_event(ctx))
    {
        alia_layout_box_array box_array = alia_layout_consume_box_array(&ctx);
        if (get_event_type(ctx) == ALIA_EVENT_DRAW)
        {
            for (int i = 0; i < box_array.count; ++i)
            {
                alia_draw_box(
                    &ctx,
                    z_index,
                    box_array.boxes[i],
                    {.fill_color = alia_srgba8_from_srgb8(color)});
            }
        }
    }

    std::forward<Content>(content)();

    alia_layout_edge_offsets_end(&ctx);
}
