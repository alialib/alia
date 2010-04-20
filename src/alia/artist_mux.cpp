#include <alia/artist_mux.hpp>

namespace alia {

artist_mux::artist_mux(boost::shared_ptr<artist> const& artist0,
        boost::shared_ptr<artist> const& artist1,
        uint32 selector)
{
    artists_[0] = artist0;
    artists_[1] = artist1;
    selector_ = selector;
}

void artist_mux::initialize()
{
    for (unsigned i = 0; i != 2; ++i)
    {
        artists_[i]->set_context(get_context());
        artists_[i]->initialize();
    }
}

void artist_mux::on_system_theme_change()
{
    for (unsigned i = 0; i != 2; ++i)
        artists_[i]->on_system_theme_change();
}

void artist_mux::on_font_scale_factor_change()
{
    for (unsigned i = 0; i != 2; ++i)
        artists_[i]->on_font_scale_factor_change();
}

void artist_mux::set_color_scheme(unsigned scheme)
{
    for (unsigned i = 0; i != 2; ++i)
        artists_[i]->on_system_theme_change();
}

unsigned artist_mux::get_code_for_style(style s, widget_state state,
    bool selected)
{
    unsigned which = (selector_ & STYLES) != 0 ? 1 : 0;
    return artists_[which]->get_code_for_style(s, state, selected);
}

void artist_mux::activate_style(unsigned style_code)
{
    unsigned which = (selector_ & STYLES) != 0 ? 1 : 0;
    // TODO: This is wrong, because the style codes could be different, but
    // it works OK for now.
    artists_[1 - which]->activate_style(style_code);
    artists_[which]->activate_style(style_code);
}
void artist_mux::restore_style(unsigned style_code)
{
    unsigned which = (selector_ & STYLES) != 0 ? 1 : 0;
    artists_[1 - which]->activate_style(style_code);
    artists_[which]->restore_style(style_code);
}

font artist_mux::translate_standard_font(standard_font font) const
{
    unsigned which = (selector_ & STYLES) != 0 ? 1 : 0;
    return artists_[which]->translate_standard_font(font);
}

void artist_mux::draw_focus_rect(box2i const& rect,
    rgba8 const& color) const
{
    unsigned which = (selector_ & STYLES) != 0 ? 1 : 0;
    return artists_[which]->draw_focus_rect(rect, color);
}

// BUTTON

vector2i artist_mux::get_button_size(artist_data_ptr& data,
    vector2i const& content_size) const
{
    unsigned which = (selector_ & BUTTON) != 0 ? 1 : 0;
    return artists_[which]->get_button_size(data, content_size);
}

vector2i artist_mux::get_button_content_offset(artist_data_ptr& data,
    vector2i const& content_size, widget_state state) const
{
    unsigned which = (selector_ & BUTTON) != 0 ? 1 : 0;
    return artists_[which]->get_button_content_offset(data, content_size,
        state);
}

void artist_mux::draw_button(artist_data_ptr& data, box2i const& region,
    widget_state state) const
{
    unsigned which = (selector_ & BUTTON) != 0 ? 1 : 0;
    artists_[which]->draw_button(data, region, state);
}

// LINK

rgba8 artist_mux::get_link_color(artist_data_ptr& data,
    widget_state state) const
{
    unsigned which = (selector_ & LINK) != 0 ? 1 : 0;
    return artists_[which]->get_link_color(data, state);
}

// CHECK BOX

vector2i artist_mux::get_check_box_size(artist_data_ptr& data,
    bool checked) const
{
    unsigned which = (selector_ & CHECK_BOX) != 0 ? 1 : 0;
    return artists_[which]->get_check_box_size(data, checked);
}
void artist_mux::draw_check_box(artist_data_ptr& data, bool checked,
    point2i const& position, widget_state state) const
{
    unsigned which = (selector_ & CHECK_BOX) != 0 ? 1 : 0;
    artists_[which]->draw_check_box(data, checked, position, state);
}

// RADIO BUTTON

vector2i artist_mux::get_radio_button_size(artist_data_ptr& data,
    bool selected) const
{
    unsigned which = (selector_ & RADIO_BUTTON) != 0 ? 1 : 0;
    return artists_[which]->get_radio_button_size(data, selected);
}
void artist_mux::draw_radio_button(artist_data_ptr& data, bool selected,
    point2i const& position, widget_state state) const
{
    unsigned which = (selector_ & RADIO_BUTTON) != 0 ? 1 : 0;
    artists_[which]->draw_radio_button(data, selected, position, state);
}

// NODE EXPANDER

vector2i artist_mux::get_node_expander_size(artist_data_ptr& data,
    int expanded) const
{
    unsigned which = (selector_ & NODE_EXPANDER) != 0 ? 1 : 0;
    return artists_[which]->get_node_expander_size(data, expanded);
}
void artist_mux::draw_node_expander(artist_data_ptr& data, int expanded,
    point2i const& position, widget_state state) const
{
    unsigned which = (selector_ & NODE_EXPANDER) != 0 ? 1 : 0;
    artists_[which]->draw_node_expander(data, expanded, position, state);
}

// SEPARATOR

int artist_mux::get_separator_width() const
{
    unsigned which = (selector_ & SEPARATOR) != 0 ? 1 : 0;
    return artists_[which]->get_separator_width();
}
void artist_mux::draw_separator(artist_data_ptr& data,
    point2i const& position, unsigned axis, int length) const
{
    unsigned which = (selector_ & SEPARATOR) != 0 ? 1 : 0;
    artists_[which]->draw_separator(data, position, axis, length);
}

// SCROLLBAR

int artist_mux::get_scrollbar_width() const
{
    unsigned which = (selector_ & SCROLLBAR) != 0 ? 1 : 0;
    return artists_[which]->get_scrollbar_width();
}
int artist_mux::get_scrollbar_button_length() const
{
    unsigned which = (selector_ & SCROLLBAR) != 0 ? 1 : 0;
    return artists_[which]->get_scrollbar_button_length();
}
int artist_mux::get_minimum_scrollbar_thumb_length() const
{
    unsigned which = (selector_ & SCROLLBAR) != 0 ? 1 : 0;
    return artists_[which]->get_minimum_scrollbar_thumb_length();
}

void artist_mux::draw_scrollbar_background(artist_data_ptr& data,
    box2i const& rect, int axis, int which_end, widget_state state) const
{
    unsigned which = (selector_ & SCROLLBAR) != 0 ? 1 : 0;
    artists_[which]->draw_scrollbar_background(data, rect, axis, which_end,
        state);
}

void artist_mux::draw_scrollbar_thumb(artist_data_ptr& data,
    box2i const& rect, int axis, widget_state state) const
{
    unsigned which = (selector_ & SCROLLBAR) != 0 ? 1 : 0;
    artists_[which]->draw_scrollbar_thumb(data, rect, axis, state);
}

void artist_mux::draw_scrollbar_button(artist_data_ptr& data,
    point2i const& position, int axis, int which_end,
    widget_state state) const
{
    unsigned which = (selector_ & SCROLLBAR) != 0 ? 1 : 0;
    artists_[which]->draw_scrollbar_button(data, position, axis, which_end,
        state);
}

void artist_mux::draw_scrollbar_junction(artist_data_ptr& data,
    point2i const& position) const
{
    unsigned which = (selector_ & SCROLLBAR) != 0 ? 1 : 0;
    artists_[which]->draw_scrollbar_junction(data, position);
}

// PANEL

border_size artist_mux::get_panel_border_size(artist_data_ptr& data,
    unsigned inner_style_code) const
{
    unsigned which = (selector_ & PANEL) != 0 ? 1 : 0;
    return artists_[which]->get_panel_border_size(data, inner_style_code);
}
void artist_mux::draw_panel_border(artist_data_ptr& data,
    unsigned inner_style_code, box2i const& rect) const
{
    unsigned which = (selector_ & PANEL) != 0 ? 1 : 0;
    artists_[which]->draw_panel_border(data, inner_style_code, rect);
}

void artist_mux::draw_panel_background(artist_data_ptr& data,
    box2i const& rect) const
{
    unsigned which = (selector_ & PANEL) != 0 ? 1 : 0;
    artists_[which]->draw_panel_background(data, rect);
}

// DROP DOWN BUTTON

vector2i artist_mux::get_minimum_drop_down_button_size() const
{
    unsigned which = (selector_ & DROP_DOWN) != 0 ? 1 : 0;
    return artists_[which]->get_minimum_drop_down_button_size();
}
void artist_mux::draw_drop_down_button(artist_data_ptr& data,
    box2i const& rect, widget_state state) const
{
    unsigned which = (selector_ & DROP_DOWN) != 0 ? 1 : 0;
    artists_[which]->draw_drop_down_button(data, rect, state);
}

// SLIDER

int artist_mux::get_slider_left_border() const
{
    unsigned which = (selector_ & SLIDER) != 0 ? 1 : 0;
    return artists_[which]->get_slider_left_border();
}
int artist_mux::get_slider_right_border() const
{
    unsigned which = (selector_ & SLIDER) != 0 ? 1 : 0;
    return artists_[which]->get_slider_right_border();
}
int artist_mux::get_slider_height() const
{
    unsigned which = (selector_ & SLIDER) != 0 ? 1 : 0;
    return artists_[which]->get_slider_height();
}
int artist_mux::get_default_slider_width() const
{
    unsigned which = (selector_ & SLIDER) != 0 ? 1 : 0;
    return artists_[which]->get_default_slider_width();
}
box1i artist_mux::get_slider_track_region() const
{
    unsigned which = (selector_ & SLIDER) != 0 ? 1 : 0;
    return artists_[which]->get_slider_track_region();
}
box1i artist_mux::get_slider_track_hot_region() const
{
    unsigned which = (selector_ & SLIDER) != 0 ? 1 : 0;
    return artists_[which]->get_slider_track_hot_region();
}
box2i artist_mux::get_slider_thumb_region() const
{
    unsigned which = (selector_ & SLIDER) != 0 ? 1 : 0;
    return artists_[which]->get_slider_thumb_region();
}
void artist_mux::draw_slider_track(artist_data_ptr& data, unsigned axis,
    int width, point2i const& position) const
{
    unsigned which = (selector_ & SLIDER) != 0 ? 1 : 0;
    artists_[which]->draw_slider_track(data, axis, width, position);
}
void artist_mux::draw_slider_thumb(artist_data_ptr& data, unsigned axis,
    point2i const& position, widget_state state) const
{
    unsigned which = (selector_ & SLIDER) != 0 ? 1 : 0;
    artists_[which]->draw_slider_thumb(data, axis, position, state);
}

// PROGRESS BAR

vector2i artist_mux::get_default_progress_bar_size() const
{
    unsigned which = (selector_ & PROGRESS_BAR) != 0 ? 1 : 0;
    return artists_[which]->get_default_progress_bar_size();
}
vector2i artist_mux::get_minimum_progress_bar_size() const
{
    unsigned which = (selector_ & PROGRESS_BAR) != 0 ? 1 : 0;
    return artists_[which]->get_minimum_progress_bar_size();
}
void artist_mux::draw_progress_bar(artist_data_ptr& data,
    box2i const& region, double value) const
{
    unsigned which = (selector_ & PROGRESS_BAR) != 0 ? 1 : 0;
    artists_[which]->draw_progress_bar(data, region, value);
}

// ICONS

vector2i artist_mux::get_icon_button_size(artist_data_ptr& data,
    standard_icon icon)
{
    unsigned which = (selector_ & PROGRESS_BAR) != 0 ? 1 : 0;
    return artists_[which]->get_icon_button_size(data, icon);
}
void artist_mux::draw_icon_button(artist_data_ptr& data,
    standard_icon icon, point2i const& position, widget_state state)
{
    unsigned which = (selector_ & PROGRESS_BAR) != 0 ? 1 : 0;
    artists_[which]->draw_icon_button(data, icon, position, state);
}

}
