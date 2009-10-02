#include <alia/spacer.hpp>
#include <alia/layout.hpp>
#include <alia/data.hpp>
#include <alia/context.hpp>

namespace alia {

void do_spacer(context& ctx, layout const& layout_spec)
{
    layout_data& data = *get_data<layout_data>(ctx);
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
        layout_widget(ctx, data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(vector2i(0, 0), 0, 0, vector2i(0, 0), FILL,
                false));
        break;
    }
}

void do_spacer(context& ctx, box2i* region, layout const& layout_spec)
{
    layout_data& data = *get_data<layout_data>(ctx);
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
        layout_widget(ctx, data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(vector2i(0, 0), 0, 0, vector2i(0, 0), FILL,
                false));
        break;
    }
    *region = data.assigned_region;
}

}
