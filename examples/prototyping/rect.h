#pragma once

bool
do_rect(
    context& ctx,
    alia_z_index z_index,
    alia_vec2f size,
    alia_srgb8 color,
    layout_flag_set flags)
{
    alia_element_id id = alia_element_get_identity(&ctx);

    switch (get_event_category(ctx))
    {
        case ALIA_CATEGORY_REFRESH: {
            alia_layout_leaf_emit(&ctx, size, raw_code(flags));
            break;
        }
        case ALIA_CATEGORY_SPATIAL: {
            alia_box box = alia_layout_leaf_read(&ctx);
            alia_element_box_region(&ctx, id, &box, ALIA_CURSOR_DEFAULT);
            break;
        }
        case ALIA_CATEGORY_DRAWING: {
            alia_box box = alia_layout_leaf_read(&ctx);
            alia_interaction_status_t status
                = alia_element_get_interaction_status(&ctx, id, 0);
            if (status & ALIA_INTERACTION_STATUS_HOVERED)
            {
                color = {0x00, 0x00, 0xff};
            }
            else if (status & ALIA_INTERACTION_STATUS_ACTIVE)
            {
                color = {0xff, 0x00, 0xff};
            }
            alia_draw_rounded_box(
                &ctx, z_index, box, alia_srgba8_from_srgb8(color), 0.0f);
            break;
        }
        case ALIA_CATEGORY_INPUT: {
            alia_box box = alia_layout_leaf_read(&ctx);
            if (alia_element_detect_click(&ctx, id, ALIA_BUTTON_LEFT))
                return true;
            break;
        }
    }
    return false;
}

bool
do_rect_with_offset(
    context& ctx,
    alia_z_index z_index,
    alia_vec2f size,
    alia_srgb8 color,
    layout_flag_set flags,
    alia_vec2f offset)
{
    switch (get_event_category(ctx))
    {
        case ALIA_CATEGORY_REFRESH: {
            alia_layout_leaf_emit(&ctx, size, raw_code(flags));
            break;
        }
        case ALIA_CATEGORY_SPATIAL: {
            alia_box box = alia_layout_leaf_read(&ctx);
            // box_region(ctx, id, box);
            break;
        }
        case ALIA_CATEGORY_DRAWING: {
            alia_box box = alia_layout_leaf_read(&ctx);
            alia_draw_rounded_box(
                &ctx, z_index, box, alia_srgba8_from_srgb8(color), 0.0f);
            break;
        }
        case ALIA_CATEGORY_INPUT: {
            alia_box box = alia_layout_leaf_read(&ctx);
            break;
        }
    }
    return false;
}

template<class LayoutMods>
void
do_rect(
    context& ctx,
    alia_z_index z_index,
    alia_vec2f size,
    alia_rgba color,
    LayoutMods mods)
{
    apply_mods(ctx, mods, [&] { do_rect(ctx, z_index, size, color, FILL); });
}

void
do_animated_rect(
    context& ctx,
    alia_z_index z_index,
    bool& initialized,
    alia_vec2f& offset,
    alia_vec2f size,
    alia_srgb8 color,
    layout_flag_set flags)
{
    placement_hook(ctx, FILL, [&](auto outer_placement) {
        alignment_override(ctx, flags, [&]() {
            placement_hook(ctx, FILL, [&](auto inner_placement) {
                alia_vec2f inner_pos
                    = inner_placement.box.min - outer_placement.box.min;
                if (get_event_type(ctx) == ALIA_EVENT_DRAW)
                {
                    if (!initialized)
                    {
                        offset = inner_pos;
                        initialized = true;
                    }
                    offset += (inner_pos - offset) * 0.1f;
                }
                do_rect_with_offset(
                    ctx, z_index, size, color, FILL, offset - inner_pos);
            });
        });
    });
}
