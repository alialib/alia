#ifndef ALIA_WX_OPENGL_SURFACE_HPP
#define ALIA_WX_OPENGL_SURFACE_HPP

#include <alia/opengl/surface.hpp>
#include <alia/wx/forward.hpp>
#include <alia/wx/wx.hpp>
#include <alia/native/ascii_text_renderer.hpp>

namespace alia { namespace wx {

class opengl_surface : public opengl::surface
{
 public:
    opengl_surface();
    ~opengl_surface();
    void set_holder(context_holder* holder);
    void create(wxWindow* parent);
    void update();
    wxWindow* get_wx_window();
    void set_current();
    //bool process_input_event(input_event const& event);
    void remove_popup(popup_window* popup);
    // event handlers that might be called by parents
    void handle_key_down(wxKeyEvent* event);
    void handle_key_up(wxKeyEvent* event);
    void handle_char(wxKeyEvent* event);

    // implementation of the surface interface...

    bool is_printer() const { return false; }
    bool is_single_use() const { return false; }
    vector2d get_ppi() const;
    vector2i get_display_size() const;
    point2i get_location_on_display() const;

    void get_font_metrics(font_metrics* metrics, font const& font);
    void cache_text(
        cached_text_ptr& data,
        font const& font,
        char const* text,
        int width,
        unsigned flags);
    vector2i get_ascii_text_size(font const& font, char const* text);
    void draw_ascii_text(point2d const& location, rgba8 const& color,
        font const& font, char const* text);

    void set_mouse_cursor(mouse_cursor cursor);
    void request_timer_event(region_id id, unsigned ms);
    std::string get_clipboard_text();
    void set_clipboard_text(std::string const& text);
    void close();
    popup_interface* open_popup(controller* controller,
        point2i const& display_location, point2i const& boundary,
        vector2i const& minimum_size, vector2i const& maximum_size);
    void show_popup_menu(menu_controller* controller);
    void beep();
    bool ask_for_color(rgb8* result, rgb8 const* initial = 0);

 private:
    void handle_paint();
    void handle_resize();
    void handle_mouse(wxMouseEvent const& event);
    void handle_focus_gain();
    void handle_focus_loss();
    void handle_idle(wxIdleEvent* event);
    void handle_menu(int id);
    void handle_sys_color_change();
    void handle_update();
    native::ascii_text_renderer* get_ascii_text_renderer(font const& font);
    void update_popups();
    void orphan_popups();
    void update_size();
    void on_close();
    void schedule_update();
    void update_mouse_cursor();
    context& get_context() const;

    class gl_canvas_wrapper;
    class scoped_selected_menu_id_ptr;

    struct impl_data;
    impl_data* impl_;
};

}}

#endif
