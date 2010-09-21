#include <alia/color_display.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <alia/scoped_state.hpp>
#include <alia/surface.hpp>

namespace alia {

struct color_display_data
{
    alia::layout_data layout_data;
};

void do_color(
    context& ctx,
    rgb8 const& value,
    layout const& layout_spec)
{
    color_display_data& data = *get_data<color_display_data>(ctx);

    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        int size = get_font_metrics(ctx, ctx.pass_state.style->font).height +
            ctx.pass_state.style->padding_size[1] * 2;
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
        ctx.surface->draw_filled_polygon(value, poly, 4);
        break;
      }
    }
}

}
