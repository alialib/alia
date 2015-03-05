#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>
#include <alia/ui/system.hpp>
#include <alia/ui/library/controls.hpp>

namespace alia {

// DROP DOWNS

struct drop_down_button_renderer : simple_button_renderer
{};

struct default_drop_down_button_renderer : drop_down_button_renderer
{
    leaf_layout_requirements get_layout(ui_context& ctx) const
    {
        return get_box_control_layout(ctx, "drop-down-button");
    }
    void draw(
        ui_context& ctx, layout_box const& region,  widget_state state) const
    {
        if (!is_render_pass(ctx))
            return;

        caching_renderer cache;
        initialize_caching_control_renderer(
            ctx, cache, region, make_id(state));
        if (cache.needs_rendering())
        {
            box_control_renderer renderer(
                ctx, cache, "drop-down-button",
                state & ~WIDGET_FOCUSED);

            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);

            renderer.canvas().translate(
                SkScalarDiv(
                    renderer.content_region().size[0],
                    SkIntToScalar(2)),
                SkScalarDiv(
                    renderer.content_region().size[1],
                    SkIntToScalar(2)));
            renderer.canvas().rotate(90);

            {
                set_color(paint, renderer.style().fg_color);
                paint.setStyle(SkPaint::kFill_Style);
                SkScalar a =
                    SkScalarDiv(
                        renderer.content_region().size[0],
                        SkDoubleToScalar(1.8));
                SkPath path;
                path.incReserve(4);
                SkPoint p0;
                p0.fX = SkScalarMul(a, SkDoubleToScalar(-0.34));
                p0.fY = SkScalarMul(a, SkDoubleToScalar(-0.5));
                path.moveTo(p0);
                SkPoint p1;
                p1.fX = p0.fX;
                p1.fY = SkScalarMul(a, SkDoubleToScalar(0.5));
                path.lineTo(p1);
                SkPoint p2;
                p2.fX = p0.fX + SkScalarMul(a, SkDoubleToScalar(0.866));
                p2.fY = 0;
                path.lineTo(p2);
                path.lineTo(p0);
                renderer.canvas().drawPath(path, paint);
            }

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }
};

typedef simple_button_data drop_down_button_data;

static bool
do_drop_down_button(
    ui_context& ctx,
    layout const& layout_spec,
    widget_id id,
    drop_down_button_data& data)
{
    return do_simple_button<drop_down_button_renderer,
        default_drop_down_button_renderer>(ctx, layout_spec, id, &data);
}

struct ddl_data
{
    popup_positioning positioning;

    // When the list is open, it may maintain a separate internal selection.
    // The internal selection can be copied into the actual control state
    // when the list is closed.
    optional<int> internal_selection;

    drop_down_button_data button;

    focus_rect_data focus_rendering;

    bool make_selection_visible;

    ddl_data() : make_selection_visible(false) {}
};

// ddl_list_query_event is used to query the drop down list to determine how
// many items there are and which is selected.
struct ddl_list_query_event : ui_event
{
    ddl_list_query_event(widget_id target)
      : ui_event(NO_CATEGORY, CUSTOM_EVENT)
      , target(target)
      , total_items(0)
    {}
    widget_id target;
    // query results
    int total_items;
    optional<int> selected_index;
};

static optional<int>
get_ddl_selected_index(dataless_ui_context& ctx, widget_id ddl_id)
{
    ddl_list_query_event event(ddl_id);
    issue_targeted_event(*ctx.system, event,
        make_routable_widget_id(ctx, ddl_id));
    return event.selected_index;
}

static int get_ddl_item_count(dataless_ui_context& ctx, widget_id ddl_id)
{
    ddl_list_query_event event(ddl_id);
    issue_targeted_event(*ctx.system, event,
        make_routable_widget_id(ctx, ddl_id));
    return event.total_items;
}

// ddl_select_index_event is used to select the item at the given index.
struct ddl_select_index_event : ui_event
{
    ddl_select_index_event(widget_id target, int index)
      : ui_event(NO_CATEGORY, CUSTOM_EVENT)
      , target(target)
      , index(index)
    {}
    widget_id target;
    int index;
};

static void select_ddl_item_at_index(
    dataless_ui_context& ctx, widget_id ddl_id, int index)
{
    ddl_select_index_event event(ddl_id, index);
    issue_targeted_event(*ctx.system, event,
        make_routable_widget_id(ctx, ddl_id));
}

static int clamp_ddl_index(
    dataless_ui_context& ctx, widget_id ddl_id, int index)
{
    int item_count = get_ddl_item_count(ctx, ddl_id);
    return clamp(index, 0, item_count > 0 ? item_count - 1 : 0);
}

static bool process_ddl_movement_keys(
    dataless_ui_context& ctx, widget_id ddl_id,
    optional<int>& selected_index, key_event_info const& info)
{
    if (info.mods == 0)
    {
        switch (info.code)
        {
         case KEY_UP:
            if (selected_index)
            {
                selected_index =
                    clamp_ddl_index(ctx, ddl_id, get(selected_index) - 1);
            }
            return true;
         case KEY_DOWN:
            selected_index = selected_index ?
                clamp_ddl_index(ctx, ddl_id, get(selected_index) + 1) : 0;
            return true;
         case KEY_PAGEUP:
            if (selected_index)
            {
                selected_index =
                    clamp_ddl_index(ctx, ddl_id, get(selected_index) - 10);
            }
            return true;
         case KEY_PAGEDOWN:
            selected_index = selected_index ?
                clamp_ddl_index(ctx, ddl_id, get(selected_index) + 10) : 0;
            return true;
         case KEY_HOME:
            selected_index = 0;
            return true;
         case KEY_END:
          {
            int item_count = get_ddl_item_count(ctx, ddl_id);
            selected_index = item_count > 0 ? item_count - 1 : 0;
            return true;
          }
        }
    }
    return false;
}

static void open_ddl(dataless_ui_context& ctx, ddl_data& data, widget_id id,
    layout_box const& bounding_region)
{
    data.internal_selection = get_ddl_selected_index(ctx, id);
    data.make_selection_visible = true;

    // Calculate popup positioning.
    layout_vector lower = bounding_region.corner;
    layout_vector upper = get_high_corner(bounding_region);
    data.positioning.lower_bound = make_vector(lower[0], upper[1]);
    data.positioning.upper_bound = make_vector(upper[0], lower[1]);
    data.positioning.absolute_lower = layout_vector(
        transform(get_transformation(ctx),
            vector<2,double>(data.positioning.lower_bound) +
            make_vector<double>(0.5, 0.5)));
    data.positioning.absolute_upper = layout_vector(
        transform(get_transformation(ctx),
            vector<2,double>(data.positioning.upper_bound) +
            make_vector<double>(0.5, 0.5)));
    data.positioning.minimum_size = bounding_region.size;

    set_active_overlay(ctx, id);
}

static void close_ddl(dataless_ui_context& ctx, ddl_data& data, widget_id id)
{
    ctx.system->overlay_id = null_widget_id;
}

untyped_ui_value const*
untyped_drop_down_list::begin(ui_context& ctx, layout const& layout_spec,
    ddl_flag_set flags)
{
    ctx_ = &ctx;
    flags_ = flags;

    untyped_ui_value const* result = 0;

    id_ = get_widget_id(ctx);
    list_index_ = 0;

    get_cached_data(ctx, &data_);
    ddl_data& data = *data_;

    widget_state state = get_button_state(ctx, id_, data.button.input);

    container_.begin(ctx, text("drop-down"),
        add_default_size(add_default_padding(add_default_alignment(
            layout_spec, LEFT, BASELINE_Y), PADDED), size(10, 1, EM)),
        PANEL_HORIZONTAL | PANEL_NO_INTERNAL_PADDING, id_, state);

    switch (ctx.event->category)
    {
     case INPUT_CATEGORY:
      {
        key_event_info info;
        if (detect_key_press(ctx, &info, id_))
        {
            if (!is_overlay_active(ctx, id_))
            {
                // If this is a list of commands, don't select them without the
                // list being open.
                if (!(flags & DDL_COMMAND_LIST))
                {
                    optional<int> selection = get_ddl_selected_index(ctx, id_);
                    if (process_ddl_movement_keys(ctx, id_, selection, info))
                    {
                        acknowledge_input_event(ctx);
                        if (selection)
                            select_ddl_item_at_index(ctx, id_, get(selection));
                    }
                }
            }
            else
            {
                if (process_ddl_movement_keys(ctx, id_,
                        data.internal_selection, info))
                {
                    data.make_selection_visible = true;
                    acknowledge_input_event(ctx);
                }
            }
            if (info.mods == 0)
            {
                switch (info.code)
                {
                 case KEY_ENTER:
                    if (!is_overlay_active(ctx, id_))
                    {
                        open_ddl(ctx, data, id_, container_.outer_region());
                        acknowledge_input_event(ctx);
                        break;
                    }
                 case KEY_SPACE:
                    if (is_overlay_active(ctx, id_))
                    {
                        if (data.internal_selection)
                        {
                            select_ddl_item_at_index(ctx, id_,
                                get(data.internal_selection));
                        }
                        close_ddl(ctx, data, id_);
                        acknowledge_input_event(ctx);
                    }
                    break;
                 case KEY_ESCAPE:
                    if (is_overlay_active(ctx, id_))
                    {
                        acknowledge_input_event(ctx);
                        close_ddl(ctx, data, id_);
                        acknowledge_input_event(ctx);
                    }
                    break;
                }
            }
        }
        break;
      }

     case NO_CATEGORY:
        switch (ctx.event->type)
        {
         case SET_VALUE_EVENT:
          {
            set_value_event& e = get_event<set_value_event>(ctx);
            if (e.target == id_)
            {
                result = e.value.get();
                close_ddl(ctx, data, id_);
            }
            break;
          }
        }
        break;
    }

    if (!(flags_ & DDL_COMMAND_LIST))
        contents_.begin(ctx, BASELINE_Y | GROW_X);

    return result;
}

bool untyped_drop_down_list::do_list()
{
    ui_context& ctx = *ctx_;
    ddl_data& data = *data_;

    if (flags_ & DDL_COMMAND_LIST)
    {
        if (do_icon_button(ctx, MENU_ICON, CENTER_X | BASELINE_Y, id_))
        {
            open_ddl(ctx, data, id_, container_.outer_region());
            end_pass(ctx);
        }
    }
    else
    {
        contents_.end();

        if (do_drop_down_button(ctx, CENTER_X | BASELINE_Y, id_, data.button))
        {
            open_ddl(ctx, data, id_, container_.outer_region());
            end_pass(ctx);
        }
    }

    // Well, this isn't quite the right condition. The list isn't relevant
    // in all passes where the ID has focus. However, it IS relevant in some
    // passes when the ID isn't the active overlay (passes that are meant to
    // query information about the list items). So this is overly conservative,
    // but there doesn't seem to be any harm in that.
    bool active = id_has_focus(ctx, id_);

    alia_if (active)
    {
        popup_.begin(ctx, id_, data.positioning);
        list_panel_.begin(ctx, text("drop-down-list"),
            layout(width(10, EM), UNPADDED),
            PANEL_NO_HORIZONTAL_SCROLLING | PANEL_NO_INTERNAL_PADDING);
    }
    alia_end

    return active;
}

void untyped_drop_down_list::end()
{
    if (ctx_)
    {
        list_panel_.end();
        popup_.end();

        container_.end();

        ctx_ = 0;
    }
}

bool untyped_ddl_item::begin(untyped_drop_down_list& list, bool is_selected)
{
    list_ = &list;
    int index = list.list_index_++;
    ddl_data& data = *list.data_;

    bool is_internally_selected = data.internal_selection &&
        get(data.internal_selection) == index;

    ui_context& ctx = *list.ctx_;

    widget_id id = get_widget_id(ctx);
    panel_.begin(ctx, text("item"), UNPADDED,
        PANEL_NO_INTERNAL_PADDING | PANEL_NO_CLICK_DETECTION, id,
        get_widget_state(ctx, id,
            is_internally_selected ? WIDGET_SELECTED : NO_FLAGS));
    layout_.begin(ctx);

    if (data.make_selection_visible && is_internally_selected)
    {
        make_widget_visible(ctx, id, MAKE_WIDGET_VISIBLE_ABRUPTLY);
        data.make_selection_visible = false;
    }

    switch (ctx.event->category)
    {
     case INPUT_CATEGORY:
        if (detect_click(ctx, id, LEFT_BUTTON))
            return true;
        break;

     case NO_CATEGORY:
        switch (ctx.event->type)
        {
         case CUSTOM_EVENT:
            {
                ddl_list_query_event* query =
                    dynamic_cast<ddl_list_query_event*>(ctx.event);
                if (query && query->target == list.id_)
                {
                    if (is_selected)
                        query->selected_index = index;
                    ++query->total_items;
                }
            }
            {
                ddl_select_index_event* event =
                    dynamic_cast<ddl_select_index_event*>(ctx.event);
                if (event && event->target == list.id_ &&
                    event->index == index)
                {
                    return true;
                }
            }
            break;
        }
        break;
    }

    return false;
}
void untyped_ddl_item::end()
{
    if (list_)
    {
        ui_context& ctx = *list_->ctx_;
        layout_.end();
        panel_.end();
        list_ = 0;
    }
}
void untyped_ddl_item::select(untyped_ui_value* value)
{
    set_value_event e(list_->id_, value);
    issue_targeted_event(*list_->ctx_->system, e,
        make_routable_widget_id(*list_->ctx_, list_->id_));
    end_pass(*list_->ctx_);
}

}
