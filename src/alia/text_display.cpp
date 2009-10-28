#include <alia/text_display.hpp>
#include <alia/layout.hpp>
#include <alia/flow_layout.hpp>
#include <alia/context.hpp>
#include <alia/surface.hpp>

namespace alia {

// TODO: flowing text

struct text_display_data
{
    cached_text_ptr renderer;
    alia::layout_data layout_data;
};

void do_text(
    context& ctx,
    char const* text,
    layout const& layout_spec,
    flag_set flags)
{
    text_display_data& data = *get_data<text_display_data>(ctx);
    font const& font = ctx.pass_state.active_font;
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
        if (ctx.event->type == REFRESH_EVENT &&
            (!data.renderer || data.renderer->get_text() != text ||
            data.renderer->get_font() != font))
        {
            record_layout_change(ctx, data.layout_data);
            ctx.surface->cache_text(data.renderer, font, text);
        }
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(data.renderer->get_size(),
                data.renderer->get_metrics().ascent,
                data.renderer->get_size()[1] -
                    data.renderer->get_metrics().ascent,
                data.renderer->get_size(), LEFT | BASELINE_Y, true));
        break;
     case RENDER_CATEGORY:
      {
        data.renderer->draw(point2d(data.layout_data.assigned_region.corner),
            ctx.pass_state.text_color);
        break;
      }
    }
}

void do_text(
    context& ctx,
    std::string const& text,
    layout const& layout_spec,
    flag_set flags)
{
    do_text(ctx, text.c_str(), layout_spec, flags);
}

struct paragraph_data
{
    cached_text_ptr renderer;
    alia::layout_data layout_data;
};

void do_paragraph(
    context& ctx,
    char const* text,
    layout const& layout_spec,
    flag_set flags)
{
    paragraph_data& data = *get_data<paragraph_data>(ctx);

    font const& font = ctx.pass_state.active_font;

    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        int minimum_width = get_font_metrics(ctx, font).average_width * 12;
        switch (ctx.event->type)
        {
         case REFRESH_EVENT:
            layout_widget(ctx, data.layout_data, layout_spec,
                resolve_size(ctx, layout_spec.size),
                widget_layout_info(vector2i(minimum_width, 0), 0, 0,
                    vector2i(minimum_width, 0), BASELINE_Y | FILL_X, true));
            if (!data.renderer || text != data.renderer->get_text() ||
                font != data.renderer->get_font())
            {
                record_layout_change(ctx, data.layout_data);
                data.renderer.reset();
            }
            break;
         case LAYOUT_PASS_0:
            layout_widget(ctx, data.layout_data, layout_spec,
                resolve_size(ctx, layout_spec.size),
                widget_layout_info(vector2i(minimum_width, 0), 0, 0,
                    vector2i(minimum_width, 0), BASELINE_Y | FILL_X, true));
            break;
         case LAYOUT_PASS_1:
            if (get_event<layout_event>(ctx).active_logic)
            {
              {
                resolved_layout_spec resolved;
                resolve_layout_spec(ctx, &resolved, layout_spec,
                    resolve_size(ctx, layout_spec.size),
                    widget_layout_info(vector2i(minimum_width, 0), 0, 0,
                        vector2i(minimum_width, 0), BASELINE_Y | FILL_X,
                        true));
                int assigned_width = get_assigned_width(ctx, resolved);
                if (!data.renderer ||
                    assigned_width != data.renderer->get_size()[0])
                {
                    ctx.surface->cache_text(data.renderer, font, text,
                        assigned_width);
                }
              }
              {
                resolved_layout_spec resolved;
                resolve_layout_spec(ctx, &resolved, layout_spec,
                    resolve_size(ctx, layout_spec.size),
                    widget_layout_info(
                        vector2i(minimum_width, data.renderer->get_size()[1]),
                        data.renderer->get_metrics().ascent,
                        data.renderer->get_size()[1] -
                            data.renderer->get_metrics().ascent,
                        vector2i(minimum_width, data.renderer->get_size()[1]),
                        BASELINE_Y | FILL_X, true));
                request_vertical_space(ctx, resolved);
              }
            }
            break;
         case LAYOUT_PASS_2:
            if (get_event<layout_event>(ctx).active_logic)
            {
              {
                resolved_layout_spec resolved;
                resolve_layout_spec(ctx, &resolved, layout_spec,
                    resolve_size(ctx, layout_spec.size),
                    widget_layout_info(
                        vector2i(minimum_width, data.renderer->get_size()[1]),
                        data.renderer->get_metrics().ascent,
                        data.renderer->get_size()[1] -
                            data.renderer->get_metrics().ascent,
                        vector2i(minimum_width, data.renderer->get_size()[1]),
                        BASELINE_Y | FILL_X, true));
                get_assigned_region(ctx, &data.layout_data.assigned_region,
                    resolved);
              }
              {
                // Need to record the same spec as will be used in the REFRESH
                // pass.
                resolved_layout_spec resolved;
                resolve_layout_spec(ctx, &resolved, layout_spec,
                    resolve_size(ctx, layout_spec.size),
                    widget_layout_info(vector2i(minimum_width, 0), 0, 0,
                        vector2i(minimum_width, 0), BASELINE_Y | FILL_X,
                        true));
                record_layout(ctx, data.layout_data, resolved);
              }
            }
            break;
        }
        break;
      }
     case RENDER_CATEGORY:
      {
        data.renderer->draw(
            point2d(data.layout_data.assigned_region.corner),
            ctx.pass_state.text_color);
        break;
      }
    }
}

void do_paragraph(
    context& ctx,
    std::string const& text,
    layout const& layout_spec,
    flag_set flags)
{
    do_paragraph(ctx, text.c_str(), layout_spec, flags);
}

}
