#include <alia/check_box.hpp>
#include <alia/context.hpp>
#include <alia/artist.hpp>
#include <alia/layout.hpp>
#include <alia/input_utils.hpp>
#include <alia/linear_layout.hpp>
#include <alia/text_display.hpp>
#include <alia/flags.hpp>
#include <alia/widget_state.hpp>

namespace alia {

struct check_box_data
{
    check_box_data() : key_state(0) {}
    alia::layout_data layout_data;
    artist_data_ptr artist_data;
    int key_state;
};

check_box_result do_check_box(
    context& ctx,
    accessor<bool> const& accessor,
    layout const& layout_spec,
    flag_set flags,
    region_id id)
{
    if (!id) id = get_region_id(ctx);
    check_box_data& data = *get_data<check_box_data>(ctx);
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        // TODO: Handle indeterminate values.
        vector2i size = ctx.artist->get_check_box_size(data.artist_data,
            accessor.is_valid() ? accessor.get() : false);
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(size, 0, 0, size, LEFT | CENTER_Y, true));
        break;
      }
     case RENDER_CATEGORY:
      {
        ctx.artist->draw_check_box(data.artist_data,
            accessor.is_valid() ? accessor.get() : false,
            data.layout_data.assigned_region.corner,
            get_widget_state(ctx, id, true, data.key_state == 1));
        break;
      }
     case REGION_CATEGORY:
        do_region(ctx, id, data.layout_data.assigned_region);
        break;
     case INPUT_CATEGORY:
      {
        add_to_focus_order(ctx, id);
        bool changed = detect_click(ctx, id, LEFT_BUTTON) ||
            detect_proper_key_release(ctx, data.key_state, id, KEY_SPACE);
        if (changed)
        {
            check_box_result result;
            result.changed = true;
            result.new_value = accessor.is_valid() ? !accessor.get() : true;
            accessor.set(result.new_value);
            return result;
        }
        break;
      }
    }
    check_box_result result;
    result.changed = false;
    return result;
}

check_box_result do_check_box(
    context& ctx,
    accessor<bool> const& accessor,
    char const* text,
    layout const& layout_spec,
    flag_set flags,
    region_id id)
{
    if (!id) id = get_region_id(ctx);
    layout ls = layout_spec;
    if (ls.flags & Y_ALIGNMENT_MASK)
        ls.flags |= BASELINE_Y;
    if (ls.flags & X_ALIGNMENT_MASK)
        ls.flags |= LEFT;
    // TODO: fix alignment of check box and text
    row_layout row(ctx, ls);
    check_box_result result;
    if (flags & REVERSED)
    {
        do_text(ctx, text);
        result = do_check_box(ctx, accessor, default_layout, flags, id);
    }
    else
    {
        result = do_check_box(ctx, accessor, default_layout, flags, id);
        do_text(ctx, text);
    }
    do_region(ctx, id, add_border(row.get_region(),
        -ctx.pass_state.style->padding_size));
    return result;
}

check_box_result do_check_box(
    context& ctx,
    accessor<bool> const& accessor,
    std::string const& text,
    layout const& layout_spec,
    flag_set flags,
    region_id id)
{
    return do_check_box(ctx, accessor, text.c_str(), layout_spec, flags, id);
}

}
