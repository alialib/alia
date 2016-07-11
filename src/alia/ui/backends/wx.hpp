#ifndef ALIA_UI_BACKENDS_WX_HPP
#define ALIA_UI_BACKENDS_WX_HPP

#include <alia/ui/api.hpp>
#include <alia/ui/backends/interface.hpp>
#include <wx/wx.h>
#include <wx/glcanvas.h>

namespace alia {

struct style_tree;

// wx_opengl_window is a wxGLCanvas with an associated alia UI.
// It takes care of dispatching events received by the canvas to the UI.
struct wx_opengl_window : public wxGLCanvas
{
 public:
    wx_opengl_window(
        alia__shared_ptr<ui_controller> const& controller,
        alia__shared_ptr<style_tree> const& alia_style,
        wxWindow* parent,
        wxWindowID id = wxID_ANY,
        int const* attrib_list = 0,
        wxPoint const& pos = wxDefaultPosition,
        wxSize const& size = wxDefaultSize,
        long style = 0,
        wxString const& name = "alia_wx_gl_window",
        wxPalette const& palette = wxNullPalette);
    ~wx_opengl_window();

    alia::ui_system& ui();

    void update();

    void on_paint(wxPaintEvent& event);
    void on_erase_background(wxEraseEvent& event);
    void on_size(wxSizeEvent& event);
    void on_mouse(wxMouseEvent& event);
    void on_set_focus(wxFocusEvent& event);
    void on_kill_focus(wxFocusEvent& event);
    void on_idle(wxIdleEvent& event);
    void on_key_down(wxKeyEvent& event);
    void on_key_up(wxKeyEvent& event);
    void on_char(wxKeyEvent& event);
    void on_menu(wxCommandEvent& event);
    void on_sys_color_change(wxSysColourChangedEvent& event);

    struct impl_data;
 private:
    impl_data* impl_;
    DECLARE_EVENT_TABLE()
};

// wx_frame is a variant of wxFrame that implements the app_window interface.
struct wx_frame : public wxFrame, app_window
{
    wx_frame(
        alia__shared_ptr<app_window_controller> const& controller,
        wxWindow* parent,
        wxWindowID id,
        wxString const& title,
        wxPoint const& pos = wxDefaultPosition,
        wxSize const& size = wxDefaultSize,
        long style = wxDEFAULT_FRAME_STYLE,
        wxString const& name = "alia_wx_frame");
    ~wx_frame();

    app_window_state state() const;

    bool is_full_screen() const;
    void set_full_screen(bool fs);

    void close();

    void on_menu(wxCommandEvent& event);
    void on_move(wxMoveEvent& event);
    void on_size(wxSizeEvent& event);
    void on_close(wxCloseEvent& event);

    void update_menu_bar(wxWindow* controller, menu_container const& menu_bar);

    struct impl_data;
 private:
    impl_data* impl_;
    DECLARE_EVENT_TABLE()
};

// Create a wx_frame with a wx_opengl_window inside, filling the frame.
// If don't need the flexibility to create wxWidgets UI elements yourself,
// then this is the simplest way to get alia working with wxWdigets.
wx_frame*
create_wx_framed_window(
    string const& title,
    alia__shared_ptr<app_window_controller> const& controller,
    alia__shared_ptr<style_tree> const& style,
    app_window_state const& initial_state,
    int const* gl_canvas_attribs);

}

#endif
