#ifndef ALIA_NATIVE_WIN32_ARTIST_HPP
#define ALIA_NATIVE_WIN32_ARTIST_HPP

#include <alia/generic_artist.hpp>

namespace alia { namespace native {

class artist : public generic_artist
{
 public:
    artist();
    ~artist();

    void initialize();

    font translate_standard_font(standard_font font) const;

    void on_system_theme_change();
    void on_font_size_adjustment_change();

    vector2i get_button_size(artist_data_ptr& data,
        vector2i const& content_size) const;
    vector2i get_button_content_offset(artist_data_ptr& data,
        vector2i const& content_size, widget_state state) const;
    void draw_button(artist_data_ptr& data, box2i const& region,
        widget_state state) const;

    vector2i get_check_box_size(artist_data_ptr& data, bool checked) const;
    void draw_check_box(artist_data_ptr& data, bool checked,
        point2i const& position, widget_state state) const;

    vector2i get_radio_button_size(artist_data_ptr& data,
        bool selected) const;
    void draw_radio_button(artist_data_ptr& data, bool selected,
        point2i const& position, widget_state state) const;

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

    //vector2i get_minimum_drop_down_button_size() const;
    //void draw_drop_down_button(artist_data_ptr& data,
    //    box2i const& rect, widget_state state) const;

    int get_slider_left_border() const;
    int get_slider_right_border() const;
    int get_slider_height() const;
    int get_default_slider_width() const;
    //box1i get_slider_track_region() const;
    //box1i get_slider_track_hot_region() const;
    box2i get_slider_thumb_region() const;
    //void draw_slider_track(artist_data_ptr& data, unsigned axis,
    //    int width, point2i const& position) const;
    void draw_slider_thumb(artist_data_ptr& data, unsigned axis,
        point2i const& position, widget_state state) const;

 private:
    void initialize_fixed_widgets();
    void initialize_fonts();

    struct impl_data;
    impl_data* impl_;
};

}}

#endif
