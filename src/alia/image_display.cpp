#include <alia/image_display.hpp>
#include <alia/data.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <alia/scoped_state.hpp>
#include <alia/transformations.hpp>

namespace alia {

struct image_display_data
{
    alia::layout_data layout_data;
    cached_image_ptr cached_image;
};

void do_image(
    context& ctx,
    image_interface const& img,
    unsigned flags,
    layout const& layout_spec)
{
    image_display_data& data = *get_data<image_display_data>(ctx);
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(vector2i(1, 1), 0, 0, img.size, CENTER, true));
        break;
     case RENDER_CATEGORY:
      {
        ctx.surface->cache_image(data.cached_image, img, flags);
        box2i const& region = data.layout_data.assigned_region;
        scoped_transformation st(ctx, translation(vector2d(region.corner)) *
            scaling_transformation(vector2d(region.size) /
                vector2d(img.size)));
        data.cached_image->draw(point2d(0, 0));
        break;
      }
    }
}

}
