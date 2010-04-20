#ifndef ALIA_ARTIST_MUX_HPP
#define ALIA_ARTIST_MUX_HPP

#include <alia/artist.hpp>
#include <boost/shared_ptr.hpp>
#include <alia/typedefs.hpp>

namespace alia {

class artist_mux : public artist
{
 public:
    static uint32 const STYLES = 0x1;
    static uint32 const BUTTON = 0x2;
    static uint32 const LINK = 0x4;
    static uint32 const CHECK_BOX = 0x8;
    static uint32 const RADIO_BUTTON = 0x10;
    static uint32 const NODE_EXPANDER = 0x20;
    static uint32 const SEPARATOR = 0x40;
    static uint32 const SCROLLBAR = 0x80;
    static uint32 const PANEL = 0x100;
    static uint32 const DROP_DOWN = 0x200;
    static uint32 const SLIDER = 0x400;
    static uint32 const PROGRESS_BAR = 0x800;
    static uint32 const ICONS = 0x1000;

    artist_mux(boost::shared_ptr<artist> const& artist0,
        boost::shared_ptr<artist> const& artist1,
        uint32 selector);

    void initialize();

    void on_system_theme_change();

    void on_font_scale_factor_change();

    void set_color_scheme(unsigned scheme);

    unsigned get_code_for_style(style s, widget_state state,
        bool selected);

    void activate_style(unsigned style_code);
    void restore_style(unsigned style_code);

    font translate_standard_font(standard_font font) const;

    void draw_focus_rect(box2i const& rect,
        rgba8 const& color) const;

  // BUTTON

    vector2i get_button_size(artist_data_ptr& data,
        vector2i const& content_size) const;

    vector2i get_button_content_offset(artist_data_ptr& data,
        vector2i const& content_size, widget_state state) const;

    void draw_button(artist_data_ptr& data, box2i const& region,
        widget_state state) const;

  // LINK

    rgba8 get_link_color(artist_data_ptr& data,
        widget_state state) const;

  // CHECK BOX

    vector2i get_check_box_size(artist_data_ptr& data,
        bool checked) const;
    void draw_check_box(artist_data_ptr& data, bool checked,
        point2i const& position, widget_state state) const;

  // RADIO BUTTON

    vector2i get_radio_button_size(artist_data_ptr& data,
        bool selected) const;
    void draw_radio_button(artist_data_ptr& data, bool selected,
        point2i const& position, widget_state state) const;

  // NODE EXPANDER

    vector2i get_node_expander_size(artist_data_ptr& data,
        int expanded) const;
    void draw_node_expander(artist_data_ptr& data, int expanded,
        point2i const& position, widget_state state) const;

  // SEPARATOR

    int get_separator_width() const;
    void draw_separator(artist_data_ptr& data,
        point2i const& position, unsigned axis, int length) const;

  // SCROLLBAR

    int get_scrollbar_width() const;
    int get_scrollbar_button_length() const;
    int get_minimum_scrollbar_thumb_length() const;

    void draw_scrollbar_background(artist_data_ptr& data,
        box2i const& rect, int axis, int which, widget_state state) const;

    void draw_scrollbar_thumb(artist_data_ptr& data,
        box2i const& rect, int axis, widget_state state) const;

    void draw_scrollbar_button(artist_data_ptr& data,
        point2i const& position, int axis, int which,
        widget_state state) const;

    void draw_scrollbar_junction(artist_data_ptr& data,
        point2i const& position) const;

  // PANEL

    border_size get_panel_border_size(artist_data_ptr& data,
        unsigned inner_style_code) const;
    void draw_panel_border(artist_data_ptr& data,
        unsigned inner_style_code, box2i const& rect) const;

    void draw_panel_background(artist_data_ptr& data,
        box2i const& rect) const;

  // DROP DOWN BUTTON

    vector2i get_minimum_drop_down_button_size() const;
    void draw_drop_down_button(artist_data_ptr& data,
        box2i const& rect, widget_state state) const;

  // SLIDER

    int get_slider_left_border() const;
    int get_slider_right_border() const;
    int get_slider_height() const;
    int get_default_slider_width() const;
    box1i get_slider_track_region() const;
    box1i get_slider_track_hot_region() const;
    box2i get_slider_thumb_region() const;
    void draw_slider_track(artist_data_ptr& data, unsigned axis,
        int width, point2i const& position) const;
    void draw_slider_thumb(artist_data_ptr& data, unsigned axis,
        point2i const& position, widget_state state) const;

  // PROGRESS BAR

    vector2i get_default_progress_bar_size() const;
    vector2i get_minimum_progress_bar_size() const;
    void draw_progress_bar(artist_data_ptr& data,
        box2i const& region, double value) const;

  // ICONS

    vector2i get_icon_button_size(artist_data_ptr& data,
        standard_icon icon);
    void draw_icon_button(artist_data_ptr& data, standard_icon icon,
        point2i const& position, widget_state state);

 private:
    boost::shared_ptr<artist> artists_[2];
    uint32 selector_;
};

}

#endif
