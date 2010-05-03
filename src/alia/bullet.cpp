#include <alia/bullet.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <alia/scoped_state.hpp>
#include <alia/surface.hpp>

namespace alia {

struct bullet_data
{
    alia::layout_data layout_data;
};

void do_bullet(
    context& ctx,
    layout const& layout_spec)
{
    bullet_data& data = *get_data<bullet_data>(ctx);

    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        int font_size = int(ctx.pass_state.active_font.get_size() + 0.5);
        vector2i size(font_size / 3, font_size / 2);
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(vector2i(0, 0), 0, 0, size, CENTER, true));
        break;
      }

     case RENDER_CATEGORY:
      {
        box2i assigned_region = data.layout_data.assigned_region;
        int font_size = int(ctx.pass_state.active_font.get_size() + 0.5);
        int clip = font_size / 2 - font_size / 3;
        assigned_region.corner[1] += clip;
        assigned_region.size[1] -= clip;
        point2i poly[4];
        make_polygon(poly, assigned_region);
        ctx.surface->draw_filled_polygon(ctx.pass_state.text_color, poly, 4);
        break;
      }
    }
}

}
