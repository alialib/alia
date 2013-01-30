#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>
#include <alia/ui/system.hpp>

namespace alia {

// DROP DOWNS

struct drop_down_button_renderer : dispatch_interface
{
    virtual layout_vector default_size(ui_context& ctx) const = 0;

    virtual void draw(
        ui_context& ctx, renderer_data_ptr& data_ptr, layout_box const& region,
        widget_state state) const = 0;
};

struct drop_down_button_data
{
    layout_leaf layout_node;
    themed_rendering_data<drop_down_button_renderer> rendering;
    button_input_state input;
};

struct default_drop_down_button_renderer : drop_down_button_renderer
{
    layout_vector default_size(ui_context& ctx) const
    {
        return as_layout_size(resolve_absolute_size(
            get_layout_traversal(ctx), size(1.f, 1.f, EM)));
    }
    void draw(
        ui_context& ctx, renderer_data_ptr& data_ptr, layout_box const& region,
        widget_state state) const
    {
        caching_renderer_data* data;
        cast_data_ptr(&data, data_ptr);

        layout_vector const& padding_size = get_padding_size(ctx);
        layout_box padded_region = add_border(region, padding_size);
        layout_vector const& unpadded_size = region.size;

        caching_renderer cache(ctx, *data,
            combine_ids(ref(*ctx.style.id), make_id(state)),
            padded_region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), padded_region.size);

            control_style_path_storage storage;
            style_search_path const* path =
                get_control_style_path(ctx, &storage, "drop-down-button",
                    state);

            rgba8 bg_color = get_color_property(path, "background");
            rgba8 fg_color = get_color_property(path, "color");

            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);

            renderer.canvas().translate(
                layout_scalar_as_skia_scalar(padding_size[0]),
                layout_scalar_as_skia_scalar(padding_size[1]));

            paint.setStyle(SkPaint::kFill_Style);
            set_color(paint, bg_color);
            draw_rect(renderer.canvas(), paint, unpadded_size);

            renderer.canvas().translate(
                SkScalarDiv(
                    layout_scalar_as_skia_scalar(unpadded_size[0]),
                    SkIntToScalar(2)),
                SkScalarDiv(
                    layout_scalar_as_skia_scalar(unpadded_size[1]),
                    SkIntToScalar(2)));
            renderer.canvas().rotate(90);

            {
                set_color(paint, fg_color);
                paint.setStyle(SkPaint::kFill_Style);
                SkScalar a =
                    SkScalarDiv(
                        layout_scalar_as_skia_scalar(unpadded_size[0]),
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

static bool
do_drop_down_button(
    ui_context& ctx,
    layout const& layout_spec,
    widget_id id,
    drop_down_button_data& data)
{
    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        static default_drop_down_button_renderer default_renderer;
        refresh_themed_rendering_data(ctx, data.rendering, &default_renderer);
        drop_down_button_renderer const* renderer = data.rendering.renderer;
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx),
            layout_spec,
            leaf_layout_requirements(renderer->default_size(ctx), 0, 0),
            LEFT | CENTER_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        data.rendering.renderer->draw(ctx, data.rendering.data,
            data.layout_node.assignment().region,
            get_button_state(ctx, id, data.input));
        break;
      }

     case REGION_CATEGORY:
        do_box_region(ctx, id, data.layout_node.assignment().region);
        break;

     case INPUT_CATEGORY:
        add_to_focus_order(ctx, id);
        if (detect_click(ctx, id, LEFT_BUTTON) ||
            detect_keyboard_click(ctx, data.input.key, id, KEY_SPACE))
        {
            return true;
        }
        break;
    }

    return false;
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

static optional<int> get_ddl_selected_index(ui_context& ctx, widget_id ddl_id)
{
    ddl_list_query_event event(ddl_id);
    issue_targeted_event(*ctx.system, event,
        make_routable_widget_id(ctx, ddl_id));
    return event.selected_index;
}

static int get_ddl_item_count(ui_context& ctx, widget_id ddl_id)
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

static void select_ddl_item_at_index(ui_context& ctx, widget_id ddl_id,
    int index)
{
    ddl_select_index_event event(ddl_id, index);
    issue_targeted_event(*ctx.system, event,
        make_routable_widget_id(ctx, ddl_id));
}

static int clamp_ddl_index(ui_context& ctx, widget_id ddl_id, int index)
{
    int item_count = get_ddl_item_count(ctx, ddl_id);
    return clamp(index, 0, item_count > 0 ? item_count - 1 : 0);
}

static bool process_ddl_movement_keys(
    ui_context& ctx, widget_id ddl_id,
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

static void open_ddl(ui_context& ctx, ddl_data& data, widget_id id,
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

static void close_ddl(ui_context& ctx, ddl_data& data, widget_id id)
{
    ctx.system->overlay_id = null_widget_id;
}

untyped_ui_value const*
untyped_drop_down_list::begin(ui_context& ctx, layout const& layout_spec,
    ui_flag_set flags)
{
    ctx_ = &ctx;

    untyped_ui_value const* result = 0;

    id_ = get_widget_id(ctx);
    list_index_ = 0;

    get_cached_data(ctx, &data_);
    ddl_data& data = *data_;

    widget_state state = get_button_state(ctx, id_, data.button.input);

    container_.begin(ctx, text("control"),
        add_default_padding(
            add_default_alignment(layout_spec, LEFT, BASELINE_Y), PADDED),
        HORIZONTAL | SHOW_FOCUS | NO_INTERNAL_PADDING, id_, state);

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
                if (!(flags & COMMAND_LIST))
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
                 case KEY_NUMPAD_ENTER:
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

    contents_.begin(ctx, BASELINE_Y | GROW_X);

    return result;
}

bool untyped_drop_down_list::do_list()
{
    ui_context& ctx = *ctx_;
    ddl_data& data = *data_;

    contents_.end();

    if (do_drop_down_button(ctx, CENTER, id_, data.button))
    {
        open_ddl(ctx, data, id_, container_.outer_region());
        end_pass(ctx);
    }

    bool active = id_has_focus(ctx, id_);

    alia_if (active)
    {
        popup_.begin(ctx, id_, data.positioning);
        list_panel_.begin(ctx, text("drop-down-list"), GROW | UNPADDED,
            NO_HORIZONTAL_SCROLL | NO_INTERNAL_PADDING);
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
        NO_INTERNAL_PADDING | NO_CLICK_DETECTION,
        id, get_widget_state(ctx, id, true, false, is_internally_selected));

    if (data.make_selection_visible && is_internally_selected)
    {
        make_widget_visible(ctx, id);
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
