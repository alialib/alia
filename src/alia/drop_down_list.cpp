#include <alia/drop_down_list.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <alia/input_utils.hpp>
#include <alia/widget_state.hpp>
#include <alia/controller.hpp>
#include <alia/input_events.hpp>
#include <cctype>

namespace alia {

struct drop_down_list_event : targeted_event
{
    drop_down_list_event(region_id list_id, context& ctx)
      : targeted_event(OTHER_CATEGORY, WRAPPED_EVENT, list_id)
      , ctx(ctx) {}
    context& ctx;
};

struct ddl_set_value_event : targeted_event
{
    ddl_set_value_event(region_id list_id, unsigned value)
      : targeted_event(OTHER_CATEGORY, SET_VALUE_EVENT, list_id)
      , value(value) {}
    unsigned value;
};

struct drop_down_list_popup_controller : controller
{
    drop_down_list_popup_controller(context& parent_ctx, region_id parent_id)
      : parent_ctx(parent_ctx), parent_id(parent_id) {}
    context& parent_ctx;
    region_id parent_id;
    void do_ui(context& ctx)
    {
        drop_down_list_event e(parent_id, ctx);
        issue_event(parent_ctx, e);
    }
};

struct drop_down_list::data
{
    popup_ptr popup;
    int internal_selection;
    bool make_selection_visible;
};

void drop_down_list::begin(context& ctx, accessor<unsigned> const& accessor,
    unsigned n_items, layout const& layout_spec, flag_set flags)
{
    ctx_ = &ctx;
    data_ = get_data<data>(ctx);
    selection_ = accessor.is_valid() ? accessor.get() : -1;
    n_items_ = n_items;
    flags_ = flags;
    id_ = get_region_id(ctx);

    layout spec = layout_spec;
    if ((spec.flags & X_ALIGNMENT_MASK) == 0)
        spec.flags |= LEFT;
    unsigned style_code =
        ctx.artist->get_code_for_style(LIST_STYLE, get_widget_state(ctx, id_));
    panel_.begin(ctx, style_code, spec, ROW_LAYOUT, id_);

    do_list_ = false;
    changed_ = false;
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
        drop_down_list_event& e = get_event<drop_down_list_event>(ctx);
        if (e.target_id == id_)
        {
            do_list_ = true;
            list_ctx_ = &e.ctx;
            index_ = 0;
            make_selection_visible_ = false;

            context& ctx = *list_ctx_;

            list_panel_.begin(ctx, LIST_STYLE, GROW, NO_AXIS | GREEDY);

            key_event_info info;
            if (detect_key_press(ctx, &info))
            {
                int selection = data_->internal_selection;
                if (handle_key_press(info, selection))
                {
                    data_->internal_selection = clamp_selection(selection);
                    acknowledge_input_event(ctx);
                    make_selection_visible_ = true;
                }
                if (info.mods == 0 && (info.code == KEY_ENTER ||
                    info.code == KEY_SPACE))
                {
                    set_selection(accessor, data_->internal_selection);
                    acknowledge_input_event(ctx);
                    data_->popup->close();
                }
                if (info.mods == 0 && info.code == KEY_ESCAPE)
                {
                    acknowledge_input_event(ctx);
                    data_->popup->close();
                }
            }

            //if (mouse_is_inside_region(ctx, list_panel_.get_region()))
            //{
            //    int wheel_movement = detect_wheel_movement(ctx);
            //    if (wheel_movement != 0)
            //    {
            //        data_->internal_selection = clamp_selection(
            //            data_->internal_selection - wheel_movement);
            //        acknowledge_input_event(ctx);
            //        make_selection_visible_ = true;
            //    }
            //}
        }
        break;
      }

     case FOCUS_LOSS_EVENT:
        if (data_->popup)
            data_->popup->close();
        break;

    // TODO: Only disable this if the mouse arrived on the widget via a scroll
    // wheel movement.
     //case SCROLL_WHEEL_EVENT:
     // {
     //   if (mouse_is_inside_region(ctx, panel_.get_region()))
     //   {
     //       set_selection(accessor, selection_ - detect_wheel_movement(ctx));
     //       acknowledge_input_event(ctx);
     //   }
     //   break;
     // }

     case KEY_DOWN_EVENT:
      {
        key_event_info info;
        if (detect_key_press(ctx, &info, id_))
        {
            int selection = selection_;
            if (handle_key_press(info, selection))
            {
                set_selection(accessor, selection);
                acknowledge_input_event(ctx);
            }
        }
        break;
      }

     case SET_VALUE_EVENT:
      {
        ddl_set_value_event& e = get_event<ddl_set_value_event>(ctx);
        if (e.target_id == id_)
            set_selection(accessor, e.value);
        break;
      }
    }
}

void drop_down_list::end()
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
            point2i boundary(-1, region.corner[1]),
                location(region.corner[0], get_high_corner(region)[1]);
            data_->internal_selection = selection_;
            data_->popup.reset(ctx_->surface->open_popup(
                new drop_down_list_popup_controller(*ctx_, id_),
                location, boundary));
            data_->make_selection_visible = true;
        }

        panel_.end();
    }
}

void drop_down_list::set_selection(accessor<unsigned> const& accessor,
    int new_selection)
{
    if (n_items_ != 0)
    {
        new_selection = clamp_selection(new_selection);
        if (selection_ != new_selection)
        {
            accessor.set(new_selection);
            selection_ = new_selection;
            changed_ = true;
        }
    }
}

int drop_down_list::clamp_selection(int selection)
{
    if (selection < 0)
        return 0;
    if (selection > int(n_items_) - 1)
        return int(n_items_) - 1;
    return selection;
}

bool drop_down_list::handle_key_press(key_event_info& info, int& selection)
{
    if (info.mods == 0)
    {
        switch (info.code)
        {
         case KEY_UP:
            --selection;
            return true;
         case KEY_DOWN:
            ++selection;
            return true;
         case KEY_PAGEUP:
            selection -= 10;
            return true;
         case KEY_PAGEDOWN:
            selection += 10;
            return true;
         case KEY_HOME:
            selection = 0;
            return true;
         case KEY_END:
            selection = n_items_ - 1;
            return true;
        }
    }
    return false;
}

void ddl_item::begin(drop_down_list& list)
{
    list_ = &list;
    int index = list.index_;
    ++list.index_;
    selected_ = index == list.data_->internal_selection;
    context& ctx = *list.list_ctx_;
    region_id id = get_region_id(ctx);
    unsigned style_code = ctx.artist->get_code_for_style(ITEM_STYLE,
        get_widget_state(ctx, id), selected_);
    panel_.begin(*list.list_ctx_, style_code, NOT_PADDED, NO_FLAGS, id);
    if (detect_click(ctx, id, LEFT_BUTTON))
    {
        ddl_set_value_event e(list.id_, index);
        issue_event(*list.ctx_, e);
        list.data_->popup->close();
    }
    else if (selected_ && (list.make_selection_visible_ ||
        ctx.event->type == INIT_EVENT &&
        list.data_->make_selection_visible))
    {
        make_region_visible(ctx, id);
        list.data_->make_selection_visible = false;
    }
    active_ = true;
}
void ddl_item::end()
{
    if (active_)
    {
        panel_.end();
        active_ = false;
    }
}

struct drop_down_button_data
{
    drop_down_button_data() : key_state(0) {}
    alia::layout_data layout_data;
    artist_data_ptr artist_data;
    int key_state;
};

bool do_drop_down_button(context& ctx, layout const& layout_spec,
    flag_set flags, region_id id)
{
    if (!id) id = get_region_id(ctx);
    drop_down_button_data& data = *get_data<drop_down_button_data>(ctx);
    switch (ctx.event->category)
    {
     case LAYOUT_CATEGORY:
      {
        vector2i size = ctx.artist->get_minimum_drop_down_button_size();
        layout_widget(ctx, data.layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(size, 0, 0, size, FILL, true));
        break;
      }
     case RENDER_CATEGORY:
      {
        ctx.artist->draw_drop_down_button(data.artist_data,
            data.layout_data.assigned_region,
            get_widget_state(ctx, id, true, data.key_state == 1));
        break;
      }
     case REGION_CATEGORY:
        do_region(ctx, id, data.layout_data.assigned_region);
        break;
     case INPUT_CATEGORY:
      {
        add_to_focus_order(ctx, id);
        return detect_click(ctx, id, LEFT_BUTTON) ||
            detect_proper_key_release(ctx, data.key_state, id, KEY_SPACE);
        break;
      }
    }
    return false;
}

}
