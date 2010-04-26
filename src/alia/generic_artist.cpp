#include <alia/generic_artist.hpp>
#include <alia/context.hpp>
#include <alia/surface.hpp>
#include <alia/standard_colors.hpp>
#include <alia/scoped_state.hpp>

namespace alia {

static unsigned const overlay_alpha = 0xc0;

void generic_artist::initialize()
{
    set_color_scheme(0);
}

void generic_artist::set_color_scheme(unsigned color_scheme_index)
{
    color_scheme cs;
    switch (color_scheme_index)
    {
     case 1:
        cs.dialog_normal_fg = rgb8(0x00, 0x00, 0x00);
        cs.dialog_normal_bg = rgb8(0xe8, 0xe8, 0xe8);
        cs.hot_fg = cs.dialog_normal_fg;
        cs.hot_bg = rgb8(0xc0, 0xc0, 0xc0);
        cs.selected_fg = cs.dialog_normal_bg;
        cs.selected_bg = cs.dialog_normal_fg;
        cs.focused_fg = cs.selected_fg;
        cs.focused_bg = cs.selected_bg;
        cs.disabled_fg = rgb8(0x40, 0x40, 0x40);
        cs.disabled_bg = cs.dialog_normal_bg;
        cs.link = rgb8(0x00, 0x00, 0xa0);
        cs.depressed_link = rgb8(0x00, 0x00, 0x80);
        cs.border = rgb8(0x40, 0x40, 0x40);
        cs.separator = rgb8(0x40, 0x40, 0x40);
        cs.content_normal_fg = rgb8(0x10, 0x10, 0x10);
        cs.content_normal_bg = rgb8(0xff, 0xff, 0xff);
        cs.title_fg = rgb8(0x40, 0x40, 0x60);
        cs.heading_fg = rgb8(0x40, 0x40, 0x60);
        cs.subheading_fg = rgb8(0x40, 0x40, 0x60);
        cs.highlighted_fg = rgb8(0x00, 0x00, 0x00);
        cs.text_control_fg = rgb8(0x00, 0x00, 0x00);
        cs.text_control_bg = rgb8(0xe8, 0xe8, 0xe8);
        break;
     case 0:
     default:
        cs.dialog_normal_fg = rgb8(0x9c, 0x9c, 0x9c);
        cs.dialog_normal_bg = rgb8(0x10, 0x10, 0x10);
        cs.hot_fg = cs.dialog_normal_fg;
        cs.hot_bg = rgb8(0x3e, 0x3e, 0x40);
        cs.selected_fg = cs.dialog_normal_bg;
        cs.selected_bg = cs.dialog_normal_fg;
        cs.focused_fg = cs.selected_fg;
        cs.focused_bg = cs.selected_bg;
        cs.disabled_fg = rgb8(0x6a, 0x6a, 0x6a);
        cs.disabled_bg = cs.dialog_normal_bg;
        cs.link = rgb8(0x66, 0x99, 0xcc);
        cs.depressed_link = rgb8(0x40, 0x60, 0xa0);
        cs.border = rgb8(0x66, 0x66, 0x66);
        cs.separator = rgb8(0x57, 0x57, 0x57);
        cs.content_normal_fg = rgb8(0xa0, 0xa0, 0xa0);
        cs.content_normal_bg = rgb8(0x21, 0x21, 0x21);
        cs.title_fg = rgb8(0xd4, 0xd4, 0xe0);
        cs.heading_fg = rgb8(0xd4, 0xd4, 0xe0);
        cs.subheading_fg = rgb8(0xc0, 0xc0, 0xc8);
        cs.highlighted_fg = rgb8(0xbb, 0xbb, 0xbb);
        cs.text_control_fg = rgb8(0xb0, 0xb0, 0xb0);
        cs.text_control_bg = rgb8(0x17, 0x17, 0x17);
        break;
    }
    set_color_scheme(cs);
}

void generic_artist::set_color_scheme(color_scheme const& cs)
{
    {
    style_colors& sc = style_color_info[DIALOG_STYLE_CODE];
    sc.normal_fg = cs.dialog_normal_fg;
    sc.normal_bg = cs.dialog_normal_bg;
    sc.hot_fg = cs.hot_fg;
    sc.hot_bg = cs.hot_bg;
    sc.selected_fg = cs.selected_fg;
    sc.selected_bg = cs.selected_bg;
    sc.focused_fg = cs.focused_fg;
    sc.focused_bg = cs.focused_bg;
    sc.disabled_fg = cs.disabled_fg;
    sc.disabled_bg = cs.disabled_bg;
    sc.link = cs.link;
    sc.hot_link = cs.link;
    sc.depressed_link = cs.depressed_link;
    sc.disabled_link = cs.disabled_fg;
    sc.border = cs.border;
    sc.separator = cs.separator;
    }
    {
    style_colors& sc = style_color_info[CONTENT_STYLE_CODE];
    sc = style_color_info[DIALOG_STYLE_CODE];
    sc.normal_fg = cs.content_normal_fg;
    sc.normal_bg = cs.content_normal_bg;
    sc.hot_fg = cs.content_normal_fg;
    sc.hot_bg = blend(cs.content_normal_bg, cs.content_normal_fg, 0.85f);
    sc.selected_fg = cs.content_normal_fg;
    sc.selected_bg = blend(cs.content_normal_bg, cs.content_normal_fg, 0.75f);
    sc.focused_fg = cs.content_normal_fg;
    sc.focused_bg = sc.selected_bg;
    //sc.hot_fg = cs.highlighted_fg;
    //sc.hot_bg = cs.content_normal_bg;
    //sc.selected_fg = cs.subheading_fg;
    //sc.selected_bg = cs.content_normal_bg;
    //sc.focused_fg = cs.subheading_fg;
    //sc.focused_bg = cs.content_normal_bg;
    }
    {
    style_colors& sc = style_color_info[ODD_CONTENT_STYLE_CODE];
    sc = style_color_info[CONTENT_STYLE_CODE];
    sc.normal_bg = blend(cs.content_normal_bg, cs.content_normal_fg, 0.96f);
    sc.hot_bg = blend(cs.content_normal_bg, cs.content_normal_fg, 0.85f);
    sc.selected_bg = blend(cs.content_normal_bg, cs.content_normal_fg, 0.75f);
    sc.focused_bg = sc.selected_bg;
    }
    {
    style_colors& sc = style_color_info[BACKGROUND_STYLE_CODE];
    sc = style_color_info[DIALOG_STYLE_CODE];
    }
    {
    style_colors& sc = style_color_info[TITLE_STYLE_CODE];
    sc = style_color_info[CONTENT_STYLE_CODE];
    sc.normal_fg = cs.title_fg;
    }
    {
    style_colors& sc = style_color_info[HEADING_STYLE_CODE];
    sc = style_color_info[CONTENT_STYLE_CODE];
    sc.normal_fg = cs.heading_fg;
    }
    {
    style_colors& sc = style_color_info[SUBHEADING_STYLE_CODE];
    sc = style_color_info[CONTENT_STYLE_CODE];
    sc.normal_fg = cs.subheading_fg;
    }
    {
    style_colors& sc = style_color_info[TEXT_CONTROL_STYLE_CODE];
    sc = style_color_info[DIALOG_STYLE_CODE];
    sc.normal_fg = cs.text_control_fg;
    sc.normal_bg = cs.text_control_bg;
    }
    {
    style_colors& sc = style_color_info[LIST_STYLE_CODE];
    sc = style_color_info[TEXT_CONTROL_STYLE_CODE];
    }
    {
    style_colors& sc = style_color_info[HIGHLIGHTED_STYLE_CODE];
    sc = style_color_info[CONTENT_STYLE_CODE];
    sc.normal_fg = cs.highlighted_fg;
    }
}

unsigned generic_artist::get_code_for_style(style s, widget_state state,
    bool selected)
{
    unsigned major_style;
    unsigned flags = (get_context().pass_state.style_code & OVERLAY_FLAG);
    switch (s)
    {
     case BACKGROUND_STYLE:
        major_style = BACKGROUND_STYLE_CODE;
        break;
     case DIALOG_STYLE:
        major_style = DIALOG_STYLE_CODE;
        break;
     case TITLE_STYLE:
        major_style = TITLE_STYLE_CODE;
        break;
     case HEADING_STYLE:
        major_style = HEADING_STYLE_CODE;
        break;
     case SUBHEADING_STYLE:
        major_style = SUBHEADING_STYLE_CODE;
        break;
     case TEXT_CONTROL_STYLE:
        major_style = TEXT_CONTROL_STYLE_CODE;
        break;
     case LIST_STYLE:
        major_style = LIST_STYLE_CODE;
        break;
     case ITEM_STYLE:
        major_style = (get_context().pass_state.style_code >> 4) & 0xf;
        break;
     case OVERLAY_STYLE:
        major_style = CONTENT_STYLE_CODE;
        flags |= OVERLAY_FLAG;
        break;
     case HIGHLIGHTED_STYLE:
        major_style = HIGHLIGHTED_STYLE_CODE;
        break;
     case ODD_CONTENT_STYLE:
        major_style = ODD_CONTENT_STYLE_CODE;
        break;
     case CONTENT_STYLE:
     default:
        major_style = CONTENT_STYLE_CODE;
    }
    assert(major_style < N_MAJOR_STYLES);
    unsigned substyle;
    if (selected)
    {
        substyle = SELECTED_SUBSTYLE_OFFSET;
    }
    else if ((state & widget_states::DISABLED) != 0)
    {
        substyle = DISABLED_SUBSTYLE_OFFSET;
    }
    else
    {
        switch (state & widget_states::PRIMARY_STATE_MASK)
        {
         case widget_states::HOT:
            substyle = HOT_SUBSTYLE_OFFSET;
            break;
         case widget_states::DEPRESSED:
            substyle = SELECTED_SUBSTYLE_OFFSET;
            break;
         default:
            substyle = NORMAL_SUBSTYLE_OFFSET;
        }
        if ((state & widget_states::FOCUSED) != 0)
        {
            substyle |= FOCUSED_SUBSTYLE_FLAG;
        }
    }
    return flags | (major_style << 4) | substyle;
}
void generic_artist::activate_style(unsigned style_code)
{
    unsigned major_style = (style_code >> 4) & 0xf;
    unsigned substyle = style_code & 0xf;
    context& ctx = get_context();
    ctx.pass_state.active_font = translate_standard_font(NORMAL_FONT);
    ctx.pass_state.padding_size =
        major_style == BACKGROUND_STYLE_CODE ?
        vector2i(0, 0) :
        vector2i((std::max)((ctx.surface->get_ascii_text_size(
            ctx.pass_state.active_font, " ")[0] + 1) / 2, 2), 2);
    style_colors const& sc = get_style_colors(style_code);
    active_style_colors = &sc;
    uint8 bg_alpha = (style_code & OVERLAY_FLAG) != 0 ? overlay_alpha : 0xff;
    switch (substyle & 0x3)
    {
     case DISABLED_SUBSTYLE_OFFSET:
        //ctx.pass_state.text_color = sc.disabled_fg;
        //ctx.pass_state.bg_color = rgba8(sc.disabled_bg, bg_alpha);
        //ctx.pass_state.selected_text_color = sc.selected_fg;
        //ctx.pass_state.selected_bg_color = rgba8(sc.selected_bg, bg_alpha);
        //break;
     case NORMAL_SUBSTYLE_OFFSET:
        ctx.pass_state.text_color = sc.normal_fg;
        ctx.pass_state.bg_color = rgba8(sc.normal_bg, bg_alpha);
        ctx.pass_state.selected_text_color = sc.selected_fg;
        ctx.pass_state.selected_bg_color = rgba8(sc.selected_bg, bg_alpha);
        break;
     case HOT_SUBSTYLE_OFFSET:
        ctx.pass_state.text_color = sc.hot_fg;
        ctx.pass_state.bg_color = rgba8(sc.hot_bg, bg_alpha);
        ctx.pass_state.selected_text_color = sc.selected_fg;
        ctx.pass_state.selected_bg_color = rgba8(sc.selected_bg, bg_alpha);
        break;
     case SELECTED_SUBSTYLE_OFFSET:
        ctx.pass_state.text_color = sc.selected_fg;
        ctx.pass_state.bg_color = rgba8(sc.selected_bg, bg_alpha);
        ctx.pass_state.selected_text_color = sc.normal_fg;
        ctx.pass_state.selected_bg_color = rgba8(sc.normal_bg, bg_alpha);
        break;
    }
}
void generic_artist::restore_style(unsigned style_code)
{
    active_style_colors = &get_style_colors(style_code);
}
generic_artist::style_colors const&
generic_artist::get_style_colors(unsigned style_code) const
{
    unsigned major_style = (style_code >> 4) & 0xf;
    assert(major_style < N_MAJOR_STYLES);
    return style_color_info[major_style];
}
rgba8 generic_artist::get_fg_color(widget_state state) const
{
    if ((state & widget_states::DISABLED) != 0)
    {
        return active_style_colors->disabled_fg;
    }
    else
    {
        switch (state & widget_states::PRIMARY_STATE_MASK)
        {
         case widget_states::HOT:
            return active_style_colors->hot_fg;
         case widget_states::DEPRESSED:
            return active_style_colors->selected_fg;
         default:
            return active_style_colors->normal_fg;
        }
    }
}
rgba8 generic_artist::get_bg_color(widget_state state) const
{
    uint8 bg_alpha = (get_context().pass_state.style_code & OVERLAY_FLAG) != 0
        ? overlay_alpha : 0xff;
    if ((state & widget_states::DISABLED) != 0)
    {
        return rgba8(active_style_colors->disabled_bg, bg_alpha);
    }
    else
    {
        switch (state & widget_states::PRIMARY_STATE_MASK)
        {
         case widget_states::HOT:
            return rgba8(active_style_colors->hot_bg, bg_alpha);
         case widget_states::DEPRESSED:
            return rgba8(active_style_colors->selected_bg, bg_alpha);
         default:
            return rgba8(active_style_colors->normal_bg, bg_alpha);
        }
    }
}

font generic_artist::translate_standard_font(standard_font font) const
{
    switch ((get_context().pass_state.style_code >> 4) & 0xf)
    {
     case TITLE_STYLE_CODE:
        switch (font)
        {
         case FIXED_FONT:
            return alia::font("courier", 10 * get_context().font_scale_factor);
         case NORMAL_FONT:
         default:
            return alia::font("georgia",
                17 * get_context().font_scale_factor, font::BOLD);
        }
        break;
     case HEADING_STYLE_CODE:
        switch (font)
        {
         case FIXED_FONT:
            return alia::font("courier", 10 * get_context().font_scale_factor);
         case NORMAL_FONT:
         default:
            return alia::font("georgia",
                15 * get_context().font_scale_factor, font::BOLD);
        }
        break;
     case SUBHEADING_STYLE_CODE:
        switch (font)
        {
         case FIXED_FONT:
            return alia::font("courier", 10 * get_context().font_scale_factor);
         case NORMAL_FONT:
         default:
            return alia::font("georgia",
                12 * get_context().font_scale_factor, font::BOLD);
        }
        break;
     case HIGHLIGHTED_STYLE_CODE:
     case TEXT_CONTROL_STYLE_CODE:
     case LIST_STYLE_CODE:
        switch (font)
        {
         case FIXED_FONT:
            return alia::font("courier", 10 * get_context().font_scale_factor);
         case NORMAL_FONT:
         default:
            return alia::font("helvetica",
                12 * get_context().font_scale_factor, 0, 1.1f);
        }
        break;
     default:
        switch (font)
        {
         case FIXED_FONT:
            return alia::font("courier", 10 * get_context().font_scale_factor);
         case NORMAL_FONT:
         default:
            return alia::font("helvetica",
                12 * get_context().font_scale_factor, 0, 1.1f);
        }
        break;
    }
}

// BUTTON

vector2i generic_artist::get_button_size(artist_data_ptr& data,
    vector2i const& content_size) const
{
    return vector2i(
        (std::max)(content_size[0] + 12, 75),
        (std::max)(content_size[1] + 8, 23));
}

rgba8 generic_artist::get_button_text_color(widget_state state) const
{
    // TODO
    return black;
}

vector2i generic_artist::get_button_content_offset(artist_data_ptr& data,
    vector2i const& content_size, widget_state state) const
{
    vector2i offset(6, 4);
    if ((state & widget_states::PRIMARY_STATE_MASK) ==
        widget_states::DEPRESSED)
    {
        offset += vector2i(1, 1);
    }
    vector2i minimum_content_size(63, 15);
    for (int i = 0; i < 2; ++i)
    {
        if (content_size[i] < minimum_content_size[i])
            offset[i] += (minimum_content_size[i] - content_size[i]) / 2;
    }
    return offset;
}

void generic_artist::draw_button(artist_data_ptr& data_, box2i const& region,
    widget_state state) const
{
    draw_box(region, state, 0);
    if ((state & widget_states::FOCUSED) != 0)
        draw_focus_rect(add_border(region, -2));
}

// LINK

rgba8 generic_artist::get_link_color(artist_data_ptr& data,
    widget_state state) const
{
    if ((state & widget_states::DISABLED) != 0)
    {
        return active_style_colors->disabled_link;
    }
    else
    {
        switch (state & widget_states::PRIMARY_STATE_MASK)
        {
         case widget_states::HOT:
            return active_style_colors->hot_link;
         case widget_states::DEPRESSED:
            return active_style_colors->depressed_link;
         default:
            return active_style_colors->link;
        }
    }
}

// CHECK BOX

static alia::vector2i const check_box_size(15, 15);

vector2i generic_artist::get_check_box_size(artist_data_ptr& data,
    bool checked) const
{
    return check_box_size;
}

void generic_artist::draw_check_box(artist_data_ptr& data, bool checked,
    point2i const& position, widget_state state) const
{
    box2i region(position, check_box_size);
    rgba8 fg = get_fg_color(state), bg = get_bg_color(state);

    // outline
    {
    point2i poly[4];
    make_polygon(poly, region);
    get_surface().draw_filled_polygon(fg, poly, 4);
    }

    // box
    box2i inside_region = add_border(region, -1);
    {
    point2i poly[4];
    make_polygon(poly, inside_region);
    get_surface().draw_filled_polygon(bg, poly, 4);
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
        get_surface().draw_filled_polygon(fg, mark0, 4);
        get_surface().draw_filled_polygon(fg, mark1, 4);
    }

    // text
    if ((state & widget_states::FOCUSED) != 0)
        draw_focus_rect(add_border(region, vector2i(2, 2)));
}

// RADIO BUTTON

static vector2i const radio_button_size(13, 13);

vector2i generic_artist::get_radio_button_size(artist_data_ptr& data,
    bool selected) const
{
    return radio_button_size;
}

void generic_artist::draw_radio_button(artist_data_ptr& data, bool selected,
    point2i const& position, widget_state state) const
{
    box2i region(position, radio_button_size);
    draw_octagon(get_fg_color(state), region, 4);
    draw_octagon(get_bg_color(state), add_border(region, -1), 3);
    if (selected)
        draw_octagon(get_fg_color(state), add_border(region, -4), 1);
    if ((state & widget_states::FOCUSED) != 0)
        draw_focus_rect(add_border(region, vector2i(2, 2)));
}

// NODE EXPANDER

static vector2i node_expander_size(15, 15);

vector2i generic_artist::get_node_expander_size(artist_data_ptr& data,
    int expanded) const
{
    return node_expander_size;
}

void generic_artist::draw_node_expander(artist_data_ptr& data, int expanded,
    point2i const& position, widget_state state) const
{
    box2i region(position, node_expander_size);

    point2i poly[4];
    make_polygon(poly, region);
    get_surface().draw_filled_polygon(get_bg_color(state), poly, 4);

    if ((state & widget_states::FOCUSED) != 0)
        draw_focus_rect(region);

    rgba8 fg_color = get_fg_color(state);
    switch (expanded)
    {
     case 0:
        draw_arrow(fg_color, region, 1, 5);
        break;
     case 1:
        draw_arrow(fg_color, region, 6, 7);
        break;
     case 2:
        draw_arrow(fg_color, region, 3, 5);
        break;
    }
}

// SEPARATOR

int generic_artist::get_separator_width() const
{
    return
        ((get_context().pass_state.style_code & ~0xf) == BACKGROUND_STYLE_CODE)
      ? 2 : 1;
}

void generic_artist::draw_separator(artist_data_ptr& data,
    point2i const& position, unsigned axis, int length) const
{
    point2f p0, p1;
    p0 = point2f(position) + vector2f(0.5, 0.5);
    p1 = p0;
    p1[axis] += length;

    get_surface().draw_line(active_style_colors->separator,
        line_style(1, solid_line), p0, p1);

    if ((get_context().pass_state.style_code & ~0xf) == BACKGROUND_STYLE_CODE)
    {
        p0[1 - axis] += 1;
        p1[1 - axis] += 1;
        get_surface().draw_line(active_style_colors->separator,
            line_style(1, solid_line), p0, p1);
    }
}

// SCROLLBAR

int generic_artist::get_scrollbar_width() const
{
    return 16;
}
int generic_artist::get_scrollbar_button_length() const
{
    return 0;
}
int generic_artist::get_minimum_scrollbar_thumb_length() const
{
    return 10;
}

// background

void generic_artist::draw_scrollbar_background(artist_data_ptr& data,
    box2i const& rect, int axis, int which, widget_state state) const
{
    box2i border_region = rect;
    border_region.size[1 - axis] = 1;
    {
    point2i poly[4];
    make_polygon(poly, border_region);
    get_surface().draw_filled_polygon(get_fg_color(state), poly, 4);
    }

    box2i fill_region = rect;
    ++fill_region.corner[1 - axis];
    --fill_region.size[1 - axis];
    {
    point2i poly[4];
    make_polygon(poly, fill_region);
    get_surface().draw_filled_polygon(get_bg_color(state), poly, 4);
    }
}

// thumb

void generic_artist::draw_scrollbar_thumb(artist_data_ptr& data,
    box2i const& rect, int axis, widget_state state) const
{
    draw_box(rect, state, axis);
}

// button

void generic_artist::draw_scrollbar_button(artist_data_ptr& data,
    point2i const& position, int axis, int which, widget_state state) const
{
    //vector2i size;
    //size[axis] = get_scrollbar_button_length();
    //size[1 - axis] = get_scrollbar_width();
    //box2i rect(position, size);
    //draw_box(rect, state, axis);
    //draw_arrow(get_fg_color(state), rect, axis * 2 + which, 5);
}

// junction

void generic_artist::draw_scrollbar_junction(artist_data_ptr& data,
    point2i const& position) const
{
    point2i poly[4];
    make_polygon(poly, box2i(position,
        vector2i(get_scrollbar_width(), get_scrollbar_width())));
    get_surface().draw_filled_polygon(get_bg_color(widget_states::NORMAL),
        poly, 4);
}

// PANEL

border_size generic_artist::get_panel_border_size(artist_data_ptr& data,
    unsigned inner_style_code) const
{
    //if ((get_context().pass_state.style_code & ~0xf) == BACKGROUND_STYLE_CODE
    //    || (inner_style_code & ~0xf) >= LIST_ITEM_STYLE_CODE)
    {
        return border_size(0, 0, 0, 0);
    }
    //else
    //    return border_size(1, 1, 1, 1);
}
void generic_artist::draw_panel_border(artist_data_ptr& data,
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
void generic_artist::draw_panel_background(artist_data_ptr& data,
    box2i const& rect) const
{
    point2i poly[4];
    make_polygon(poly, rect);
    get_surface().draw_filled_polygon(get_context().pass_state.bg_color, poly,
        4);
    // TODO: This should technically be here, but it makes the Astroid UI look
    // noisy. Ideally, these should only be drawn when using keyboard
    // navigation (or there should be a flag for that). Alternatively, focus
    // could be indicated with shading instead.
    //if ((get_context().pass_state.style_code & FOCUSED_SUBSTYLE_FLAG) != 0)
    //    draw_focus_rect(rect);
}

// DROP DOWN BUTTON

vector2i generic_artist::get_minimum_drop_down_button_size() const
{
    return vector2i(15, 15);
}
void generic_artist::draw_drop_down_button(artist_data_ptr& data,
    box2i const& rect, widget_state state) const
{
    point2i poly[4];
    make_polygon(poly, rect);
    get_surface().draw_filled_polygon(get_bg_color(state), poly, 4);
    draw_arrow(get_fg_color(state), rect, 3, 5);
}

// SLIDER

// track

int generic_artist::get_slider_left_border() const
{
    return 5;
}
int generic_artist::get_slider_right_border() const
{
    return 5;
}
int generic_artist::get_slider_height() const
{
    return 20;
}
int generic_artist::get_default_slider_width() const
{
    return 130;
}
box1i generic_artist::get_slider_track_region() const
{
    return box1i(point1i(10), vector1i(4));
}
box1i generic_artist::get_slider_track_hot_region() const
{
    return box1i(point1i(6), vector1i(10));
}
box2i generic_artist::get_slider_thumb_region() const
{
    return box2i(point2i(-6, 0), vector2i(12, 20));
}
void generic_artist::draw_slider_track(artist_data_ptr& data, unsigned axis,
    int width, point2i const& position) const
{
    vector2i size;
    size[axis] = width;
    size[1 - axis] = 2;
    point2i poly[4];
    make_polygon(poly, box2i(position, size));
    get_surface().draw_filled_polygon(gray, poly, 4); // TODO: color
}
void generic_artist::draw_slider_thumb(artist_data_ptr& data, unsigned axis,
    point2i const& position, widget_state state) const
{
    box2i thumb_region = get_slider_thumb_region();
    box2i region(position, thumb_region.size);
    region.corner[axis] += thumb_region.corner[0];
    region.corner[1 - axis] += thumb_region.corner[1];
    if (axis != 0)
        std::swap(region.size[0], region.size[1]);
    draw_box(region, state, axis);
}

// UTILITY FUNCTIONS

void generic_artist::draw_box(box2i const& region, widget_state state,
    int gradient_axis, bool draw_border) const
{
    surface& surface = get_surface();
    rgba8 bg_color = get_bg_color(state);
    box2i fill_region;
    point2i poly[4];
    if (draw_border)
    {
        // TODO
        make_polygon(poly, box2i(region.corner,
            vector2i(1, region.size[1] - 1)));
        surface.draw_filled_polygon(blend(bg_color, white, 0.7f), poly, 4);
        make_polygon(poly, box2i(region.corner + vector2i(1, 0),
            vector2i(region.size[0] - 1, 1)));
        surface.draw_filled_polygon(blend(bg_color, white, 0.7f), poly, 4);

        make_polygon(poly, box2i(
            region.corner + vector2i(0, region.size[1] - 1),
            vector2i(region.size[0] - 1, 1)));
        surface.draw_filled_polygon(scale(bg_color, 0.7f), poly, 4);
        make_polygon(poly, box2i(
            region.corner + vector2i(region.size[0] - 1, 1),
            vector2i(1, region.size[1] - 1)));
        surface.draw_filled_polygon(scale(bg_color, 0.7f), poly, 4);

        fill_region = add_border(region, -1);
    }
    else
        fill_region = region;
    make_polygon(poly, fill_region);
    surface.draw_filled_polygon(bg_color, poly, 4);
}

void generic_artist::draw_octagon(rgba8 const& color,
    box2i const& region, int corner_size) const
{
    point2f position(region.corner);
    vector2f size(region.size);
    point2f octagon[8];
    octagon[0] = position + vector2f(0, (corner_size + 0.5f));
    octagon[1] = position + vector2f(0, size[1] - (corner_size + 0.5f));
    octagon[2] = position + vector2f((corner_size + 0.5f), size[1]);
    octagon[3] = position + vector2f(size[0] - (corner_size + 0.5f), size[1]);
    octagon[4] = position + vector2f(size[0], size[1] - (corner_size + 0.5f));
    octagon[5] = position + vector2f(size[0], (corner_size + 0.5f));
    octagon[6] = position + vector2f(size[0] - (corner_size + 0.5f), 0);
    octagon[7] = position + vector2f((corner_size + 0.5f), 0);
    get_surface().draw_filled_polygon(color, octagon, 8);
}

void generic_artist::draw_arrow(rgba8 const& color, box2i const& region,
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

void generic_artist::draw_focus_rect(box2i const& rect) const
{
    draw_focus_rect(rect, active_style_colors->normal_fg);
}

void generic_artist::draw_focus_rect(box2i const& rect,
    rgba8 const& color) const
{
    box2f r(rect);
    r.corner[0] += 0.375;
    r.corner[1] += 0.375;
    r.size[0] -= 1;
    r.size[1] -= 1;
    point2f poly[4];
    make_polygon(poly, r);
    get_surface().draw_line_loop(color, line_style(1, dotted_line), poly, 4);
}

void generic_artist::draw_outline(box2i const& region,
    rgba8 const& color) const
{
    box2f adjusted_rect(
        point2f(region.corner) + vector2f(0.5, 0.5),
        vector2f(region.size) - vector2f(1, 1));
    point2f poly[4];
    make_polygon(poly, adjusted_rect);
    get_surface().draw_line_loop(color, line_style(1, solid_line), poly, 4);
}

// PROGRESS BAR

vector2i generic_artist::get_default_progress_bar_size() const
{
    return vector2i(100, 20);
}
vector2i generic_artist::get_minimum_progress_bar_size() const
{
    return vector2i(40, 20);
}
void generic_artist::draw_progress_bar(artist_data_ptr& data,
    box2i const& region, double value) const
{
    draw_outline(region, active_style_colors->normal_fg);
    box2i bar_rect(
        region.corner + vector2i(2, 2),
        region.size - vector2i(4, 4));
    bar_rect.size[0] = int(bar_rect.size[0] * value + 0.5);
    point2i poly[4];
    make_polygon(poly, bar_rect);
    get_surface().draw_filled_polygon(dark_blue, poly, 4);
}

// ICON BUTTONS

static vector2i icon_button_size(15, 15);

vector2i generic_artist::get_icon_button_size(artist_data_ptr& data,
    standard_icon icon)
{
    return icon_button_size;
}
void generic_artist::draw_icon_button(artist_data_ptr& data,
    standard_icon icon, point2i const& position, widget_state state)
{
    box2i region(position, icon_button_size);

    point2i poly[4];
    make_polygon(poly, region);
    get_surface().draw_filled_polygon(get_bg_color(state), poly, 4);

    if ((state & widget_states::FOCUSED) != 0)
        draw_focus_rect(region);

    rgba8 fg_color = get_fg_color(state);
    // TODO
}

}
