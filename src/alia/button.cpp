#include <alia/button.hpp>
#include <alia/context.hpp>
#include <alia/surface.hpp>
#include <alia/artist.hpp>
#include <alia/widget_state.hpp>
#include <alia/layout.hpp>
#include <alia/input_utils.hpp>
#include <alia/flags.hpp>
#include <alia/style_utils.hpp>

namespace alia {

struct button_data
{
    button_data() : key_state(0) {}

    cached_text_ptr cached_text;
    artist_data_ptr artist_data;
    alia::layout_data layout_data;

    int key_state;

    style_versioning_data style_versioning;
    alia::font font;
};

button_result do_button(
    context& ctx,
    char const* text,
    layout const& layout_spec,
    flag_set flags,
    region_id id)
{
    if (!id)
        id = get_region_id(ctx);

    button_data* data_ptr;
    if (get_data(ctx, &data_ptr) ||
        is_outdated(ctx, data_ptr->style_versioning))
    {
        update(ctx, data_ptr->style_versioning);
        style_node const* button_style = get_substyle(ctx, "button");
        get_font_property(&data_ptr->font, ctx, button_style, "font");
    }
    button_data& data = *data_ptr;

    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        if (ctx.event->type == REFRESH_EVENT &&
            (!data.cached_text || data.cached_text->get_text() != text ||
            data.cached_text->get_font() != data.font))
        {
            ctx.surface->cache_text(data.cached_text, data.font, text);
        }
        vector2i min_size = ctx.artist->get_button_size(data.artist_data,
            data.cached_text->get_size());
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(min_size, 0, 0, min_size, LEFT | CENTER_Y,
                true));
        break;
      }
     case RENDER_CATEGORY:
      {
        widget_state state =
            get_widget_state(ctx, id, !(flags & DISABLED),
                data.key_state == 1);
        box2i const& region = data.layout_data.assigned_region;
        ctx.artist->draw_button(data.artist_data, region, state);
        vector2i content_offset = ctx.artist->get_button_content_offset(
            data.artist_data, data.cached_text->get_size(), state);
        data.cached_text->draw(point2d(region.corner + content_offset),
            ctx.artist->get_button_text_color(data.artist_data, state));
        break;
      }
     case REGION_CATEGORY:
        do_region(ctx, id, data.layout_data.assigned_region);
        break;
     case INPUT_CATEGORY:
      {
        if (!(flags & DISABLED))
        {
            add_to_focus_order(ctx, id);
            return detect_click(ctx, id, LEFT_BUTTON) ||
                detect_proper_key_release(ctx, data.key_state, id, KEY_SPACE);
        }
        else
            return false;
      }
    }
    return false;
}

button_result do_button(
    context& ctx,
    std::string const& text,
    layout const& layout_spec,
    flag_set flags,
    region_id id)
{
    return do_button(ctx, text.c_str(), layout_spec, flags, id);
}

}
