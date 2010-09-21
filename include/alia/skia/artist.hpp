#ifndef ALIA_SKIA_ARTIST_HPP
#define ALIA_SKIA_ARTIST_HPP

#include <alia/artist.hpp>
#include <alia/color.hpp>

namespace alia { namespace skia {

class artist : public alia::artist
{
 public:
    void initialize();

    void draw_focus_rect(box2i const& rect, rgba8 const& color) const;

    void draw_focus_rect(artist_data_ptr& data, box2i const& rect) const;

    vector2i get_button_size(artist_data_ptr& data,
        vector2i const& content_size) const;
    rgba8 get_button_text_color(artist_data_ptr& data,
        widget_state state) const;
    vector2i get_button_content_offset(artist_data_ptr& data,
        vector2i const& content_size, widget_state state) const;
    void draw_button(artist_data_ptr& data, box2i const& region,
        widget_state state) const;

    rgba8 get_link_color(artist_data_ptr& data, widget_state state) const;

    vector2i get_check_box_size(artist_data_ptr& data, bool checked) const;
    void draw_check_box(artist_data_ptr& data, bool checked,
        point2i const& position, widget_state state) const;

    vector2i get_radio_button_size(artist_data_ptr& data,
        bool selected) const;
    void draw_radio_button(artist_data_ptr& data, bool selected,
        point2i const& position, widget_state state) const;

    vector2i get_node_expander_size(artist_data_ptr& data,
        int expanded) const;
    void draw_node_expander(artist_data_ptr& data, int expanded,
        point2i const& position, widget_state state) const;

    int get_separator_width(artist_data_ptr& data) const;
    void draw_separator(artist_data_ptr& data,
        point2i const& position, unsigned axis, int length) const;

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

    border_size get_panel_border_size(artist_data_ptr& data,
        unsigned inner_style_code) const;
    void draw_panel_border(artist_data_ptr& data,
        unsigned inner_style_code, box2i const& rect) const;
    void draw_panel_background(artist_data_ptr& data,
        box2i const& rect) const;

    vector2i get_minimum_drop_down_button_size() const;
    void draw_drop_down_button(artist_data_ptr& data,
        box2i const& rect, widget_state state) const;

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

    vector2i get_default_progress_bar_size() const;
    vector2i get_minimum_progress_bar_size() const;
    void draw_progress_bar(artist_data_ptr& data,
        box2i const& region, double value) const;

    vector2i get_icon_button_size(artist_data_ptr& data,
        standard_icon icon);
    void draw_icon_button(artist_data_ptr& data, standard_icon icon,
        point2i const& position, widget_state state);

    // direction: 0: left, 1 : right, 2: up, 3: down
    // 4: top-left, 5: top-right, 6: bottom-right, 7: bottom-left
    void draw_arrow(rgba8 const& color, box2i const& region, int direction,
        int size) const;

    void draw_outline(box2i const& region, rgba8 const& color) const;
};

}}

#endif
