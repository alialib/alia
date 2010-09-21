#include <alia/drop_down_menu.hpp>
#include <alia/drop_down_list.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <alia/input_utils.hpp>
#include <alia/widget_state.hpp>
#include <alia/controller.hpp>
#include <alia/input_events.hpp>
#include <cctype>

namespace alia {

struct drop_down_menu_event : targeted_event
{
    drop_down_menu_event(region_id list_id, context& ctx)
      : targeted_event(OTHER_CATEGORY, WRAPPED_EVENT, list_id)
      , ctx(ctx) {}
    context& ctx;
};

struct drop_down_menu_popup_controller : controller
{
    drop_down_menu_popup_controller(context& parent_ctx, region_id parent_id)
      : parent_ctx(parent_ctx), parent_id(parent_id) {}
    context& parent_ctx;
    region_id parent_id;
    void do_ui(context& ctx)
    {
        drop_down_menu_event e(parent_id, ctx);
        issue_event(parent_ctx, e);
    }
};

struct drop_down_menu::data
{
    popup_ptr popup;
};

void drop_down_menu::begin(context& ctx, layout const& layout_spec)
{
    ctx_ = &ctx;
    data_ = get_data<data>(ctx);
    id_ = get_region_id(ctx);

    layout spec = layout_spec;
    if ((spec.flags & X_ALIGNMENT_MASK) == 0)
        spec.flags |= LEFT;
    panel_.begin(ctx, "list", spec, HORIZONTAL, id_);

    do_list_ = false;
    active_ = true;

    switch (ctx.event->type)
    {
     case REFRESH_EVENT:
        // TODO: this really shouldn't have to be here
        if (data_->popup && data_->popup->was_dismissed())
            data_->popup.reset();
        break;

     case WRAPPED_EVENT:
      {
        drop_down_menu_event& e = get_event<drop_down_menu_event>(ctx);
        if (e.target_id == id_)
        {
            e.saw_target = true;

            do_list_ = true;
            list_ctx_ = &e.ctx;

            context& ctx = *list_ctx_;

            list_panel_.begin(ctx, "list", GROW, GREEDY);
        }
        break;
      }

     case FOCUS_LOSS_EVENT:
        if (data_->popup)
            data_->popup->close();
        break;
    }
}

void drop_down_menu::end()
{
    if (active_)
    {
        active_ = false;

        if (do_list_)
        {
            // Forward unprocessed key presses in the popup to the parent
            // context.
            if (list_ctx_->event->type == KEY_DOWN_EVENT)
            {
                key_event& e = get_event<key_event>(*list_ctx_);
                if (!e.processed)
                {
                    key_event e2(e.type, e.info, id_);
                    issue_event(*ctx_, e2);
                }
            }
        }

        if (do_drop_down_button(*ctx_, default_layout, NO_FLAGS, id_))
        {
            box2i const& region = panel_.get_region();
            point2i boundary(get_high_corner(region)[0], region.corner[1]),
                location(region.corner[0], get_high_corner(region)[1]);
            data_->popup.reset(ctx_->surface->open_popup(
                new drop_down_menu_popup_controller(*ctx_, id_),
                location, boundary, vector2i(-1, -1), vector2i(-1, -1), true));
        }

        panel_.end();
    }
}

void ddm_item::begin(drop_down_menu& menu)
{
    context& ctx = *menu.list_ctx_;
    region_id id = get_region_id(ctx);
    panel_.begin(ctx, "item", NOT_PADDED, NO_FLAGS, id);
    if (detect_click(ctx, id, LEFT_BUTTON))
    {
        selected_ = true;
        menu.data_->popup->close();
    }
    else
        selected_ = false;
    active_ = true;
}
void ddm_item::end()
{
    if (active_)
    {
        panel_.end();
        active_ = false;
    }
}

}
