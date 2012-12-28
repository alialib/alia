#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

// BORDERED BOX

void bordered_box::begin(
    ui_context& ctx, layout const& layout_spec, ui_flag_set flags)
{
    box_.begin(ctx, (flags & HORIZONTAL) ? 0 : 1, layout_spec);
    if (is_render_pass(ctx))
    {
        ctx.surface->draw_filled_box(ctx.style.properties->text_color,
            box<2,double>(add_border(box_.region(), get_padding_size(ctx))));
    }
}
void bordered_box::end()
{
    box_.end();
}

// PANELS

struct panel_data
{
    caching_renderer_data rendering;
};

static void begin_panel(
    ui_context& ctx, column_layout& outer, widget_id id, ui_flag_set flags,
    panel_data& data)
{
    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        break;

     case RENDER_CATEGORY:
      {
        if (flags & ROUNDED)
        {
            layout_box const& rect = outer.region();
            caching_renderer cache(ctx, data.rendering, *ctx.style.id, rect);
            if (cache.needs_rendering())
            {
                int padding = get_padding_size(ctx)[0];
                skia_renderer renderer(ctx, cache.image(), rect.size);
                SkPaint paint;
                paint.setFlags(SkPaint::kAntiAlias_Flag);
                set_color(paint, ctx.style.properties->background_color);
                SkRect sr;
                sr.fLeft = 0;
                sr.fRight = layout_scalar_as_skia_scalar(rect.size[0]);
                sr.fTop = 0;
                sr.fBottom = layout_scalar_as_skia_scalar(rect.size[1]);
                if (flags & ROUNDED)
                {
                    SkScalar radius =
                        layout_scalar_as_skia_scalar(padding * 2);
                    renderer.canvas().drawRoundRect(sr, radius, radius, paint);
                }
                else
                    renderer.canvas().drawRect(sr, paint);
                renderer.cache();
                cache.mark_valid();
            }
            cache.draw();
        }
        else
        {
            ctx.surface->draw_filled_box(
                ctx.style.properties->background_color,
                box<2,double>(outer.region()));
        }
        break;
      }

     case REGION_CATEGORY:
        // So the panel will block mouse events on things behind it.
        do_box_region(ctx, id, outer.region());
        break;

     case INPUT_CATEGORY:
        // So the panel will steal the focus if clicked on.
        if (!(flags && NO_CLICK_DETECTION) &&
            ctx.event->type == MOUSE_PRESS_EVENT && is_region_hot(ctx, id))
        {
            set_focus(ctx, id);
        }
        break;
    }
}

void panel::begin(
    ui_context& ctx, getter<string> const& style,
    layout const& layout_spec, ui_flag_set flags, widget_id id,
    widget_state state)
{
    ctx_ = &ctx;
    flags_ = flags;

    panel_data* data;
    get_cached_data(ctx, &data);

    init_optional_widget_id(ctx, id, data);

    outer_.begin(ctx, add_default_padding(layout_spec, PADDED));

    substyle_.begin(ctx, style, state);

    begin_panel(ctx, outer_, id, flags, *data);

    layout_flag_set inner_layout_flags =
        FILL_X |
        ((layout_spec.flags & Y_ALIGNMENT_MASK) == BASELINE_Y ?
            BASELINE_Y : FILL_Y) |
        ((flags & NO_INTERNAL_PADDING) ? UNPADDED : PADDED);
    inner_.begin(ctx, (flags & HORIZONTAL) ? 0 : 1,
	layout(inner_layout_flags, 1));
}
void panel::end()
{
    if (ctx_)
    {
        inner_.end();
        substyle_.end();
        outer_.end();
        ctx_ = 0;
    }
}

layout_box panel::outer_region() const
{
    // outer_.region() returns its region in its own frame of reference, which
    // isn't valid for panel users, so this is used instead.
    if (flags_ & NO_INTERNAL_PADDING)
        return inner_.region();
    else
        return add_border(inner_.region(), get_padding_size(*ctx_));
}

struct clickable_panel_data
{
    button_input_state input;
    caching_renderer_data rendering;
};

static void draw_panel_focus_border(
    ui_context& ctx, panel& p, ui_flag_set flags,
    caching_renderer_data& rendering)
{
    if (!(flags & NO_FOCUS_INDICATOR))
    {
        layout_box rect = p.outer_region();
        if (flags & ROUNDED)
        {
            caching_renderer cache(ctx, rendering, *ctx.style.id,
                add_border(rect, get_padding_size(ctx)));
            if (cache.needs_rendering())
            {
                layout_vector const& padding = get_padding_size(ctx);
                skia_renderer renderer(ctx, cache.image(),
                    rect.size + padding * 2);
                SkPaint paint;
                paint.setFlags(SkPaint::kAntiAlias_Flag);
                setup_focus_drawing(ctx, paint);
                renderer.canvas().translate(
                    layout_scalar_as_skia_scalar(padding[0]),
                    layout_scalar_as_skia_scalar(padding[1]));
                SkRect sr;
                sr.fLeft = 0;
                sr.fRight = layout_scalar_as_skia_scalar(rect.size[0]);
                sr.fTop = 0;
                sr.fBottom = layout_scalar_as_skia_scalar(rect.size[1]);
                SkScalar radius = layout_scalar_as_skia_scalar(padding[0] * 2);
                renderer.canvas().drawRoundRect(sr, radius, radius, paint);
                renderer.cache();
                cache.mark_valid();
            }
            cache.draw();
        }
        else
            draw_focus_rect(ctx, rendering, rect);
    }
}

void clickable_panel::begin(
    ui_context& ctx, layout const& layout_spec,
    ui_flag_set flags, widget_id id)
{
    ALIA_GET_CACHED_DATA(clickable_panel_data)

    get_widget_id_if_needed(ctx, id);

    widget_state state = get_button_state(ctx, id, data.input);
    panel_.begin(ctx, text("clickable_panel"), layout_spec, flags, id, state);
    if ((flags & DISABLED) == 0)
    {
        clicked_ = do_button_input(ctx, id, data.input);
        if (is_render_pass(ctx) && (state & WIDGET_FOCUSED))
            draw_panel_focus_border(ctx, panel_, flags, data.rendering);
    }
    else
        clicked_ = false;
}

void scrollable_panel::begin(
    ui_context& ctx, getter<string> const& style,
    unsigned scrollable_axes,
    layout const& layout_spec, ui_flag_set flags)
{
    widget_id id = get_widget_id(ctx);
    outer_.begin(ctx, layout_spec);
    substyle_.begin(ctx, style, WIDGET_NORMAL);
    panel_data* data;
    get_cached_data(ctx, &data);
    begin_panel(ctx, outer_, id, flags, *data);
    region_.begin(ctx, GROW | UNPADDED, scrollable_axes, id);
    inner_.begin(ctx, (flags & HORIZONTAL) ? 0 : 1, GROW | PADDED);
}
void scrollable_panel::end()
{
    inner_.end();
    region_.end();
    substyle_.end();
    outer_.end();
}

// TREE NODE

struct tree_node_data
{
    bool expanded;
};

void tree_node::begin(
    ui_context& ctx,
    layout const& layout_spec,
    ui_flag_set flags,
    optional_storage<bool> const& expanded,
    widget_id expander_id)
{
    ctx_ = &ctx;

    tree_node_data* data;
    if (get_data(ctx, &data))
    {
        if (flags & INITIALLY_EXPANDED)
            data->expanded = true;
        else
            data->expanded = false;
    }

    accessor_mux<indirect_accessor<bool>,inout_accessor<bool> > state =
        resolve_storage(expanded, &data->expanded);

    grid_.begin(ctx, layout_spec);
    row_.begin(grid_);
    do_children_ = state.is_gettable() ? state.get() : false;
    expander_result_ =
        do_node_expander(ctx, state, default_layout, expander_id);
    label_region_.begin(ctx, layout(GROW));
}

void tree_node::end_header()
{
    label_region_.end();
}

bool tree_node::do_children()
{
    ui_context& ctx = *ctx_;
    end_header();
    row_.end();
    alia_if(do_children_)
    {
        row_.begin(grid_, layout(GROW));
        do_spacer(ctx);
        column_.begin(ctx, layout(GROW));
        return true;
    }
    alia_end
    return false;
}

void tree_node::end()
{
    column_.end();
    row_.end();
    grid_.end();
}

struct draggable_separator_data
{
    keyed_data<float> width;
    layout_leaf layout_node;
    caching_renderer_data rendering;
    int drag_start_delta;
};

bool do_draggable_separator(ui_context& ctx, accessor<int> const& width,
    layout const& layout_spec, ui_flag_set flags, widget_id id)
{
    ALIA_GET_CACHED_DATA(draggable_separator_data)

    get_widget_id_if_needed(ctx, id);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        refresh_keyed_data(data.width, *ctx.style.id);
        if (!data.width.is_valid)
            set(data.width, get_float_property(ctx, "separator_width", 2));
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec,
            leaf_layout_requirements(
                make_layout_vector(
                    as_layout_size(data.width.value),
                    as_layout_size(data.width.value)),
                0, 0),
            FILL | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        layout_box const& region = data.layout_node.assignment().region;
        caching_renderer cache(ctx, data.rendering, *ctx.style.id, region);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), region.size);
            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);
            paint.setStrokeWidth(2);
            paint.setStrokeCap(SkPaint::kRound_Cap);
            set_color(paint, get_color_property(ctx, "separator_color"));
            renderer.canvas().drawLine(
                SkIntToScalar(1), SkIntToScalar(1),
                layout_scalar_as_skia_scalar(region.size[0] - 1),
                layout_scalar_as_skia_scalar(region.size[1] - 1),
                paint);
            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
        break;
      }

     case REGION_CATEGORY:
      {
        layout_box region = data.layout_node.assignment().region;
        int axis = (flags & HORIZONTAL) ? 0 : 1;
        int const drag_axis = 1 - axis;
        // Add a couple of pixels to make it easier to click on.
        region.corner[drag_axis] -= 1;
        region.size[drag_axis] += 2;
        do_box_region(ctx, id, region, flags & HORIZONTAL ?
            UP_DOWN_ARROW_CURSOR : LEFT_RIGHT_ARROW_CURSOR);
        break;
      }

     case INPUT_CATEGORY:
      {
        int axis = (flags & HORIZONTAL) ? 0 : 1;
        int const drag_axis = 1 - axis;

        if (detect_mouse_press(ctx, id, LEFT_BUTTON))
        {
            int position = get_integer_mouse_position(ctx)[drag_axis];
            int current_width = width.is_gettable() ? get(width) : 0;
            data.drag_start_delta = (flags & FLIPPED) ?
                current_width + position : position - current_width;
        }
        if (detect_drag(ctx, id, LEFT_BUTTON))
        {
            int position = get_integer_mouse_position(ctx)[drag_axis];
            set(width, (flags & FLIPPED) ?
                data.drag_start_delta - position :
                position - data.drag_start_delta);
            return true;
        }
        break;
      }
    }
    return false;
}

void resizable_content::begin(
    ui_context& ctx, accessor<int> const& size, ui_flag_set flags)
{
    ctx_ = &ctx;
    id_ = get_widget_id(ctx);
    flags_ = flags;
    size_ = get(size);

    if (flags & PREPEND)
        do_draggable_separator(ctx, size, UNPADDED, flags | FLIPPED);

    if (flags & HORIZONTAL)
        layout_.begin(ctx, 1, alia::height(float(size_), PIXELS));
    else
        layout_.begin(ctx, 0, alia::width(float(size_), PIXELS));

    // It's possible that the content will be too big for the requested size
    // and the layout engine will force the container to a larger size.
    // If this happens, we have to record that as the real size.
    if (detect_event(ctx, MOUSE_HIT_TEST_EVENT))
    {
        unsigned drag_axis = (flags & HORIZONTAL) ? 1 : 0;
        int new_size = layout_.region().size[drag_axis];
        if (new_size != size_)
            set(size, new_size);
    }

    handle_set_value_events(ctx, id_, size);
}
void resizable_content::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;
        layout_.end();
        if (!(flags_ & PREPEND) && !ctx.pass_aborted)
        {
            if (do_draggable_separator(ctx, inout(&size_), UNPADDED, flags_))
                issue_set_value_event(ctx, id_, size_);
        }
        ctx_ = 0;
    }
}

void popup::begin(ui_context& ctx, widget_id id,
    layout_vector const& lower_bound, layout_vector const& upper_bound)
{
    ctx_ = &ctx;
    id_ = id;

    // Update the context's layer Z so that the popup is above other content.
    set_layer_z(ctx, ctx.layer_z + 1);

    vector<2,int> absolute_lower = vector<2,int>(
        transform(get_transformation(ctx),
            vector<2,double>(lower_bound) + make_vector<double>(0.5, 0.5)));
    vector<2,int> absolute_upper = vector<2,int>(
        transform(get_transformation(ctx),
            vector<2,double>(upper_bound) + make_vector<double>(0.5, 0.5)));

    layout_vector surface_size = layout_vector(ctx.surface->size());
    layout_vector maximum_size;
    for (unsigned i = 0; i != 2; ++i)
    {
        maximum_size[i] =
            (std::max)(absolute_upper[i], surface_size[i] - absolute_lower[i]);
    }

    layout_.begin(ctx, maximum_size);

    if (!is_refresh_pass(ctx))
    {
        vector<2,int> position;
        for (unsigned i = 0; i != 2; ++i)
        {
            if (absolute_lower[i] + layout_.size()[i] <= surface_size[i] ||
                surface_size[i] - absolute_lower[i] > absolute_upper[i])
            {
                position[i] = lower_bound[i];
            }
            else
            {
                position[i] = upper_bound[i] - layout_.size()[i];
            }
        }
        transform_.begin(*get_layout_traversal(ctx).geometry);
        transform_.set(translation_matrix(vector<2,double>(position)));
    }

    // Intercept mouse clicks and wheel movement to other parts of the surface.
    background_id_ = get_widget_id(ctx);
    handle_mouse_hit(ctx, background_id_, HIT_TEST_MOUSE | HIT_TEST_WHEEL);
}
bool popup::user_closed()
{
    ui_context& ctx = *ctx_;
    return
        detect_mouse_press(ctx, background_id_, LEFT_BUTTON) || 
        detect_mouse_press(ctx, background_id_, MIDDLE_BUTTON) || 
        detect_mouse_press(ctx, background_id_, RIGHT_BUTTON) ||
        detect_focus_loss(ctx, id_);
}
void popup::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;

        transform_.end();
	layout_.end();

        // Restore the context's layer Z.
	set_layer_z(ctx, ctx.layer_z - 1);

	ctx_ = 0;
    }
}

}
