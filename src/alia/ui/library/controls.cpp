#include <alia/ui/api.hpp>
#include <alia/ui/library/controls.hpp>

#include <SkPath.h>

namespace alia {

style_search_path const*
get_control_style_path(dataless_ui_context& ctx,
    stateless_control_style_path_storage* storage,
    char const* control_type)
{
    return
        add_substyle_to_path(
            &storage->storage[1],
            ctx.style.path,
            add_substyle_to_path(&storage->storage[0], ctx.style.path, 0,
                "control"),
            control_type, ADD_SUBSTYLE_NO_PATH_SEPARATOR);
}

style_search_path const*
get_control_style_path(
    dataless_ui_context& ctx, control_style_path_storage* storage,
    char const* control_type, widget_state state)
{
    return
        add_substyle_to_path(
            &storage->storage[1],
            ctx.style.path,
            add_substyle_to_path(&storage->storage[0], ctx.style.path,
                ctx.style.path, "control", state),
            control_type, state, ADD_SUBSTYLE_NO_PATH_SEPARATOR);
}

control_style_properties
get_control_style_properties(
    dataless_ui_context& ctx, style_search_path const* path,
    layout_vector const& size)
{
    control_style_properties properties;

    properties.bg_color = get_color_property(path, "background");
    properties.fg_color = get_color_property(path, "color");
    properties.border_color = get_color_property(path, "border-color");

    properties.border_width =
        resolve_absolute_length(get_layout_traversal(ctx), 0,
            get_property(path, "border-width", UNINHERITED_PROPERTY,
                absolute_length(0, PIXELS)));

    box_corner_sizes border_radius_spec =
        get_border_radius_property(path, relative_length(0.25));
    properties.border_radii =
        resolve_box_corner_sizes(get_layout_traversal(ctx),
            border_radius_spec, vector<2,float>(size));

    return properties;
}

control_style_properties
get_control_style_properties(
    dataless_ui_context& ctx, char const* control_type, widget_state state,
    layout_vector const& size)
{
    control_style_path_storage storage;
    return get_control_style_properties(ctx,
        get_control_style_path(ctx, &storage, control_type, state),
        size);
}

leaf_layout_requirements
get_box_control_layout(ui_context& ctx, char const* control_type)
{
    ALIA_GET_CACHED_DATA(keyed_data<leaf_layout_requirements>)
    refresh_keyed_data(data, *ctx.style.id);
    if (!is_valid(data))
    {
        stateless_control_style_path_storage storage;
        style_search_path const* path =
            get_control_style_path(ctx, &storage, control_type);
        float border_width =
            resolve_absolute_length(get_layout_traversal(ctx), 0,
                get_property(path, "border-width", UNINHERITED_PROPERTY,
                    absolute_length(0, PIXELS)));
        layout_vector size =
            as_layout_size(
                resolve_absolute_size(get_layout_traversal(ctx),
                    get_property(path, "size", UNINHERITED_PROPERTY,
                        make_vector(
                            absolute_length(1.2f, EM),
                            absolute_length(1.2f, EM)))) +
                make_vector(border_width, border_width) * 2);
        layout_scalar descent =
            as_layout_size(
                resolve_absolute_length(get_layout_traversal(ctx), 0,
                    get_property(path, "descent", UNINHERITED_PROPERTY,
                        absolute_length(0, PIXELS))) +
                border_width);
        set(data, leaf_layout_requirements(size, size[1] - descent, descent));
    }
    return get(data);
}

skia_box
get_box_control_content_region(
    layout_box const& region, control_style_properties const& style)
{
    return add_border(layout_box_as_skia_box(region), -style.border_width);
}

void draw_box_control(
    dataless_ui_context& ctx, SkCanvas& canvas, layout_vector const& size,
    control_style_properties const& style, bool has_focus)
{
    SkPaint paint;
    paint.setFlags(SkPaint::kAntiAlias_Flag);

    skia_box full_region =
        layout_box_as_skia_box(layout_box(make_layout_vector(0, 0), size));

    set_color(paint, style.bg_color);
    paint.setStyle(SkPaint::kFill_Style);
    draw_rect(canvas, paint, full_region,
        adjust_border_radii_for_border_width(style.border_radii,
            box_border_width<float>(style.border_width)));

    if (style.border_width != 0 && style.border_color.a != 0)
    {
        set_color(paint, style.border_color);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(style.border_width);
        paint.setStrokeCap(SkPaint::kSquare_Cap);
        skia_box border_rect =
            add_border(full_region, SkFloatToScalar(-style.border_width / 2));
        draw_rect(canvas, paint, full_region, style.border_radii);
    }

    if (has_focus)
    {
        setup_focus_drawing(ctx, paint);
        draw_rect(canvas, paint, full_region, style.border_radii);
    }
}

void initialize_caching_control_renderer(
    ui_context& ctx, caching_renderer& cache, layout_box const& region,
    id_interface const& content_id)
{
    ALIA_GET_CACHED_DATA(caching_renderer_data);

    layout_box padded_region = add_border(region, get_padding_size(ctx));

    cache.begin(ctx, data,
        combine_ids(ref(&content_id), ref(ctx.style.id)),
        padded_region);
}

box_control_renderer::box_control_renderer(
    ui_context& ctx, caching_renderer& cache,
    char const* control_type, widget_state state)
  : renderer_(ctx, cache.image(), cache.region().size)
{
    style_path_ =
        get_control_style_path(ctx, &path_storage_, control_type, state);

    layout_box unpadded_region =
        add_border(cache.region(), -get_padding_size(ctx));

    style_ = get_control_style_properties(ctx, style_path_,
        unpadded_region.size);

    content_region_ = get_box_control_content_region(unpadded_region, style_);

    renderer_.canvas().translate(
        layout_scalar_as_skia_scalar(get_padding_size(ctx)[0]),
        layout_scalar_as_skia_scalar(get_padding_size(ctx)[1]));

    draw_box_control(ctx, renderer_.canvas(), unpadded_region.size,
        style_, (state & WIDGET_FOCUSED) ? true : false);

    renderer_.canvas().translate(
        SkFloatToScalar(style_.border_width),
        SkFloatToScalar(style_.border_width));
    content_region_.corner = make_vector(SkIntToScalar(0), SkIntToScalar(0));
}

// ICON BUTTON

// This can't use do_simple_button since it requires an extra parameter to the
// renderer (the icon type). So instead, it's implemented as a control.

struct icon_button_renderer : simple_control_renderer<icon_type>
{};

struct default_icon_button_renderer : icon_button_renderer
{
    leaf_layout_requirements get_layout(ui_context& ctx) const
    {
        return get_box_control_layout(ctx, "icon-button");
    }
    void draw(
        ui_context& ctx, layout_box const& region,
        accessor<icon_type> const& icon, widget_state state) const
    {
        if (!is_render_pass(ctx))
            return;

        caching_renderer cache;
        initialize_caching_control_renderer(ctx, cache, region,
            combine_ids(ref(&icon.id()), make_id(state)));
        if (cache.needs_rendering())
        {
            box_control_renderer renderer(ctx, cache, "icon-button", state);

            renderer.canvas().translate(
                renderer.content_region().size[0] / SkIntToScalar(2),
                renderer.content_region().size[1] / SkIntToScalar(2));

            SkPaint paint;
            paint.setAntiAlias(true);
            set_color(paint, renderer.style().fg_color);

            switch (get(icon))
            {
             case REMOVE_ICON:
              {
                SkScalar a =
                    renderer.content_region().size[0] / SkIntToScalar(4);
                paint.setStrokeWidth(a);
                paint.setStrokeCap(SkPaint::kRound_Cap);
                renderer.canvas().drawLine(-a, -a,  a,  a, paint);
                renderer.canvas().drawLine(-a,  a,  a, -a, paint);
                break;
              }
             case DRAG_ICON:
              {
                SkScalar const a =
                    renderer.content_region().size[0] / SkFloatToScalar(2.6f);
                SkScalar const b = a / SkIntToScalar(4);
                paint.setStrokeWidth(a / SkFloatToScalar(2.5f));
                paint.setStrokeCap(SkPaint::kRound_Cap);
                renderer.canvas().drawLine(-a,  0,      a,      0, paint);
                renderer.canvas().drawLine(-a,  0, -a + b,     -b, paint);
                renderer.canvas().drawLine(-a,  0, -a + b,      b, paint);
                renderer.canvas().drawLine( a,  0,  a - b,     -b, paint);
                renderer.canvas().drawLine( a,  0,  a - b,      b, paint);
                renderer.canvas().drawLine( 0, -a,      0,      a, paint);
                renderer.canvas().drawLine( 0, -a,     -b, -a + b, paint);
                renderer.canvas().drawLine( 0, -a,      b, -a + b, paint);
                renderer.canvas().drawLine( 0,  a,     -b,  a - b, paint);
                renderer.canvas().drawLine( 0,  a,      b,  a - b, paint);
                break;
              }
             case MENU_ICON:
              {
                SkScalar a =
                    renderer.content_region().size[0] / SkIntToScalar(4);
                SkScalar b =
                    renderer.content_region().size[0] / SkIntToScalar(4);
                SkScalar c =
                    renderer.content_region().size[0] / SkIntToScalar(5);
                paint.setStrokeWidth(c);
                paint.setStrokeCap(SkPaint::kRound_Cap);
                renderer.canvas().drawLine(-b,  0,  b,  0, paint);
                renderer.canvas().drawLine(-b,  a,  b,  a, paint);
                renderer.canvas().drawLine(-b, -a,  b, -a, paint);
                break;
              }
             case PLUS_ICON:
              {
                SkScalar a =
                    renderer.content_region().size[0] / SkIntToScalar(4);
                SkScalar c =
                    renderer.content_region().size[0] / SkIntToScalar(5);
                paint.setStrokeWidth(c);
                paint.setStrokeCap(SkPaint::kRound_Cap);
                renderer.canvas().drawLine(-a,  0,  a,  0, paint);
                renderer.canvas().drawLine(0, -a,  0,  a,  paint);
                break;
              }
             case MINUS_ICON:
              {
                SkScalar a =
                    renderer.content_region().size[0] / SkIntToScalar(4);
                SkScalar c =
                    renderer.content_region().size[0] / SkIntToScalar(5);
                paint.setStrokeWidth(c);
                paint.setStrokeCap(SkPaint::kRound_Cap);
                renderer.canvas().drawLine(-a,  0,  a,  0, paint);
                break;
              }
             case CONTOUR_ICON:
             {
                 SkScalar a = renderer.content_region().size[0] / SkIntToScalar(2);
                 SkScalar b = renderer.content_region().size[0] / SkIntToScalar(6);

                 paint.setStrokeWidth(3);
                 paint.setColor(SK_ColorBLACK);
                 paint.setStrokeCap(SkPaint::kRound_Cap);

                 //top
                 renderer.canvas().drawLine(-a, -a, a, -a, paint);
                 //upmid
                 renderer.canvas().drawLine(-a, -b, a, -b, paint);
                 //lowmid
                 renderer.canvas().drawLine(-a, b, a, b, paint);
                 //bottom
                 renderer.canvas().drawLine(-a, a, a, a, paint);

                 break;
             }
             case SOLID_ICON:
             {
                 // empty icon for use with the solid/contour structure render modes
                 break;
             }
             default:
                break;
            }

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }
};

icon_button_result
do_unsafe_icon_button(
    ui_context& ctx,
    icon_type icon,
    layout const& layout_spec,
    simple_control_flag_set flags,
    widget_id id)
{
    return
        do_simple_control<icon_button_renderer,default_icon_button_renderer>(
            ctx,
            in(icon),
            layout_spec,
            flags,
            id);
}

icon_button_result
do_unsafe_icon_button(
    ui_context& ctx,
    icon_type icon,
    accessor<string> const& tooltip,
    layout const& layout_spec,
    simple_control_flag_set flags,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    auto result = do_icon_button(ctx, icon, layout_spec, flags, id);
    set_tooltip_message(ctx, id, tooltip);
    return result;
}

void
do_icon_button(
    ui_context& ctx,
    icon_type icon,
    action const& on_press,
    layout const& layout_spec,
    simple_control_flag_set flags,
    widget_id id)
{
    if (do_unsafe_icon_button(
            ctx,
            icon,
            layout_spec,
            flags | (on_press.is_ready() ? NO_FLAGS : SIMPLE_CONTROL_DISABLED),
            id))
    {
        perform_action(on_press);
        end_pass(ctx);
    }

}

void
do_icon_button(
    ui_context& ctx,
    icon_type icon,
    accessor<string> const& tooltip,
    action const& on_press,
    layout const& layout_spec,
    simple_control_flag_set flags,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    do_icon_button(ctx, icon, on_press, layout_spec, flags, id);
    set_tooltip_message(ctx, id, tooltip);
}

// CHECK BOX

struct check_box_renderer : simple_control_renderer<bool>
{};

struct default_check_box_renderer : check_box_renderer
{
    leaf_layout_requirements get_layout(ui_context& ctx) const
    {
        return get_box_control_layout(ctx, "check-box");
    }
    void draw(
        ui_context& ctx, layout_box const& region,
        accessor<bool> const& value, widget_state state) const
    {
        if (!is_render_pass(ctx))
            return;

        caching_renderer cache;
        initialize_caching_control_renderer(ctx, cache, region,
            combine_ids(ref(&value.id()), make_id(state)));
        if (cache.needs_rendering())
        {
            box_control_renderer renderer(ctx, cache, "check-box", state);

            if (value.is_gettable() && get(value))
            {
                SkPaint paint;
                paint.setFlags(SkPaint::kAntiAlias_Flag);
                set_color(paint, renderer.style().fg_color);
                paint.setStrokeCap(SkPaint::kRound_Cap);
                SkScalar dx =
                    renderer.content_region().size[0] / SkIntToScalar(10);
                SkScalar dy =
                    renderer.content_region().size[1] / SkIntToScalar(10);
                paint.setStrokeWidth(SkScalarMul(dx, SkDoubleToScalar(1.6)));
                renderer.canvas().drawLine(
                    SkScalarMul(dx, SkIntToScalar(3)),
                    SkScalarMul(dy, SkIntToScalar(3)),
                    SkScalarMul(dx, SkIntToScalar(7)),
                    SkScalarMul(dy, SkIntToScalar(7)),
                    paint);
                renderer.canvas().drawLine(
                    SkScalarMul(dx, SkIntToScalar(3)),
                    SkScalarMul(dy, SkIntToScalar(7)),
                    SkScalarMul(dx, SkIntToScalar(7)),
                    SkScalarMul(dy, SkIntToScalar(3)),
                    paint);
            }

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }
};

check_box_result
do_unsafe_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec,
    simple_control_flag_set flags,
    widget_id id)
{
    check_box_result result;
    if (do_simple_control<check_box_renderer,default_check_box_renderer>(
        ctx, value, layout_spec, flags, id))
    {
        result.changed = true;
        set(value, value.is_gettable() ? !value.get() : true);
    }
    else
        result.changed = false;
    return result;
}

check_box_result
do_unsafe_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& text,
    layout const& layout_spec,
    simple_control_flag_set flags,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    row_layout row(ctx, add_default_y_alignment(layout_spec, BASELINE_Y));
    check_box_result result =
        do_unsafe_check_box(ctx, value, default_layout, flags, id);
    do_paragraph(ctx, text, GROW_X);
    do_box_region(ctx, id, row.region());
    return result;
}

check_box_result
do_unsafe_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& text,
    accessor<string> const& tooltip,
    layout const& layout_spec,
    simple_control_flag_set flags,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    auto result = do_unsafe_check_box(ctx, value, text, layout_spec, flags, id);
    set_tooltip_message(ctx, id, tooltip);
    return result;
}

// RADIO BUTTON

struct radio_button_renderer : simple_control_renderer<bool>
{};

struct default_radio_button_renderer : radio_button_renderer
{
    leaf_layout_requirements get_layout(ui_context& ctx) const
    {
        return get_box_control_layout(ctx, "radio-button");
    }
    void draw(
        ui_context& ctx, layout_box const& region,
        accessor<bool> const& value, widget_state state) const
    {
        if (!is_render_pass(ctx))
            return;

        caching_renderer cache;
        initialize_caching_control_renderer(ctx, cache, region,
            combine_ids(ref(&value.id()), make_id(state)));
        if (cache.needs_rendering())
        {
            box_control_renderer renderer(ctx, cache, "radio-button", state);

            if (value.is_gettable() && get(value))
            {
                SkPaint paint;
                paint.setFlags(SkPaint::kAntiAlias_Flag);
                set_color(paint, renderer.style().fg_color);
                paint.setStyle(SkPaint::kFill_Style);
                draw_rect(renderer.canvas(), paint,
                    add_border(renderer.content_region(),
                        -make_vector(
                            renderer.content_region().size[0] / SkIntToScalar(4),
                            renderer.content_region().size[1] / SkIntToScalar(4))),
                    renderer.style().border_radii);
            }

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }
};

radio_button_result
do_unsafe_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec,
    simple_control_flag_set flags,
    widget_id id)
{
    radio_button_result result;
    if (do_simple_control<radio_button_renderer,
        default_radio_button_renderer>(ctx, value, layout_spec, flags, id))
    {
        result.changed = true;
        set(value, true);
    }
    else
        result.changed = false;
    return result;
}

radio_button_result
do_unsafe_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& text,
    layout const& layout_spec,
    simple_control_flag_set flags,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    row_layout row(ctx, add_default_y_alignment(layout_spec, BASELINE_Y));
    radio_button_result result =
        do_unsafe_radio_button(ctx, value, default_layout, flags, id);
    do_paragraph(ctx, text, GROW_X);
    do_box_region(ctx, id, row.region());
    return result;
}

radio_button_result
do_unsafe_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& text,
    accessor<string> const& tooltip,
    layout const& layout_spec,
    simple_control_flag_set flags,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    row_layout row(ctx, add_default_y_alignment(layout_spec, BASELINE_Y));
    radio_button_result result =
        do_unsafe_radio_button(ctx, value, default_layout, flags, id);
    do_paragraph(ctx, text, GROW_X);
    do_box_region(ctx, id, row.region());
    set_tooltip_message(ctx, id, tooltip);
    return result;
}

radio_button_result
do_unsafe_radio_button_with_description(
    ui_context& ctx,
    accessor<bool> const& value,
    accessor<string> const& label,
    accessor<string> const& description,
    layout const& layout_spec,
    simple_control_flag_set flags,
    widget_id id)
{
    column_layout box(ctx); // just here to record region
    widget_id radio_id = get_widget_id(ctx);
    do_box_region(ctx, radio_id, box.region());
    radio_button_result result;
    {
        row_layout row(ctx);
        result = do_unsafe_radio_button(ctx, value, default_layout, flags, radio_id);
        {
            column_layout col(ctx, GROW);
            do_styled_text(ctx, text("radio-label"), label);
            do_paragraph(ctx, description);
        }
    }
    return result;
}

// NODE EXPANDER

struct node_expander_renderer : simple_control_renderer<bool>
{};

struct default_node_expander_renderer : node_expander_renderer
{
    leaf_layout_requirements get_layout(ui_context& ctx) const
    {
        return get_box_control_layout(ctx, "node-expander");
    }
    void draw(
        ui_context& ctx, layout_box const& region,
        accessor<bool> const& value, widget_state state) const
    {
        double angle =
            smooth_raw_value(ctx, value.is_gettable() && get(value) ? 90. : 0.,
                animated_transition(linear_curve, 200));

        if (!is_render_pass(ctx))
            return;

        caching_renderer cache;
        initialize_caching_control_renderer(ctx, cache, region,
            combine_ids(make_id(angle), make_id(state)));
        if (cache.needs_rendering())
        {
            box_control_renderer renderer(ctx, cache, "node-expander", state);

            renderer.canvas().translate(
                renderer.content_region().size[0] / SkIntToScalar(2),
                renderer.content_region().size[1] / SkIntToScalar(2));
            renderer.canvas().rotate(float(angle));

            {
                SkPaint paint;
                paint.setFlags(SkPaint::kAntiAlias_Flag);
                set_color(paint, renderer.style().fg_color);
                paint.setStyle(SkPaint::kFill_Style);
                SkScalar a =
                    renderer.content_region().size[0] / SkIntToScalar(2);
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

node_expander_result
do_unsafe_node_expander(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec,
    simple_control_flag_set flags,
    widget_id id)
{
    node_expander_result result;
    if (do_simple_control<node_expander_renderer,
            default_node_expander_renderer>(ctx, value, layout_spec, flags, id))
    {
        result.changed = true;
        set(value, value.is_gettable() ? !value.get() : true);
    }
    else
        result.changed = false;
    return result;
}

// BUTTON

struct button_data
{
    button_input_state input;
    focus_rect_data focus_rect;
};

button_result
do_unsafe_styled_button(
    ui_context& ctx,
    accessor<string> const& style,
    accessor<string> const& label,
    layout const& layout_spec,
    button_flag_set flags,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    ALIA_GET_CACHED_DATA(button_data)
    widget_state state = (flags & BUTTON_DISABLED) ? WIDGET_DISABLED :
        get_button_state(ctx, id, data.input);
    panel p(ctx, style,
        add_default_alignment(layout_spec, LEFT, TOP),
        PANEL_UNSAFE_CLICK_DETECTION, id, state);
    do_text(ctx, label, CENTER);
    return (flags & BUTTON_DISABLED) ? false :
        do_button_input(ctx, id, data.input);
}

button_result
do_unsafe_styled_button(
    ui_context& ctx,
    accessor<string> const& style,
    accessor<string> const& label,
    accessor<string> const& tooltip,
    layout const& layout_spec,
    button_flag_set flags,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    auto result = do_unsafe_styled_button(ctx, style, label, layout_spec, flags, id);
    set_tooltip_message(ctx, id, tooltip);
    return result;
}

void
do_styled_button(
    ui_context& ctx,
    accessor<string> const& style,
    accessor<string> const& label,
    action const& on_press,
    layout const& layout_spec,
    button_flag_set flags,
    widget_id id)
{
    if (do_unsafe_styled_button(
            ctx,
            style,
            label,
            layout_spec,
            flags | (on_press.is_ready() ? NO_FLAGS : BUTTON_DISABLED),
            id))
    {
        perform_action(on_press);
        end_pass(ctx);
    }
}

void
do_styled_button(
    ui_context& ctx,
    accessor<string> const& style,
    accessor<string> const& label,
    accessor<string> const& tooltip,
    action const& on_press,
    layout const& layout_spec,
    button_flag_set flags,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    do_styled_button(ctx, style, label, on_press, layout_spec, flags, id);
    set_tooltip_message(ctx, id, tooltip);
}

button_result
do_unsafe_button(
    ui_context& ctx,
    accessor<string> const& label,
    layout const& layout_spec,
    button_flag_set flags,
    widget_id id)
{
    return
        do_unsafe_styled_button(ctx,
            text("button"),
            label,
            layout_spec,
            flags,
            id);
}

button_result
do_unsafe_button(
    ui_context& ctx,
    accessor<string> const& label,
    accessor<string> const& tooltip,
    layout const& layout_spec,
    button_flag_set flags,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    auto result = do_unsafe_button(ctx, label, layout_spec, flags, id);
    set_tooltip_message(ctx, id, tooltip);
    return result;
}

void
do_button(
    ui_context& ctx,
    accessor<string> const& text,
    action const& on_press,
    layout const& layout_spec,
    button_flag_set flags,
    widget_id id)
{
    if (do_unsafe_button(
            ctx,
            text,
            layout_spec,
            flags | (on_press.is_ready() ? NO_FLAGS : BUTTON_DISABLED),
            id))
    {
        perform_action(on_press);
        end_pass(ctx);
    }
}

void
do_button(
    ui_context& ctx,
    accessor<string> const& label,
    accessor<string> const& tooltip,
    action const& on_press,
    layout const& layout_spec,
    button_flag_set flags,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    do_button(ctx, label, on_press, layout_spec, flags, id);
    set_tooltip_message(ctx, id, tooltip);
}

button_result
do_unsafe_primary_button(
    ui_context& ctx,
    accessor<string> const& label,
    layout const& layout_spec,
    button_flag_set flags,
    widget_id id)
{
    return
        do_unsafe_styled_button(ctx,
            text("primary-button"),
            label,
            layout_spec,
            flags,
            id);
}

void
do_primary_button(
    ui_context& ctx,
    accessor<string> const& label,
    action const& on_press,
    layout const& layout_spec,
    button_flag_set flags,
    widget_id id)
{
    if (do_unsafe_primary_button(
            ctx,
            label,
            layout_spec,
            flags | (on_press.is_ready() ? NO_FLAGS : BUTTON_DISABLED),
            id))
    {
        perform_action(on_press);
        end_pass(ctx);
    }
}

}
