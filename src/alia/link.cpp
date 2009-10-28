#include <alia/link.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <alia/input_utils.hpp>
#include <alia/artist.hpp>
#include <alia/surface.hpp>
#include <alia/flags.hpp>
#include <alia/widget_state.hpp>

namespace alia {

struct link_data
{
    link_data() : key_state(0) {}
    alia::layout_data layout_data;
    cached_text_ptr renderer, underlined_renderer;
    artist_data_ptr artist_data;
    int key_state;
};

bool do_link(
    context& ctx,
    char const* text,
    flag_set flags,
    layout const& layout_spec,
    region_id id)
{
    if (!id) id = get_region_id(ctx);
    link_data& data = *get_data<link_data>(ctx);
    font const& font = ctx.pass_state.active_font;
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        if (ctx.event->type == REFRESH_EVENT &&
            (!data.renderer || data.renderer->get_text() != text ||
            data.renderer->get_font() != font))
        {
            record_layout_change(ctx, data.layout_data);
            ctx.surface->cache_text(data.renderer, font, text);
            alia::font underlined_font = font;
            underlined_font.set_style(
                font.get_style() | alia::font::UNDERLINED);
            ctx.surface->cache_text(data.underlined_renderer, underlined_font,
                text);
        }
        vector2i size = data.renderer->get_size();
        size[0] += 2;
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(size,
                data.renderer->get_metrics().ascent,
                data.renderer->get_metrics().descent,
                size, LEFT | BASELINE_Y, true));
        break;
      }
     case RENDER_CATEGORY:
      {
        cached_text_ptr* r;
        if ((flags & DISABLED) == 0 &&
            (detect_potential_click(ctx, id) ||
            detect_click_in_progress(ctx, id, LEFT_BUTTON)))
        {
            ctx.surface->set_mouse_cursor(HAND_CURSOR);
            r = &data.underlined_renderer;
        }
        else
            r = &data.renderer;
        widget_state state =
            get_widget_state(ctx, id, (flags & DISABLED) == 0,
                data.key_state == 1);
        (*r)->draw(
            point2d(data.layout_data.assigned_region.corner + vector2i(1, 0)),
            ctx.artist->get_link_color(data.artist_data, state));
        if ((state & widget_states::FOCUSED) != 0)
        {
            // TODO: This should be a little less hackish, but that might
            // require more information about the font metrics.
            box2i const& ar = data.layout_data.assigned_region;
            box2i r;
            r.corner[0] = ar.corner[0] - 2;
            r.corner[1] = ar.corner[1] - 1;
            r.size[0] = ar.size[0] + 3;
            r.size[1] = ar.size[1] + 3;
            ctx.artist->draw_focus_rect(r,
                ctx.artist->get_link_color(data.artist_data, state));
        }
        break;
      }
     case REGION_CATEGORY:
        do_region(ctx, id, data.layout_data.assigned_region);
        break;
     case INPUT_CATEGORY:
      {
        add_to_focus_order(ctx, id);
        return detect_click(ctx, id, LEFT_BUTTON) ||
            detect_proper_key_release(ctx, data.key_state, id, KEY_SPACE);
      }
    }
    return false;
}

bool do_link(
    context& ctx,
    std::string const& text,
    flag_set flags,
    layout const& layout_spec,
    region_id id)
{
    return do_link(ctx, text.c_str(), flags, layout_spec, id);
}

}
