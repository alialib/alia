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
        int size = 4;
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(vector2i(0, 0), 0, 0, vector2i(size, size),
                LEFT | CENTER_Y, true));
        break;
      }

     case RENDER_CATEGORY:
      {
        point2i poly[4];
        make_polygon(poly, data.layout_data.assigned_region);
        ctx.surface->draw_filled_polygon(ctx.pass_state.text_color, poly, 4);
        break;
      }
    }
}

}
