#include <alia/node_expander.hpp>
#include <alia/context.hpp>
#include <alia/artist.hpp>
#include <alia/layout.hpp>
#include <alia/input_utils.hpp>
#include <alia/flags.hpp>
#include <alia/widget_state.hpp>

namespace alia {

struct node_expander_data
{
    node_expander_data() : key_state(0) {}
    alia::layout_data layout_data;
    artist_data_ptr artist_data;
    int key_state;
};

node_expander_result do_node_expander(
    context& ctx, accessor<bool> const& expanded, unsigned flags,
    layout const& layout_spec, region_id id)
{
    if (!id) id = get_region_id(ctx);
    node_expander_data& data = *get_data<node_expander_data>(ctx);
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        vector2i size = ctx.artist->get_node_expander_size(data.artist_data,
            expanded.is_valid() && expanded.get() ? 2 : 0);
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(size, 0, 0, size, LEFT | CENTER_Y, true));
        break;
      }
     case RENDER_CATEGORY:
      {
        ctx.artist->draw_node_expander(data.artist_data,
            expanded.is_valid() && expanded.get() ? 2 : 0,
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
            node_expander_result result;
            result.changed = true;
            result.new_value = expanded.is_valid() ? !expanded.get() : true;
            expanded.set(result.new_value);
            return result;
        }
        break;
      }
    }
    node_expander_result result;
    result.changed = false;
    return result;
}

struct tristate_node_expander_data
{
    tristate_node_expander_data() : key_state(0) {}
    alia::layout_data layout_data;
    artist_data_ptr artist_data;
    int key_state;
};

tristate_node_expander_result do_tristate_node_expander(
    context& ctx, accessor<int> const& expanded, unsigned flags,
    layout const& layout_spec, region_id id)
{
    if (!id) id = get_region_id(ctx);
    node_expander_data& data = *get_data<node_expander_data>(ctx);
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        vector2i size = ctx.artist->get_node_expander_size(data.artist_data,
            expanded.is_valid() ? expanded.get() : 0);
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(size, 0, 0, size, LEFT | CENTER_Y, true));
        break;
      }
     case RENDER_CATEGORY:
      {
        ctx.artist->draw_node_expander(data.artist_data,
            expanded.is_valid() ? expanded.get() : 0,
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
            tristate_node_expander_result result;
            result.changed = true;
            result.new_value =
                (!expanded.is_valid() || expanded.get() < 2) ? 2 : 0;
            expanded.set(result.new_value);
            return result;
        }
        break;
      }
    }
    tristate_node_expander_result result;
    result.changed = false;
    return result;
}

}
