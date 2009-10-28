#include <alia/separator.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <alia/artist.hpp>

namespace alia {

struct separator_data
{
    artist_data_ptr artist_data;
    alia::layout_data layout_data;
};

void do_separator(context& ctx, flag_set axis, layout const& layout_spec)
{
    separator_data& data = *get_data<separator_data>(ctx);
    artist& artist = *ctx.artist;
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        vector2i minimum_size(artist.get_separator_width(),
            artist.get_separator_width());
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(minimum_size, 0, 0, minimum_size,
            axis == Y_AXIS ? CENTER_X | FILL_Y : FILL_X | CENTER_Y, true));
        break;
      }
     case RENDER_CATEGORY:
      {
        box2i const& assigned_region = data.layout_data.assigned_region;
        point2i const& corner = assigned_region.corner + (axis == Y_AXIS ?
            vector2i((assigned_region.size[0] -
                artist.get_separator_width()) / 2, 0) :
            vector2i(0, (assigned_region.size[1] -
                artist.get_separator_width()) / 2));
        int length = axis == Y_AXIS ? assigned_region.size[1] :
            assigned_region.size[0];
        artist.draw_separator(data.artist_data, corner, axis == Y_AXIS,
            length);
        break;
      }
    }
}

}
