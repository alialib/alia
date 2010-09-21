#ifndef ALIA_DROP_DOWN_LIST_HPP
#define ALIA_DROP_DOWN_LIST_HPP

#include <alia/accessor.hpp>
#include <alia/panel.hpp>
#include <alia/flags.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <alia/input_utils.hpp>
#include <alia/widget_state.hpp>
#include <alia/controller.hpp>
#include <alia/input_events.hpp>
#include <boost/noncopyable.hpp>
#include <vector>

namespace alia {

template<class Index>
class drop_down_list : boost::noncopyable
{
 public:
    drop_down_list() : active_(false) {}
    drop_down_list(context& ctx, accessor<Index> const& selection,
        layout const& layout_spec = default_layout, flag_set flags = NO_FLAGS)
    { begin(ctx, selection, layout_spec, flags); }
    ~drop_down_list() { end(); }

    void begin(context& ctx, accessor<Index> const& selection,
        layout const& layout_spec = default_layout, flag_set flags = NO_FLAGS);
    void end();

    bool do_list() const { return do_list_; }
    context& list_context() const { return *list_ctx_; }

    bool changed() const { return changed_; }

    bool has_selection() const { return has_selection_; }
    Index const& selection() const { return selection_; }

 private:
    bool handle_key_press(key_event_info& info, bool& has_selection,
        Index& selection);
    void set_selection(accessor<Index> const& accessor,
        Index const& selection);
    void get_item_list(std::vector<Index>* items);

    template<class Index>
    friend class ddl_item;

    struct data
    {
        popup_ptr popup;
        bool has_internal_selection;
        Index internal_selection;
        bool make_selection_visible;
    };

    context* ctx_;
    data* data_;
    flag_set flags_;
    bool has_selection_;
    Index selection_;
    bool active_, changed_;
    region_id id_;
    panel panel_;
    bool do_list_, make_selection_visible_;
    context* list_ctx_;
    scrollable_panel list_panel_;
    column_layout padding_;
};

template<class Index>
class ddl_item : boost::noncopyable
{
 public:
    ddl_item() : active_(false) {}
    ddl_item(drop_down_list<Index>& list, Index const& index)
    { begin(list, index); }
    ~ddl_item() { end(); }
    void begin(drop_down_list<Index>& list, Index const& index);
    void end();
    bool is_selected() const { return selected_; }
 private:
    drop_down_list<Index>* list_;
    bool active_, selected_;
    panel panel_;
};

struct drop_down_list_event : targeted_event
{
    drop_down_list_event(region_id list_id, context& ctx)
      : targeted_event(OTHER_CATEGORY, WRAPPED_EVENT, list_id)
      , ctx(ctx) {}
    context& ctx;
};

template<class Index>
struct ddl_set_value_event : targeted_event
{
    ddl_set_value_event(region_id list_id, Index value)
      : targeted_event(OTHER_CATEGORY, SET_VALUE_EVENT, list_id)
      , value(value) {}
    Index value;
};

template<class Index>
struct ddl_get_contents_event : targeted_event
{
    ddl_get_contents_event(region_id list_id)
      : targeted_event(OTHER_CATEGORY, GET_CONTENTS_EVENT, list_id)
    {}
    std::vector<Index> contents;
};

struct drop_down_list_popup_controller : alia::controller
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

template<class Index>
void drop_down_list<Index>::begin(
    context& ctx, accessor<Index> const& accessor,
    layout const& layout_spec, flag_set flags)
{
    ctx_ = &ctx;
    data_ = get_data<data>(ctx);
    flags_ = flags;

    has_selection_ = accessor.is_valid();
    if (has_selection_)
        selection_ = accessor.get();

    id_ = get_region_id(ctx);

    layout spec = layout_spec;
    if ((spec.flags & X_ALIGNMENT_MASK) == 0)
        spec.flags |= LEFT;
    panel_.begin(ctx, "control", spec, HORIZONTAL, id_);

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
            e.saw_target = true;

            do_list_ = true;
            list_ctx_ = &e.ctx;

            make_selection_visible_ = false;

            // start the list panel
            {
                context& ctx = *list_ctx_;

                // TODO: fix this
                ctx.pass_state.active_style = ctx_->pass_state.active_style;
                ctx.pass_state.style = ctx_->pass_state.style;

                list_panel_.begin(ctx, "list", GROW | NOT_PADDED, GREEDY);
                padding_.begin(ctx, GROW | PADDED);

                key_event_info info;
                if (detect_key_press(ctx, &info))
                {
                    if (handle_key_press(info, data_->has_internal_selection,
                            data_->internal_selection))
                    {
                        acknowledge_input_event(ctx);
                        make_selection_visible_ = true;
                    }
                    if (info.mods == 0 && (info.code == KEY_ENTER ||
                        info.code == KEY_SPACE))
                    {
                        if (data_->has_internal_selection)
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
            }
        }
        break;
      }

     case FOCUS_LOSS_EVENT:
        if (data_->popup)
            data_->popup->close();
        break;

     case KEY_DOWN_EVENT:
      {
        key_event_info info;
        if (detect_key_press(ctx, &info, id_))
        {
            bool has_selection = has_selection_;
            Index selection = selection_;
            if (handle_key_press(info, has_selection, selection))
            {
                if (has_selection)
                    set_selection(accessor, selection);
                acknowledge_input_event(ctx);
            }
        }
        break;
      }

     case GET_CONTENTS_EVENT:
      {
        ddl_get_contents_event<Index>& e =
            get_event<ddl_get_contents_event<Index> >(ctx);

        e.saw_target = true;

        do_list_ = true;
        list_ctx_ = ctx_;

        make_selection_visible_ = false;

        break;
      }

     case SET_VALUE_EVENT:
      {
        targeted_event& e = get_event<targeted_event>(ctx);
        if (e.target_id == id_)
        {
            ddl_set_value_event<Index>& e =
                get_event<ddl_set_value_event<Index> >(ctx);
            e.saw_target = true;
            set_selection(accessor, e.value);
        }
        break;
      }
    }
}

template<class Index>
void drop_down_list<Index>::end()
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

            padding_.end();
            list_panel_.end();
        }

        if (do_drop_down_button(*ctx_, default_layout, NO_FLAGS, id_))
        {
            box2i const& region = panel_.get_region();
            point2i boundary(get_high_corner(region)[0], region.corner[1]),
                location(region.corner[0], get_high_corner(region)[1]);
            data_->has_internal_selection = has_selection_;
            data_->internal_selection = selection_;
            data_->popup.reset(ctx_->surface->open_popup(
                new drop_down_list_popup_controller(*ctx_, id_),
                location, boundary));
            data_->make_selection_visible = true;
        }

        panel_.end();
    }
}

template<class Index>
void drop_down_list<Index>::set_selection(accessor<Index> const& accessor,
    Index const& new_selection)
{
    if (selection_ != new_selection)
    {
        accessor.set(new_selection);
        has_selection_ = true;
        selection_ = new_selection;
        changed_ = true;
    }
}

template<class Index>
int get_selected_offset(std::vector<Index> const& items,
    Index const& selection)
{
    std::vector<Index>::const_iterator i =
        std::find(items.begin(), items.end(), selection);
    if (i != items.end())
        return int(i - items.begin());
    return -1;
}

template<class Index>
void select_item_at_offset(std::vector<Index> const& items, int offset,
    bool& has_selection, Index& selection)
{
    if (!items.empty())
    {
        if (offset < 0)
            offset = 0;
        if (offset >= int(items.size()))
            offset = int(items.size()) - 1;
        selection = items[offset];
        has_selection = true;
    }
    else
        has_selection = false;
}

template<class Index>
void drop_down_list<Index>::get_item_list(std::vector<Index>* items)
{
    ddl_get_contents_event<Index> event(id_);
    issue_event(*ctx_, event);
    std::swap(*items, event.contents);
}

template<class Index>
bool drop_down_list<Index>::handle_key_press(key_event_info& info,
    bool& has_selection, Index& selection)
{
    if (info.mods == 0)
    {
        std::vector<Index> items;
        get_item_list(&items);
        int offset = has_selection ? get_selected_offset(items, selection) :
            -1;
        switch (info.code)
        {
         case KEY_UP:
            select_item_at_offset(items, offset - 1, has_selection, selection);
            return true;
         case KEY_DOWN:
            select_item_at_offset(items, offset + 1, has_selection, selection);
            return true;
         case KEY_PAGEUP:
            select_item_at_offset(items, offset - 10, has_selection,
                selection);
            return true;
         case KEY_PAGEDOWN:
            select_item_at_offset(items, offset + 10, has_selection,
                selection);
            return true;
         case KEY_HOME:
            select_item_at_offset(items, 0, has_selection, selection);
            return true;
         case KEY_END:
            select_item_at_offset(items, int(items.size()) - 1, has_selection,
                selection);
            return true;
        }
    }
    return false;
}

template<class Index>
void ddl_item<Index>::begin(drop_down_list<Index>& list, Index const& index)
{
    list_ = &list;
    selected_ = list.data_->has_internal_selection &&
        index == list.data_->internal_selection;

    context& ctx = *list.list_ctx_;

    region_id id = get_region_id(ctx);
    panel_.begin(*list.list_ctx_, "item", NOT_PADDED, NO_FLAGS, id,
        get_widget_state(ctx, id, true, false, selected_));
    if (ctx.event->type == GET_CONTENTS_EVENT)
    {
        ddl_get_contents_event<Index>& event =
            get_event<ddl_get_contents_event<Index> >(ctx);
        event.contents.push_back(index);
    }
    else if (detect_click(ctx, id, LEFT_BUTTON))
    {
        ddl_set_value_event<Index> e(list.id_, index);
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

template<class Index>
void ddl_item<Index>::end()
{
    if (active_)
    {
        panel_.end();
        active_ = false;
    }
}

bool do_drop_down_button(
    context& ctx,
    layout const& layout_spec = default_layout,
    flag_set flags = NO_FLAGS,
    region_id id = auto_id);

}

#endif
