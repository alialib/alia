#include <alia/panel.hpp>
#include <alia/context.hpp>
#include <alia/artist.hpp>
#include <alia/scrollable_region.hpp>
#include <alia/layout.hpp>
#include <alia/widget_state.hpp>
#include <alia/input_utils.hpp>

namespace alia {

//struct panel_border::data
//{
//    data() : minimum_size(0, 0), style_code(0),
//        overlay_region(point2i(0, 0), vector2i(0, 0)) {}
//    alia::layout_data layout_data;
//    vector2i minimum_size;
//    unsigned style_code;
//    artist_data_ptr artist_data;
//    box2i overlay_region;
//};
//
//void panel_border::begin(context& ctx, unsigned style_code,
//    layout const& layout_spec)
//{
//    ctx_ = &ctx;
//    data_ = get_data<data>(ctx);
//    style_code_ = style_code;
//    layout_spec_ = layout_spec;
//
//    if (ctx.event->category == LAYOUT_CATEGORY &&
//        get_event<layout_event>(ctx).active_logic)
//    {
//        resolved_layout_spec resolved;
//        resolve_layout_spec(ctx, &resolved, layout_spec,
//            resolve_size(ctx, layout_spec.size),
//            widget_layout_info(data_->minimum_size, 0, 0, data_->minimum_size,
//                FILL, true));
//        switch (ctx.event->type)
//        {
//         case REFRESH_EVENT:
//            diff_widget_location(ctx, data_->layout_data);
//            if (style_code != data_->style_code)
//            {
//                record_layout_change(ctx, data_->layout_data);
//                data_->style_code = style_code;
//            }
//            break;
//         case LAYOUT_PASS_1:
//          {
//            data_->layout_data.assigned_region.size[0] =
//                get_assigned_width(ctx, resolved);
//            border_size bs =
//                ctx.artist->get_panel_border_size(data_->artist_data,
//                    data_->style_code);
//            data_->overlay_region.size[0] =
//                data_->layout_data.assigned_region.size[0] - bs.left -
//                bs.right;
//            break;
//          }
//         case LAYOUT_PASS_2:
//          {
//            get_assigned_region(ctx, &data_->layout_data.assigned_region,
//                resolved);
//            record_layout(ctx, data_->layout_data, resolved);
//            border_size bs =
//                ctx.artist->get_panel_border_size(data_->artist_data,
//                    style_code);
//            box2i const& ar = data_->layout_data.assigned_region;
//            data_->overlay_region.corner[0] = ar.corner[0] + bs.left;
//            data_->overlay_region.corner[1] = ar.corner[1] + bs.top;
//            data_->overlay_region.size[0] = ar.size[0] - bs.left - bs.right;
//            data_->overlay_region.size[1] = ar.size[1] - bs.top - bs.bottom;
//            break;
//          }
//        }
//    }
//    else if (ctx.event->type == RENDER_EVENT)
//    {
//        ctx.artist->draw_panel_border(data_->artist_data, style_code,
//            data_->layout_data.assigned_region);
//    }
//
//    overlay_.begin(ctx, data_->overlay_region);
//
//    active_ = true;
//}
//void panel_border::end()
//{
//    if (!active_)
//        return;
//    active_ = false;
//
//    overlay_.end();
//
//    if (ctx_->event->category == LAYOUT_CATEGORY &&
//        get_event<layout_event>(*ctx_).active_logic)
//    {
//        switch (ctx_->event->type)
//        {
//         case LAYOUT_PASS_0:
//          {
//            border_size bs =
//                ctx_->artist->get_panel_border_size(data_->artist_data,
//                    style_code_);
//            data_->minimum_size[0] = overlay_.get_minimum_size()[0] +
//                bs.left + bs.right;
//            break;
//          }
//         case LAYOUT_PASS_1:
//          {
//            border_size bs =
//                ctx_->artist->get_panel_border_size(data_->artist_data,
//                    style_code_);
//            data_->minimum_size[1] = overlay_.get_minimum_size()[1] +
//                bs.top + bs.bottom;
//            break;
//          }
//        }
//        resolved_layout_spec resolved;
//        resolve_layout_spec(*ctx_, &resolved, layout_spec_,
//            resolve_size(*ctx_, layout_spec_.size),
//            widget_layout_info(data_->minimum_size, 0, 0,
//                data_->minimum_size, FILL, true));
//        switch (ctx_->event->type)
//        {
//         case REFRESH_EVENT:
//            diff_layout_spec(*ctx_, data_->layout_data, resolved);
//            break;
//         case LAYOUT_PASS_0:
//            request_horizontal_space(*ctx_, resolved);
//            break;
//         case LAYOUT_PASS_1:
//            request_vertical_space(*ctx_, resolved);
//            break;
//        }
//    }
//}
//box2i const& panel_border::get_content_region() const
//{
//    return data_->overlay_region;
//}

void panel_border::begin(context& ctx, layout const& layout_spec)
{
    bool padded = (layout_spec.flags & NOT_PADDED) == 0;

    layout adjusted_layout_spec = layout_spec;
    if (padded)
        adjusted_layout_spec.flags |= PADDED;
    outer_.begin(ctx, adjusted_layout_spec);
    region_ = outer_.get_region();
}
void panel_border::end()
{
    outer_.end();
}
bool panel_border::is_relevant() const
{
    return outer_.is_relevant();
}

void panel_background::begin(context& ctx, char const* style,
     layout const& layout_spec, flag_set flags, region_id id,
     widget_state state)
{
    if (!id)
        id = get_region_id(ctx);

    outer_.begin(ctx, layout_spec);
    region_ = outer_.get_region();

    substyle_.begin(ctx, style, state);

    layout adjusted_layout_spec = layout_spec;
    adjusted_layout_spec.flags |= PADDED;
    inner_.begin(ctx, adjusted_layout_spec, flags);

    artist_data_ptr& artist_data = *get_data<artist_data_ptr>(ctx);
    if (ctx.event->type == RENDER_EVENT)
        ctx.artist->draw_panel_background(artist_data, region_);

    // This is so the panel will block mouse events on things behind it.
    do_region(ctx, id, region_);

    // So the panel will steal the focus if clicked on.
    if (ctx.event->type == BUTTON_DOWN_EVENT && is_region_hot(ctx, id))
        set_focus(ctx, id);
}
void panel_background::end()
{
    inner_.end();
    substyle_.end();
    outer_.end();
}
bool panel_background::is_relevant() const
{
    return outer_.is_relevant();
}

void panel::begin(context& ctx, char const* style,
    layout const& layout_spec, flag_set flags, region_id id,
    widget_state state)
{
    border_.begin(ctx, layout_spec);
    bg_.begin(ctx, style, layout_spec, flags, id, state);
}
void panel::end()
{
    bg_.end();
    border_.end();
}
bool panel::is_relevant() const
{
    return bg_.is_relevant();
}
box2i const& panel::get_region() const
{
    return bg_.get_region();
}

void scrollable_panel::begin(context& ctx, char const* style,
    layout const& layout_spec, flag_set flags, region_id id)
{
    border_.begin(ctx, layout_spec);
    sr_.begin(ctx, layout_spec, flags);
    bg_.begin(ctx, style, layout_spec, flags, id, widget_states::NORMAL);
}
void scrollable_panel::end()
{
    bg_.end();
    sr_.end();
    border_.end();
}
bool scrollable_panel::is_relevant() const
{
    return bg_.is_relevant();
}
box2i const& scrollable_panel::get_region() const
{
    return bg_.get_region();
}

}
