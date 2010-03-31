#include <alia/progress_bar.hpp>
#include <alia/context.hpp>
#include <alia/artist.hpp>
#include <alia/layout.hpp>

namespace alia {

struct progress_bar_data
{
    progress_bar_data() {}
    alia::layout_data layout_data;
    artist_data_ptr artist_data;
};

void do_progress_bar(context& ctx, double value,
    layout const& layout_spec)
{
    progress_bar_data& data = *get_data<progress_bar_data>(ctx);
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(ctx.artist->get_minimum_progress_bar_size(),
                0, 0, ctx.artist->get_default_progress_bar_size(),
                FILL_X | CENTER_Y, true));
        break;
      }
     case RENDER_CATEGORY:
      {
        ctx.artist->draw_progress_bar(data.artist_data,
            data.layout_data.assigned_region, value);
        break;
      }
    }
}

}
