#include <alia/skia/artist.hpp>
#include <alia/skia/image_canvas.hpp>
#include <alia/context.hpp>
#include <alia/surface.hpp>
#include <alia/standard_colors.hpp>
#include <alia/scoped_state.hpp>
#include <alia/style_utils.hpp>
#include "SkGradientShader.h"

namespace alia { namespace skia {

// TODO: Share resources between fixed-size widgets (radio buttons, etc).

template<class Data>
bool cast_data_ptr(Data**typed_data, artist_data_ptr& data_ptr)
{
    assert(!data_ptr || dynamic_cast<Data*>(data_ptr.get()));
    *typed_data = static_cast<Data*>(data_ptr.get());
    if (!*typed_data)
    {
        *typed_data = new Data;
        data_ptr.reset(*typed_data);
        return true;
    }
    return false;
}

void artist::initialize()
{
}

// BUTTON

vector2i artist::get_button_size(artist_data_ptr& data,
    vector2i const& content_size) const
{
    int font_size = int(get_context().pass_state.style->font.get_size() + 0.5);
    return vector2i(
        (std::max)(content_size[0] + font_size + 4, font_size * 5 + 4),
        (std::max)(content_size[1] + font_size / 3 + 4, font_size + 4));
}

vector2i artist::get_button_content_offset(artist_data_ptr& data,
    vector2i const& content_size, widget_state state) const
{
    int font_size = int(get_context().pass_state.style->font.get_size() + 0.5);
    vector2i offset(font_size / 2 + 2, font_size / 6 + 2);
    if ((state & widget_states::PRIMARY_STATE_MASK) ==
        widget_states::DEPRESSED)
    {
        offset += vector2i(1, 1);
    }
    vector2i minimum_content_size(font_size * 4, font_size * 2 / 3);
    for (int i = 0; i < 2; ++i)
    {
        if (content_size[i] < minimum_content_size[i])
            offset[i] += (minimum_content_size[i] - content_size[i]) / 2;
    }
    return offset;
}

struct button_data : artist_data
{
    stateful_style_versioning_data versioning;
    rgba8 fg_color, bg_color, focus_color;
};

static button_data* get_button_data(alia::context& ctx,
    artist_data_ptr& data_ptr, widget_state state)
{
    button_data* data;
    if (cast_data_ptr(&data, data_ptr) ||
        is_outdated(ctx, data->versioning, state))
    {
        update(ctx, data->versioning, state);
        style_node const* button_style = get_substyle(ctx, "button");
        get_color_property(&data->fg_color, button_style, "color", state);
        get_color_property(&data->bg_color, button_style, "background_color",
            state);
        get_color_property(&data->focus_color, button_style, "focus_color",
            state);
    }
    return data;
}

rgba8 artist::get_button_text_color(artist_data_ptr& data_ptr,
    widget_state state) const
{
    button_data* data = get_button_data(get_context(), data_ptr, state);
    return data->fg_color;
}

void artist::draw_button(artist_data_ptr& data_ptr, box2i const& region,
    widget_state state) const
{
    button_data* data = get_button_data(get_context(), data_ptr, state);

    surface& surface = get_surface();

    point2i poly[4];
    box2i fill_region = region;
    if ((state & widget_states::PRIMARY_STATE_MASK) ==
        widget_states::DEPRESSED)
    {
        ++fill_region.corner[0];
        ++fill_region.corner[1];
    }
    make_polygon(poly, fill_region);
    surface.draw_filled_polygon(data->bg_color, poly, 4);

    if ((state & widget_states::FOCUSED) != 0)
        draw_focus_rect(add_border(fill_region, -2), data->focus_color);
}

// LINK

struct link_data : artist_data
{
    stateful_style_versioning_data versioning;
    rgba8 color;
};

static link_data* get_link_data(alia::context& ctx,
    artist_data_ptr& data_ptr, widget_state state)
{
    link_data* data;
    if (cast_data_ptr(&data, data_ptr) ||
        is_outdated(ctx, data->versioning, state))
    {
        update(ctx, data->versioning, state);
        style_node const* link_style = get_substyle(ctx, "link");
        get_color_property(&data->color, link_style, "color", state);
    }
    return data;
}

rgba8 artist::get_link_color(artist_data_ptr& data_ptr,
    widget_state state) const
{
    link_data* data = get_link_data(get_context(), data_ptr, state);
    return data->color;
}

// CONTROLS

namespace {

struct control_style_properties
{
    rgba8 fg_color, bg_color, border_color, focus_color;
    stateful_style_versioning_data versioning;
};

bool is_outdated(context& ctx, control_style_properties& props,
    widget_state state)
{
    return is_outdated(ctx, props.versioning, state);
}

void update(context& ctx, control_style_properties& props,
    widget_state state)
{
    style_node const* control_style = get_substyle(ctx, "control");
    get_color_property(&props.fg_color, control_style, "color", state);
    get_color_property(&props.bg_color, control_style, "background_color",
        state);
    get_color_property(&props.border_color, control_style, "border_color",
        state);
    get_color_property(&props.focus_color, control_style, "focus_color",
        state);
    update(ctx, props.versioning, state);
}

}

// CHECK BOX

static alia::vector2i const check_box_size(15, 15);

struct check_box_data : artist_data
{
    control_style_properties style;
};

vector2i artist::get_check_box_size(artist_data_ptr& data,
    bool checked) const
{
    return check_box_size;
}

void artist::draw_check_box(artist_data_ptr& data_ptr, bool checked,
    point2i const& position, widget_state state) const
{
    check_box_data* data;
    if (cast_data_ptr(&data, data_ptr)
        || is_outdated(get_context(), data->style, state))
    {
        update(get_context(), data->style, state);
    }

    box2i region(position, check_box_size);

    // outline
    {
    point2i poly[4];
    make_polygon(poly, region);
    get_surface().draw_filled_polygon(data->style.border_color, poly, 4);
    }

    // box
    box2i inside_region = add_border(region, -1);
    {
    point2i poly[4];
    make_polygon(poly, inside_region);
    get_surface().draw_filled_polygon(data->style.bg_color, poly, 4);
    }

    // check
    if (checked)
    {
        point2i check_position = region.corner + vector2i(4, 4);
        point2i mark0[4];
        mark0[0] = check_position + vector2i( 0, 1);
        mark0[1] = check_position + vector2i( 0, 5);
        mark0[2] = check_position + vector2i( 3, 7);
        mark0[3] = check_position + vector2i( 3, 5);
        point2i mark1[4];
        mark1[0] = check_position + vector2i( 3, 7);
        mark1[1] = check_position + vector2i( 7, 3);
        mark1[2] = check_position + vector2i( 7, 0);
        mark1[3] = check_position + vector2i( 3, 4);
        get_surface().draw_filled_polygon(data->style.fg_color, mark0, 4);
        get_surface().draw_filled_polygon(data->style.fg_color, mark1, 4);
    }

    // focus
    if ((state & widget_states::FOCUSED) != 0)
    {
        draw_focus_rect(add_border(region, vector2i(2, 2)),
            data->style.focus_color);
    }
}

// RADIO BUTTON

static vector2i const radio_button_size(19, 17);
static vector2i const radio_button_image_size(25, 25);

struct radio_button_data : artist_data
{
    control_style_properties style;
    widget_state state;
    bool selected;
    image<rgba8> img;
    cached_image_ptr cached_img;
};

void draw_radio_button(
    SkCanvas& canvas, rgba8 const& bg, rgba8 const& dot,
    rgba8 const& inner_border, rgba8 const& outer_border)
{
    SkPaint paint;
    paint.setFlags(SkPaint::kAntiAlias_Flag);

    vector2d center = vector2d(radio_button_image_size) / 2;

    paint.setARGB(bg.a, bg.r, bg.g, bg.b);
    paint.setStyle(SkPaint::kFill_Style);
    canvas.drawCircle(SkScalar(center[0]), SkScalar(center[1]), 7.5, paint);

    if (dot.a)
    {
        paint.setARGB(dot.a, dot.r, dot.g, dot.b);
        paint.setStyle(SkPaint::kFill_Style);
        canvas.drawCircle(SkScalar(center[0]), SkScalar(center[1]), 5,
            paint);
    }

    if (inner_border.a)
    {
        paint.setARGB(inner_border.a, inner_border.r, inner_border.g,
            inner_border.b);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(SkIntToScalar(1.5));
        canvas.drawCircle(SkScalar(center[0]), SkScalar(center[1]), 7.5,
            paint);
    }

    if (outer_border.a)
    {
        paint.setARGB(outer_border.a, outer_border.r, outer_border.g,
            outer_border.b);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(SkIntToScalar(1));
        canvas.drawCircle(SkScalar(center[0]), SkScalar(center[1]), 10,
            paint);
    }
}

void create_radio_button_image(
    image<rgba8>& img, rgba8 const& bg, rgba8 const& dot,
    rgba8 const& inner_border, rgba8 const& outer_border)
{
    create_image(img, radio_button_image_size);
    alia_foreach_pixel(img.view, rgba8, i,
        i.r = 0x00;
        i.g = 0x00;
        i.b = 0x00;
        i.a = 0x00)
    image_canvas canvas(img.view);
    draw_radio_button(canvas.canvas, bg, dot, inner_border, outer_border);
    alia_foreach_pixel(img.view, rgba8, i,
        if (i.a != 0)
        {
            i.r = uint8(int(i.r) * 0xff / i.a);
            i.g = uint8(int(i.g) * 0xff / i.a);
            i.b = uint8(int(i.b) * 0xff / i.a);
        })
}

vector2i artist::get_radio_button_size(artist_data_ptr& data,
    bool selected) const
{
    return radio_button_size;
}

void artist::draw_radio_button(artist_data_ptr& data_ptr, bool selected,
    point2i const& position, widget_state state) const
{
    radio_button_data* data;
    if (cast_data_ptr(&data, data_ptr)
        || is_outdated(get_context(), data->style, state)
        || data->state != state || data->selected != selected
        || !is_valid(data->cached_img))
    {
        update(get_context(), data->style, state);
        create_radio_button_image(data->img,
            data->style.bg_color,
            selected ? data->style.fg_color : rgba8(0, 0, 0, 0),
            data->style.border_color,
            (state & widget_states::FOCUSED) != 0 ?
            data->style.focus_color : rgba8(0, 0, 0, 0));
        get_context().surface->cache_image(data->cached_img,
            make_interface(data->img.view));
        data->state = state;
        data->selected = selected;
    }
    // - (4, 4) to compensate for the fact that the image is larger than the
    // supposed size of the radio button
    data->cached_img->draw(point2d(position - vector2i(3, 4)));
}

// NODE EXPANDER

struct node_expander_data : artist_data
{
    stateful_style_versioning_data versioning;
    rgba8 fg_color, bg_color, focus_color;
};

static node_expander_data* get_node_expander_data(alia::context& ctx,
    artist_data_ptr& data_ptr, widget_state state)
{
    node_expander_data* data;
    if (cast_data_ptr(&data, data_ptr) ||
        is_outdated(ctx, data->versioning, state))
    {
        update(ctx, data->versioning, state);
        get_color_property(&data->fg_color, ctx, "color", state);
        get_color_property(&data->bg_color, ctx, "background_color", state);
        get_color_property(&data->focus_color, ctx, "focus_color", state);
    }
    return data;
}

static vector2i node_expander_size(15, 15);

vector2i artist::get_node_expander_size(artist_data_ptr& data,
    int expanded) const
{
    return node_expander_size;
}

void artist::draw_node_expander(artist_data_ptr& data_ptr, int expanded,
    point2i const& position, widget_state state) const
{
    node_expander_data* data = get_node_expander_data(get_context(), data_ptr,
        state);

    box2i region(position, node_expander_size);

    point2i poly[4];
    make_polygon(poly, region);
    get_surface().draw_filled_polygon(data->bg_color, poly, 4);

    if ((state & widget_states::FOCUSED) != 0)
        draw_focus_rect(region, data->focus_color);

    switch (expanded)
    {
     case 0:
        draw_arrow(data->fg_color, region, 1, 5);
        break;
     case 1:
        draw_arrow(data->fg_color, region, 6, 7);
        break;
     case 2:
        draw_arrow(data->fg_color, region, 3, 5);
        break;
    }
}

// SEPARATOR

struct separator_data : artist_data
{
    style_versioning_data versioning;
    int width;
    rgba8 color;
};

static separator_data* get_separator_data(alia::context& ctx,
    artist_data_ptr& data_ptr)
{
    separator_data* data;
    if (cast_data_ptr(&data, data_ptr) ||
        is_outdated(ctx, data->versioning))
    {
        update(ctx, data->versioning);
        style_node const* separator_style = get_substyle(ctx, "separator");
        get_color_property(&data->color, separator_style, "color");
        get_numeric_property(&data->width, separator_style, "line_width");
    }
    return data;
}

int artist::get_separator_width(artist_data_ptr& data_ptr) const
{
    separator_data* data = get_separator_data(get_context(), data_ptr);
    return data->width;
}

void artist::draw_separator(artist_data_ptr& data_ptr,
    point2i const& position, unsigned axis, int length) const
{
    separator_data* data = get_separator_data(get_context(), data_ptr);

    point2f p0, p1;
    p0 = point2f(position) + vector2f(0.5, 0.5);
    p1 = p0;
    p1[axis] += length;

    // TODO: use a polygon
    for (int i = 0; i != data->width; ++i)
    {
        get_surface().draw_line(data->color,
            line_style(1, solid_line), p0, p1);
        p0[1 - axis] += 1;
        p1[1 - axis] += 1;
    }
}

// SCROLLBAR

struct scrollbar_style_properties
{
    stateful_style_versioning_data versioning;
    rgba8 fg_color, bg_color;
};

bool is_outdated(context& ctx, scrollbar_style_properties& props,
    widget_state state)
{
    return is_outdated(ctx, props.versioning, state);
}

void update(context& ctx, scrollbar_style_properties& props,
    widget_state state)
{
    style_node const* control_style = get_substyle(ctx, "scrollbar");
    get_color_property(&props.fg_color, control_style, "color", state);
    get_color_property(&props.bg_color, control_style, "background_color");
    update(ctx, props.versioning, state);
}

struct scrollbar_data : artist_data
{
    scrollbar_style_properties style;
};

static scrollbar_data* get_scrollbar_data(alia::context& ctx,
    artist_data_ptr& data_ptr, widget_state state)
{
    scrollbar_data* data;
    if (cast_data_ptr(&data, data_ptr) ||
        is_outdated(ctx, data->style, state))
    {
        update(ctx, data->style, state);
    }
    return data;
}

static int const scrollbar_width = 14;

int artist::get_scrollbar_width() const
{
    return scrollbar_width;
}
int artist::get_scrollbar_button_length() const
{
    return 0;
}
int artist::get_minimum_scrollbar_thumb_length() const
{
    return 10;
}

// background

void artist::draw_scrollbar_background(artist_data_ptr& data_ptr,
    box2i const& rect, int axis, int which, widget_state state) const
{
    scrollbar_data* data = get_scrollbar_data(get_context(), data_ptr, state);
    point2i poly[4];
    make_polygon(poly, rect);
    get_surface().draw_filled_polygon(data->style.bg_color, poly, 4);
}

// thumb

struct scrollbar_thumb_data : artist_data
{
    scrollbar_style_properties style;
    widget_state state;
    vector2i size;
    image<rgba8> img;
    cached_image_ptr cached_img;
};

void draw_scrollbar_thumb(
    SkCanvas& canvas, vector2i const& size, rgba8 const& fg,
    rgba8 const& bg)
{
    SkPaint paint;
    paint.setFlags(SkPaint::kAntiAlias_Flag);
    paint.setARGB(fg.a, fg.r, fg.g, fg.b);
    paint.setStrokeWidth(SkIntToScalar(scrollbar_width - 2));
    paint.setStrokeCap(SkPaint::kRound_Cap);
    float const r = scrollbar_width / 2;
    canvas.drawLine(SkScalar(r), SkScalar(r), SkScalar(size[0] - r),
        SkScalar(size[1] - r), paint);
}

void create_scrollbar_thumb_image(
    image<rgba8>& img, vector2i const& size, rgba8 const& fg_color,
    rgba8 const& bg_color)
{
    create_image(img, size);
    alia_foreach_pixel(img.view, rgba8, i,
        i.r = bg_color.r;
        i.g = bg_color.g;
        i.b = bg_color.b;
        i.a = bg_color.a)
    image_canvas canvas(img.view);
    draw_scrollbar_thumb(canvas.canvas, size, fg_color, bg_color);
    alia_foreach_pixel(img.view, rgba8, i,
        if (i.a != 0)
        {
            i.r = uint8(int(i.r) * 0xff / i.a);
            i.g = uint8(int(i.g) * 0xff / i.a);
            i.b = uint8(int(i.b) * 0xff / i.a);
        })
}

void artist::draw_scrollbar_thumb(artist_data_ptr& data_ptr,
    box2i const& rect, int axis, widget_state state) const
{
    scrollbar_thumb_data* data;
    if (cast_data_ptr(&data, data_ptr) ||
        is_outdated(get_context(), data->style, state) ||
        data->state != state || data->size != rect.size ||
        !is_valid(data->cached_img))
    {
        update(get_context(), data->style, state);
        create_scrollbar_thumb_image(data->img, rect.size,
            data->style.fg_color, data->style.bg_color);
        get_context().surface->cache_image(data->cached_img,
            make_interface(data->img.view));
        data->size = rect.size;
        data->state = state;
    }
    data->cached_img->draw(point2d(rect.corner));
}

// button

void artist::draw_scrollbar_button(artist_data_ptr& data,
    point2i const& position, int axis, int which, widget_state state) const
{
}

// junction

void artist::draw_scrollbar_junction(artist_data_ptr& data_ptr,
    point2i const& position) const
{
    scrollbar_data* data = get_scrollbar_data(get_context(), data_ptr,
        widget_states::NORMAL);
    point2i poly[4];
    make_polygon(poly, box2i(position,
        vector2i(get_scrollbar_width(), get_scrollbar_width())));
    get_surface().draw_filled_polygon(data->style.bg_color, poly, 4);
}

// PANEL

border_size artist::get_panel_border_size(artist_data_ptr& data,
    unsigned inner_style_code) const
{
    return border_size(0, 0, 0, 0);
}
void artist::draw_panel_border(artist_data_ptr& data,
    unsigned inner_style_code, box2i const& rect) const
{
    //if ((get_context().pass_state.style_code & ~0xf) == BACKGROUND_STYLE_CODE
    //    || (inner_style_code & ~0xf) == TEXT_CONTROL_STYLE_CODE)
    //{
    //    box2f adjusted_rect(
    //        point2f(rect.corner) + vector2f(0.5, 0.5),
    //        vector2f(rect.size) - vector2f(1, 1));
    //    point2f poly[4];
    //    make_polygon(poly, adjusted_rect);
    //    get_surface().draw_line_loop(
    //        get_style_colors(inner_style_code).border,
    //        line_style(1, solid_line), poly, 4);
    //}
}

struct panel_background_data : artist_data
{
    style_versioning_data versioning;
    rgba8 color, border_color;
};

static panel_background_data* get_panel_background_data(alia::context& ctx,
    artist_data_ptr& data_ptr)
{
    panel_background_data* data;
    if (cast_data_ptr(&data, data_ptr) ||
        is_outdated(ctx, data->versioning))
    {
        update(ctx, data->versioning);
        get_color_property(&data->color, ctx, "background_color");
        get_color_property(&data->border_color, ctx, "border_color");
    }
    return data;
}

void artist::draw_panel_background(artist_data_ptr& data_ptr,
    box2i const& rect) const
{
    panel_background_data* data = get_panel_background_data(get_context(),
        data_ptr);

    point2i poly[4];
    make_polygon(poly, rect);
    get_surface().draw_filled_polygon(data->color, poly, 4);

    if (data->border_color.a != 0)
        draw_focus_rect(rect, data->border_color);

    // TODO: This should technically be here, but it makes the Astroid UI look
    // noisy. Ideally, these should only be drawn when using keyboard
    // navigation (or there should be a flag for that). Alternatively, focus
    // could be indicated with shading instead.
    //if ((get_context().pass_state.style_code & FOCUSED_SUBSTYLE_FLAG) != 0)
    //    draw_focus_rect(rect);
}

// DROP DOWN BUTTON

struct drop_down_data : artist_data
{
    stateful_style_versioning_data versioning;
    rgba8 fg_color, bg_color;
};

static drop_down_data* get_drop_down_data(alia::context& ctx,
    artist_data_ptr& data_ptr, widget_state state)
{
    drop_down_data* data;
    if (cast_data_ptr(&data, data_ptr) ||
        is_outdated(ctx, data->versioning, state))
    {
        update(ctx, data->versioning, state);
        style_node const* drop_down_style = get_substyle(ctx, "drop_down");
        get_color_property(&data->fg_color, drop_down_style, "color", state);
        get_color_property(&data->bg_color, drop_down_style,
            "background_color", state);
    }
    return data;
}

vector2i artist::get_minimum_drop_down_button_size() const
{
    return vector2i(15, 15);
}
void artist::draw_drop_down_button(artist_data_ptr& data_ptr,
    box2i const& rect, widget_state state) const
{
    drop_down_data* data = get_drop_down_data(get_context(), data_ptr, state);
    point2i poly[4];
    make_polygon(poly, rect);
    get_surface().draw_filled_polygon(data->bg_color, poly, 4);
    draw_arrow(data->fg_color, rect, 3, 5);
}

// SLIDER

struct slider_data : artist_data
{
    stateful_style_versioning_data versioning;
    rgba8 thumb_color, track_color, focus_color;
};

static slider_data* get_slider_data(alia::context& ctx,
    artist_data_ptr& data_ptr, widget_state state)
{
    slider_data* data;
    if (cast_data_ptr(&data, data_ptr) ||
        is_outdated(ctx, data->versioning, state))
    {
        update(ctx, data->versioning, state);
        style_node const* slider_style = get_substyle(ctx, "slider");
        get_color_property(&data->thumb_color, slider_style, "thumb_color",
            state);
        get_color_property(&data->track_color, slider_style, "track_color",
            state);
        get_color_property(&data->focus_color, slider_style, "focus_color",
            state);
    }
    return data;
}

int artist::get_slider_left_border() const
{
    return 5;
}
int artist::get_slider_right_border() const
{
    return 5;
}
int artist::get_slider_height() const
{
    return 20;
}
int artist::get_default_slider_width() const
{
    return 130;
}
box1i artist::get_slider_track_region() const
{
    return box1i(point1i(10), vector1i(4));
}
box1i artist::get_slider_track_hot_region() const
{
    return box1i(point1i(6), vector1i(10));
}
box2i artist::get_slider_thumb_region() const
{
    return box2i(point2i(-5, 0), vector2i(10, 20));
}
void artist::draw_slider_track(artist_data_ptr& data_ptr, unsigned axis,
    int width, point2i const& position) const
{
    slider_data* data = get_slider_data(get_context(), data_ptr,
        widget_states::NORMAL);
    vector2i size;
    size[axis] = width;
    size[1 - axis] = 2;
    point2i poly[4];
    make_polygon(poly, box2i(position, size));
    get_surface().draw_filled_polygon(data->track_color, poly, 4);
}
void artist::draw_slider_thumb(artist_data_ptr& data_ptr, unsigned axis,
    point2i const& position, widget_state state) const
{
    slider_data* data = get_slider_data(get_context(), data_ptr, state);

    box2i thumb_region = get_slider_thumb_region();
    box2i region(position, thumb_region.size);
    region.corner[axis] += thumb_region.corner[0];
    region.corner[1 - axis] += thumb_region.corner[1];
    if (axis != 0)
        std::swap(region.size[0], region.size[1]);

    ++region.corner[axis];
    region.size[axis] -= 2;

    surface& surface = get_surface();
    point2i poly[4];
    make_polygon(poly, region);
    surface.draw_filled_polygon(data->thumb_color, poly, 4);

    if ((state & widget_states::FOCUSED) != 0)
        draw_focus_rect(add_border(region, vector2i(2, 2)), data->focus_color);
}

// UTILITY FUNCTIONS

void artist::draw_arrow(rgba8 const& color, box2i const& region,
    int direction, int size) const
{
    point2f arrow[3];
    if (direction < 4)
    {
        int axis = direction / 2;
        point2f position(region.corner + (region.size -
            vector2i(size * (axis + 1), size * (2 - axis))) / 2);
        position[1 - axis] += 0.5;
        float s = float(size);
        switch (direction)
        {
         case 0:
            arrow[0] = position + vector2f(0, s);
            arrow[1] = position + vector2f(s, s * 2);
            arrow[2] = position + vector2f(s, 0);
            break;
         case 1:
            arrow[0] = position + vector2f(0, s * 2);
            arrow[1] = position + vector2f(s, s);
            arrow[2] = position + vector2f(0, 0);
            break;
         case 2:
            arrow[0] = position + vector2f(0, s);
            arrow[1] = position + vector2f(s * 2, s);
            arrow[2] = position + vector2f(s, 0);
            break;
         case 3:
            arrow[0] = position + vector2f(0, 0);
            arrow[1] = position + vector2f(s, s);
            arrow[2] = position + vector2f(s * 2, 0);
            break;
        }
    }
    else
    {
        point2f position(region.corner + (region.size -
            vector2i(size, size)) / 2);
        float s = float(size);
        switch (direction)
        {
         case 4:
            arrow[0] = position + vector2f(0, s);
            arrow[1] = position + vector2f(0, 0);
            arrow[2] = position + vector2f(s, 0);
            break;
         case 5:
            arrow[0] = position + vector2f(0, 0);
            arrow[1] = position + vector2f(s, 0);
            arrow[2] = position + vector2f(s, s);
            break;
         case 6:
            position -= vector2f(2, 1);
            arrow[0] = position + vector2f(s, 0);
            arrow[1] = position + vector2f(s, s);
            arrow[2] = position + vector2f(0, s);
            break;
         case 7:
            arrow[0] = position + vector2f(s, s);
            arrow[1] = position + vector2f(0, s);
            arrow[2] = position + vector2f(0, 0);
            break;
        }
    }
    get_surface().draw_filled_polygon(color, arrow, 3);
}

void artist::draw_focus_rect(box2i const& rect,
    rgba8 const& color) const
{
    box2f r(rect);
    r.corner[0] += 0.375;
    r.corner[1] += 0.375;
    r.size[0] -= 1;
    r.size[1] -= 1;
    point2f poly[4];
    make_polygon(poly, r);
    get_surface().draw_line_loop(color, line_style(1, solid_line), poly, 4);
}

void artist::draw_outline(box2i const& region,
    rgba8 const& color) const
{
    box2f adjusted_rect(
        point2f(region.corner) + vector2f(0.5, 0.5),
        vector2f(region.size) - vector2f(1, 1));
    point2f poly[4];
    make_polygon(poly, adjusted_rect);
    get_surface().draw_line_loop(color, line_style(1, solid_line), poly, 4);
}

// FOCUS RECT

struct focus_rect_data : artist_data
{
    style_versioning_data versioning;
    rgba8 color;
};

static focus_rect_data* get_focus_rect_data(alia::context& ctx,
    artist_data_ptr& data_ptr)
{
    focus_rect_data* data;
    if (cast_data_ptr(&data, data_ptr) ||
        is_outdated(ctx, data->versioning))
    {
        update(ctx, data->versioning);
        get_color_property(&data->color, ctx, "focus_color");
    }
    return data;
}

void artist::draw_focus_rect(artist_data_ptr& data_ptr,
    box2i const& rect) const
{
    focus_rect_data* data = get_focus_rect_data(get_context(), data_ptr);
    draw_focus_rect(rect, data->color);
}

// PROGRESS BAR

struct progress_bar_data : artist_data
{
    style_versioning_data versioning;
    rgba8 border_color, bar_color;
};

static progress_bar_data* get_progress_bar_data(alia::context& ctx,
    artist_data_ptr& data_ptr)
{
    progress_bar_data* data;
    if (cast_data_ptr(&data, data_ptr) ||
        is_outdated(ctx, data->versioning))
    {
        update(ctx, data->versioning);
        style_node const* bar_style = get_substyle(ctx, "progress_bar");
        get_color_property(&data->border_color, bar_style, "border_color");
        get_color_property(&data->bar_color, bar_style, "color");
    }
    return data;
}

vector2i artist::get_default_progress_bar_size() const
{
    return vector2i(100, 20);
}
vector2i artist::get_minimum_progress_bar_size() const
{
    return vector2i(40, 20);
}
void artist::draw_progress_bar(artist_data_ptr& data_ptr,
    box2i const& region, double value) const
{
    progress_bar_data* data = get_progress_bar_data(get_context(), data_ptr);
    draw_outline(region, data->border_color);
    box2i bar_rect(
        region.corner + vector2i(2, 2),
        region.size - vector2i(4, 4));
    bar_rect.size[0] = int(bar_rect.size[0] * value + 0.5);
    point2i poly[4];
    make_polygon(poly, bar_rect);
    get_surface().draw_filled_polygon(data->bar_color, poly, 4);
}

// ICON BUTTONS

static vector2i const icon_button_size(17, 17);
static vector2i const icon_button_image_size(25, 25);

struct icon_button_data : artist_data
{
    widget_state state;
    standard_icon icon;

    image<rgba8> img;
    cached_image_ptr cached_img;

    stateful_style_versioning_data style_versioning;
};

void draw_icon_button(
    SkCanvas& canvas, standard_icon icon, rgba8 const& bg, rgba8 const& fg,
    rgba8 const& border)
{
    SkPaint paint;
    paint.setFlags(SkPaint::kAntiAlias_Flag);

    vector2d center = vector2d(icon_button_image_size) / 2;

    paint.setARGB(bg.a, bg.r, bg.g, bg.b);
    paint.setStyle(SkPaint::kFill_Style);
    canvas.drawCircle(SkScalar(center[0]), SkScalar(center[1]), 9, paint);

    switch (icon)
    {
     case REMOVE_ICON:
      {
        float a = 8.5;
        paint.setARGB(fg.a, fg.r, fg.g, fg.b);
        paint.setStrokeWidth(SkIntToScalar(3));
        paint.setStrokeCap(SkPaint::kRound_Cap);
        paint.setStyle(SkPaint::kFill_Style);
        canvas.drawLine(SkScalar(a), SkScalar(a),
            SkScalar(icon_button_image_size[0] - a),
            SkScalar(icon_button_image_size[1] - a), paint);
        canvas.drawLine(SkScalar(icon_button_image_size[0] - a), SkScalar(a),
            SkScalar(a), SkScalar(icon_button_image_size[1] - a), paint);
        break;
      }
    }

    if (border.a)
    {
        paint.setARGB(border.a, border.r, border.g, border.b);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(SkIntToScalar(1.5));
            canvas.drawCircle(SkScalar(center[0]), SkScalar(center[1]), 9.5,
            paint);
    }
}

void create_icon_button_image(
    image<rgba8>& img, standard_icon icon, rgba8 const& bg, rgba8 const& fg,
    rgba8 const& border)
{
    create_image(img, icon_button_image_size);
    alia_foreach_pixel(img.view, rgba8, i,
        i.r = 0x00;
        i.g = 0x00;
        i.b = 0x00;
        i.a = 0x00)
    image_canvas canvas(img.view);
    draw_icon_button(canvas.canvas, icon, bg, fg, border);
    alia_foreach_pixel(img.view, rgba8, i,
        if (i.a != 0)
        {
            i.r = uint8(int(i.r) * 0xff / i.a);
            i.g = uint8(int(i.g) * 0xff / i.a);
            i.b = uint8(int(i.b) * 0xff / i.a);
        })
}

vector2i artist::get_icon_button_size(artist_data_ptr& data,
    standard_icon icon)
{
    return icon_button_size;
}

void artist::draw_icon_button(artist_data_ptr& data_ptr, standard_icon icon,
    point2i const& position, widget_state state)
{
    context& ctx = get_context();
    icon_button_data* data;
    if (cast_data_ptr(&data, data_ptr) ||
        is_outdated(ctx, data->style_versioning, state) ||
        data->state != state || data->icon != icon ||
        !is_valid(data->cached_img))
    {
        update(ctx, data->style_versioning, state);
        rgba8 fg_color, bg_color, focus_color;
        style_node const* icon_button_style = get_substyle(ctx, "icon_button");
        get_color_property(&fg_color, icon_button_style, "color", state);
        get_color_property(&bg_color, icon_button_style, "background_color",
            state);
        get_color_property(&focus_color, icon_button_style, "focus_color",
            state);
        create_icon_button_image(data->img, icon,
            bg_color, fg_color,
            (state & widget_states::FOCUSED) != 0 ?
            focus_color : rgba8(0, 0, 0, 0));
        get_context().surface->cache_image(data->cached_img,
            make_interface(data->img.view));
        data->state = state;
        data->icon = icon;
    }
    // - (4, 4) to compensate for the fact that the image is larger than the
    // supposed size of the icon button
    data->cached_img->draw(point2d(position - vector2i(4, 4)));
}

}}
