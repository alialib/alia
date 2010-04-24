#ifndef ALIA_ARTIST_HPP
#define ALIA_ARTIST_HPP

#include <alia/forward.hpp>
#include <alia/surface.hpp>
#include <alia/widget_state.hpp>
#include <boost/scoped_ptr.hpp>

namespace alia {

enum standard_font
{
    NORMAL_FONT,
    FIXED_FONT,
};

enum style
{
    BACKGROUND_STYLE,
    DIALOG_STYLE,
    CONTENT_STYLE,
    ODD_CONTENT_STYLE,
    TITLE_STYLE,
    HEADING_STYLE,
    SUBHEADING_STYLE,
    HIGHLIGHTED_STYLE,
    LIST_STYLE,
    ITEM_STYLE,
    TEXT_CONTROL_STYLE,
    OVERLAY_STYLE,
};

enum standard_icon
{
    REMOVE_ICON,
};

struct border_size
{
    border_size(int left, int top, int right, int bottom)
      : left(left), top(top), right(right), bottom(bottom) {}
    border_size() {}
    int left, top, right, bottom;
};

struct artist_data
{
    virtual ~artist_data() {}
};
typedef boost::scoped_ptr<artist_data> artist_data_ptr;

class artist
{
 public:
    virtual ~artist() {}

    context& get_context() const { return *context_; }
    surface& get_surface() const { return *surface_; }

    void set_context(context& ctx);

    virtual void initialize() {}

    // This is called when the underlying window system changes themes.
    virtual void on_system_theme_change() {}

    // This is called when the context's font_scale_factor setting is
    // changed.
    virtual void on_font_scale_factor_change() {}

    // TODO: Enumeration.
    virtual void set_color_scheme(unsigned scheme) {}

    // Get the code for a particular style.  Styles are context-dependent, so
    // the code that's returned is dependent not only on the arguments, but
    // also on the style that's active when it's called.
    virtual unsigned get_code_for_style(style s, widget_state state = 0,
        bool selected = false) = 0;

    virtual void activate_style(unsigned style_code) = 0;
    virtual void restore_style(unsigned style_code) = 0;

    virtual font translate_standard_font(standard_font font) const = 0;

    virtual void draw_focus_rect(box2i const& rect) const = 0;

  // BUTTON

    // Get the size of a button, given the size of its contents.
    virtual vector2i get_button_size(artist_data_ptr& data,
        vector2i const& content_size) const = 0;

    // Get the position within a button at which the contents should be
    // rendered.
    virtual vector2i get_button_content_offset(artist_data_ptr& data,
        vector2i const& content_size, widget_state state) const = 0;

    virtual void draw_button(artist_data_ptr& data, box2i const& region,
        widget_state state) const = 0;

  // LINK

    virtual rgba8 get_link_color(artist_data_ptr& data,
        widget_state state) const = 0;

  // CHECK BOX

    virtual vector2i get_check_box_size(artist_data_ptr& data,
        bool checked) const = 0;
    virtual void draw_check_box(artist_data_ptr& data, bool checked,
        point2i const& position, widget_state state) const = 0;

  // RADIO BUTTON

    virtual vector2i get_radio_button_size(artist_data_ptr& data,
        bool selected) const = 0;
    virtual void draw_radio_button(artist_data_ptr& data, bool selected,
        point2i const& position, widget_state state) const = 0;

  // NODE EXPANDER

    virtual vector2i get_node_expander_size(artist_data_ptr& data,
        int expanded) const = 0;
    virtual void draw_node_expander(artist_data_ptr& data, int expanded,
        point2i const& position, widget_state state) const = 0;

  // SEPARATOR

    virtual int get_separator_width() const = 0;
    virtual void draw_separator(artist_data_ptr& data,
        point2i const& position, unsigned axis, int length) const = 0;

  // SCROLLBAR

    virtual int get_scrollbar_width() const = 0;
    virtual int get_scrollbar_button_length() const = 0;
    virtual int get_minimum_scrollbar_thumb_length() const = 0;

    // background
    virtual void draw_scrollbar_background(artist_data_ptr& data,
        box2i const& rect, int axis, int which, widget_state state) const = 0;

    // thumb
    virtual void draw_scrollbar_thumb(artist_data_ptr& data,
        box2i const& rect, int axis, widget_state state) const = 0;

    // button
    virtual void draw_scrollbar_button(artist_data_ptr& data,
        point2i const& position, int axis, int which,
        widget_state state) const = 0;

    // junction - the little square where two scrollbars meet
    virtual void draw_scrollbar_junction(artist_data_ptr& data,
        point2i const& position) const = 0;

  // PANEL

    // border
    virtual border_size get_panel_border_size(artist_data_ptr& data,
        unsigned inner_style_code) const = 0;
    virtual void draw_panel_border(artist_data_ptr& data,
        unsigned inner_style_code, box2i const& rect) const = 0;

    // background
    virtual void draw_panel_background(artist_data_ptr& data,
        box2i const& rect) const = 0;

  // DROP DOWN BUTTON

    virtual vector2i get_minimum_drop_down_button_size() const = 0;
    virtual void draw_drop_down_button(artist_data_ptr& data,
        box2i const& rect, widget_state state) const = 0;

  // SLIDER

    virtual int get_slider_left_border() const = 0;
    virtual int get_slider_right_border() const = 0;
    virtual int get_slider_height() const = 0;
    virtual int get_default_slider_width() const = 0;
    virtual box1i get_slider_track_region() const = 0;
    virtual box1i get_slider_track_hot_region() const = 0;
    virtual box2i get_slider_thumb_region() const = 0;
    virtual void draw_slider_track(artist_data_ptr& data, unsigned axis,
        int width, point2i const& position) const = 0;
    virtual void draw_slider_thumb(artist_data_ptr& data, unsigned axis,
        point2i const& position, widget_state state) const = 0;

  // PROGRESS BAR

    virtual vector2i get_default_progress_bar_size() const = 0;
    virtual vector2i get_minimum_progress_bar_size() const = 0;
    virtual void draw_progress_bar(artist_data_ptr& data,
        box2i const& region, double value) const = 0;

  // ICONS

    virtual vector2i get_icon_button_size(artist_data_ptr& data,
        standard_icon icon) = 0;
    virtual void draw_icon_button(artist_data_ptr& data, standard_icon icon,
        point2i const& position, widget_state state) = 0;

 private:
    context* context_;
    surface* surface_;
};

}

#endif
