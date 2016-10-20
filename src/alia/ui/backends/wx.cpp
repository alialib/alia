#include <alia/ui/backends/wx.hpp>
#include <alia/ui/system.hpp>
#include <alia/ui/backends/opengl.hpp>
#include <alia/ui/utilities.hpp>

#include <wx/stopwatch.h>
#include <wx/clipbrd.h>
#include <wx/utils.h>

namespace alia {

// ENUM TRANSLATION

key_code static
translate_key_code(int code)
{
    // Translate letters to their lowercase equivalents.
    if (code >= 0x41 && code <= 0x5a)
    {
        return key_code(code + 0x20);
    }

    switch (code)
    {
     case WXK_BACK:
        return KEY_BACKSPACE;
     case WXK_TAB:
        return KEY_TAB;
     case WXK_CLEAR:
        return KEY_CLEAR;
     case WXK_RETURN:
        return KEY_ENTER;
     case WXK_PAUSE:
        return KEY_PAUSE;
     case WXK_ESCAPE:
        return KEY_ESCAPE;
     case WXK_SPACE:
        return KEY_SPACE;
     case WXK_PAGEUP:
        return KEY_PAGEUP;
     case WXK_PAGEDOWN:
        return KEY_PAGEDOWN;
     case WXK_END:
        return KEY_END;
     case WXK_HOME:
        return KEY_HOME;
     case WXK_UP:
        return KEY_UP;
     case WXK_DOWN:
        return KEY_DOWN;
     case WXK_LEFT:
        return KEY_LEFT;
     case WXK_RIGHT:
        return KEY_RIGHT;
     case WXK_PRINT:
        return KEY_PRINT_SCREEN;
     case WXK_INSERT:
        return KEY_INSERT;
     case WXK_DELETE:
        return KEY_DELETE;
     case WXK_HELP:
        return KEY_HELP;
     case WXK_F1:
        return KEY_F1;
     case WXK_F2:
        return KEY_F2;
     case WXK_F3:
        return KEY_F3;
     case WXK_F4:
        return KEY_F4;
     case WXK_F5:
        return KEY_F5;
     case WXK_F6:
        return KEY_F6;
     case WXK_F7:
        return KEY_F7;
     case WXK_F8:
        return KEY_F8;
     case WXK_F9:
        return KEY_F9;
     case WXK_F10:
        return KEY_F10;
     case WXK_F11:
        return KEY_F11;
     case WXK_F12:
        return KEY_F12;
     case WXK_F13:
        return KEY_F13;
     case WXK_F14:
        return KEY_F14;
     case WXK_F15:
        return KEY_F15;
     case WXK_F16:
        return KEY_F16;
     case WXK_F17:
        return KEY_F17;
     case WXK_F18:
        return KEY_F18;
     case WXK_F19:
        return KEY_F19;
     case WXK_F20:
        return KEY_F20;
     case WXK_F21:
        return KEY_F21;
     case WXK_F22:
        return KEY_F22;
     case WXK_F23:
        return KEY_F23;
     case WXK_F24:
        return KEY_F24;
     case WXK_NUMPAD_ENTER:
        return KEY_ENTER;
     case WXK_NUMPAD0:
        return key_code('0');
     case WXK_NUMPAD1:
        return key_code('1');
     case WXK_NUMPAD2:
        return key_code('2');
     case WXK_NUMPAD3:
        return key_code('3');
     case WXK_NUMPAD4:
        return key_code('4');
     case WXK_NUMPAD5:
        return key_code('5');
     case WXK_NUMPAD6:
        return key_code('6');
     case WXK_NUMPAD7:
        return key_code('7');
     case WXK_NUMPAD8:
        return key_code('8');
     case WXK_NUMPAD9:
        return key_code('9');
     case WXK_NUMPAD_ADD:
        return key_code('+');
     case WXK_NUMPAD_SUBTRACT:
        return key_code('-');
     case WXK_NUMPAD_DIVIDE:
        return key_code('/');
     case WXK_NUMPAD_MULTIPLY:
        return key_code('*');
     case WXK_NUMPAD_DECIMAL:
        return key_code('.');
     case WXK_NUMPAD_EQUAL:
        return KEY_EQUALS;
     case WXK_NUMPAD_UP:
        return KEY_UP;
     case WXK_NUMPAD_DOWN:
        return KEY_DOWN;
     case WXK_NUMPAD_LEFT:
        return KEY_LEFT;
     case WXK_NUMPAD_RIGHT:
        return KEY_RIGHT;
     case WXK_NUMPAD_INSERT:
        return KEY_INSERT;
     case WXK_NUMPAD_DELETE:
        return KEY_DELETE;
     case WXK_NUMPAD_HOME:
        return KEY_HOME;
     case WXK_NUMPAD_END:
        return KEY_END;
    }

    // Return ASCII characters untranslated.
    if (code < 0x80)
        return key_code(code);

    return KEY_UNKNOWN;
}

key_event_info static
get_key_event_info(wxKeyEvent const& event)
{
    key_modifiers mods = KMOD_NONE;
    if (event.ShiftDown())
        mods |= KMOD_SHIFT;
    if (event.ControlDown())
        mods |= KMOD_CTRL;
    if (event.AltDown())
        mods |= KMOD_ALT;
    if (event.MetaDown())
        mods |= KMOD_META;
    return key_event_info(translate_key_code(event.GetKeyCode()), mods);
}

wxCursor static
translate_mouse_cursor(mouse_cursor cursor)
{
    switch (cursor)
    {
     case DEFAULT_CURSOR:
     default:
        return wxCursor(wxCURSOR_ARROW);
     case CROSS_CURSOR:
        return wxCursor(wxCURSOR_CROSS);
     case BUSY_CURSOR:
        return wxCursor(wxCURSOR_WAIT);
     case BLANK_CURSOR:
        return wxCursor(wxCURSOR_BLANK);
     case IBEAM_CURSOR:
        return wxCursor(wxCURSOR_IBEAM);
     case NO_ENTRY_CURSOR:
        return wxCursor(wxCURSOR_NO_ENTRY);
     case OPEN_HAND_CURSOR:
        return wxCursor(wxCURSOR_HAND);
     case POINTING_HAND_CURSOR:
        return wxCursor(wxCURSOR_HAND);
     case LEFT_RIGHT_ARROW_CURSOR:
        return wxCursor(wxCURSOR_SIZEWE);
     case UP_DOWN_ARROW_CURSOR:
        return wxCursor(wxCURSOR_SIZENS);
     case FOUR_WAY_ARROW_CURSOR:
        return wxCursor(wxCURSOR_SIZING);
    }
}

// OS INTERFACE

struct wx_os_interface : os_interface
{
    string get_clipboard_text();
    void set_clipboard_text(string const& text);
};

string wx_os_interface::get_clipboard_text()
{
    std::string text;
    if (wxTheClipboard->Open())
    {
        if (wxTheClipboard->IsSupported(wxDF_TEXT))
        {
            wxTextDataObject data;
            wxTheClipboard->GetData(data);
            text = data.GetText().c_str();
        }
        wxTheClipboard->Close();
    }
    return text;
}

void wx_os_interface::set_clipboard_text(string const& text)
{
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(text.c_str()));
        wxTheClipboard->Flush();
        wxTheClipboard->Close();
    }
}

// OPENGL WINDOW

#define INVOKE_CALLBACK(callback) \
    try \
    { \
        callback \
    } \
    catch(std::exception& e) \
    { \
        show_error(e.what()); \
    } \
    catch(...) \
    { \
        show_error("An unknown error has occurred."); \
    }

BEGIN_EVENT_TABLE(wx_opengl_window, wxGLCanvas)
    EVT_PAINT(wx_opengl_window::on_paint)
    EVT_ERASE_BACKGROUND(wx_opengl_window::on_erase_background)
    EVT_SIZE(wx_opengl_window::on_size)
    EVT_MOUSE_EVENTS(wx_opengl_window::on_mouse)
    EVT_SET_FOCUS(wx_opengl_window::on_set_focus)
    EVT_KILL_FOCUS(wx_opengl_window::on_kill_focus)
    EVT_KEY_DOWN(wx_opengl_window::on_key_down)
    EVT_KEY_UP(wx_opengl_window::on_key_up)
    EVT_CHAR(wx_opengl_window::on_char)
    EVT_IDLE(wx_opengl_window::on_idle)
    EVT_MENU(-1, wx_opengl_window::on_menu)
    EVT_SYS_COLOUR_CHANGED(wx_opengl_window::on_sys_color_change)
END_EVENT_TABLE()

struct wx_opengl_window::impl_data
{
    ui_system ui;
    opengl_context alia_gl_context;
    wxGLContext* wx_gl_context;
    wx_opengl_window* window;
    int wheel_movement; // accumulates fractional mouse wheel movement
    bool vsync_disabled;
    counter_type last_menu_bar_update;
    // this is used to hold key info between events for the same key press
    wxKeyEvent last_key_down;
};

ui_time_type static
get_time(wx_opengl_window::impl_data& impl)
{
    return ui_time_type(wxGetLocalTimeMillis().GetLo());
}

void static
set_cursor(wx_opengl_window::impl_data& impl, mouse_cursor cursor)
{
    // wxCURSOR_BLANK doesn't seem to work on Windows, so instead just hide
    // the mouse cursor. (This also means we have to make sure the cursor is
    // shown again when we don't want a blank cursor.)
    #ifdef __WXMSW__
        if (cursor == BLANK_CURSOR)
        {
            while (ShowCursor(false) >= 0)
                ;
        }
        else
        {
            impl.window->SetCursor(translate_mouse_cursor(cursor));
            while (ShowCursor(true) < 0)
                ;
        }
    #else
        impl.window->SetCursor(translate_mouse_cursor(cursor));
    #endif
}

void static
update_window(wx_opengl_window::impl_data& impl)
{
    opengl_surface* surface =
        static_cast<opengl_surface*>(impl.ui.surface.get());
    vector<2,int> size;
    impl.window->GetClientSize(&size[0], &size[1]);

    mouse_cursor cursor;
    update_ui(impl.ui,
        vector<2,unsigned>(size),
        get_time(impl), &cursor);
    set_cursor(impl, cursor);

    // If the menu bar has changed, find the parent frame, test if it's an
    // alia frame, and if so, ask it to update its menu bar.
    if (impl.ui.menu_bar.last_change != impl.last_menu_bar_update)
    {
        wxWindow* frame = impl.window;
        while (!frame->IsTopLevel())
            frame = frame->GetParent();
        wx_frame* alia_frame = dynamic_cast<wx_frame*>(frame);
        if (alia_frame)
            alia_frame->update_menu_bar(impl.window, impl.ui.menu_bar);
        impl.last_menu_bar_update = impl.ui.menu_bar.last_change;
    }

    impl.window->Refresh(false);
    impl.window->Update();
}

void static
handle_paint(wx_opengl_window::impl_data& impl)
{
    opengl_surface* surface =
        static_cast<opengl_surface*>(impl.ui.surface.get());

    // Windows requires this
    wxPaintDC dc(impl.window);

    impl.window->SetCurrent(*impl.wx_gl_context);

    vector<2,int> size;
    impl.window->GetSize(&size[0], &size[1]);
    surface->initialize_render_state(vector<2,unsigned>(size));

    if (!impl.vsync_disabled)
    {
        disable_vsync();
        impl.vsync_disabled = true;
    }

    render_ui(impl.ui);

    impl.window->SwapBuffers();
}

mouse_button static
translate_button(int wx_button)
{
    switch (wx_button)
    {
     case wxMOUSE_BTN_LEFT:
        return LEFT_BUTTON;
     case wxMOUSE_BTN_MIDDLE:
        return MIDDLE_BUTTON;
     case wxMOUSE_BTN_RIGHT:
        return RIGHT_BUTTON;
     default:
        return mouse_button(-1);
    }
}

void static
handle_mouse(wx_opengl_window::impl_data& impl, wxMouseEvent& event)
{
    // Wheel events are treated specially because it seems they end up going to
    // the wrong window sometimes.  In particular, if there's an active popup,
    // other mouse events will go to the popup, but wheel events will go to the
    // parent surface.
    if (event.GetEventType() == wxEVT_MOUSEWHEEL)
    {
        impl.wheel_movement += event.GetWheelRotation();
        int lines = impl.wheel_movement / event.GetWheelDelta();
        impl.wheel_movement -= lines * event.GetWheelDelta();
        if (lines != 0)
        {
            process_mouse_wheel(impl.ui, get_time(impl), lines);
            update_window(impl);
        }
        return;
    }

    // Get the current mouse position.
    vector<2,int> position = make_vector<int>(event.GetX(), event.GetY());

    // Determine if the mouse is in the surface.
    {
        vector<2,int> client_size;
        impl.window->GetClientSize(&client_size[0], &client_size[1]);
        if (impl.window->HasCapture() || !event.Leaving() &&
            is_inside(box<2,int>(make_vector(0, 0), client_size), position))
        {
            process_mouse_move(impl.ui, get_time(impl), position);
        }
        else
            process_mouse_leave(impl.ui, get_time(impl));
    }

    if (event.ButtonDClick())
    {
        process_double_click(impl.ui, get_time(impl), position,
            translate_button(event.GetButton()));
        if (!impl.window->HasCapture())
            impl.window->CaptureMouse();
    }
    else if (event.ButtonDown())
    {
        process_mouse_press(impl.ui, get_time(impl), position,
            translate_button(event.GetButton()));
        impl.window->SetFocus();
        if (!impl.window->HasCapture())
            impl.window->CaptureMouse();
    }
    else if (event.ButtonUp())
    {
        process_mouse_release(impl.ui, get_time(impl), position,
            translate_button(event.GetButton()));
        // TODO: fix this mess
        if (!event.LeftIsDown() && !event.MiddleIsDown() &&
            !event.RightIsDown() && impl.window->HasCapture())
        {
            impl.window->ReleaseMouse();
        }
    }

    update_window(impl);
}

void static
handle_key_down(wx_opengl_window::impl_data& impl, wxKeyEvent& event)
{
    ui_time_type time = get_time(impl);
    key_event_info info = get_key_event_info(event);

    // If ALT or CTRL is pressed, assume there's no text equivalent and just
    // process it as a normal key press.
    if (event.AltDown() || event.ControlDown())
    {
        // Try processing it as a focused key press.
        bool acknowledged = process_focused_key_press(impl.ui, time, info);

        // Try processing it as a background key press.
        if (!acknowledged)
            acknowledged = process_background_key_press(impl.ui, time, info);

        if (!acknowledged)
            event.Skip();
    }
    else
    {
        impl.last_key_down = event;
        event.Skip();
    }
}

void static
handle_char(wx_opengl_window::impl_data& impl, wxKeyEvent& event)
{
    ui_time_type time = get_time(impl);
    key_event_info info = get_key_event_info(impl.last_key_down);

    if (!event.AltDown() && !event.ControlDown())
    {
        // Try processing it as a focused key press.
        bool acknowledged = process_focused_key_press(impl.ui, time, info);

        // Try processing it as text.
        if (!acknowledged)
        {
            wxChar unicode = event.GetUnicodeKey();
            if (unicode != 0 && unicode != '\t') // don't count TAB as text
            {
                wxChar buffer[2] = { unicode, 0 };
                wxString string(buffer);
                wxCharBuffer char_buffer = string.utf8_str();
                utf8_string utf8;
                utf8.begin = char_buffer.data();
                utf8.end = char_buffer.data() + char_buffer.length();
                acknowledged =
                    process_text_input(impl.ui, get_time(impl), utf8);
            }
        }

        // Try processing it as a background key press.
        if (!acknowledged)
            acknowledged = process_background_key_press(impl.ui, time, info);

        update_window(impl);
        if (!acknowledged)
            event.Skip();
    }
    else
        event.Skip();
}

void static
handle_key_up(wx_opengl_window::impl_data& impl, wxKeyEvent& event)
{
    bool acknowledged =
        process_key_release(impl.ui, get_time(impl),
            get_key_event_info(event));
    update_window(impl);
    if (!acknowledged)
        event.Skip();
}

widget_id static
resolve_wx_menu_id(menu_node const* nodes, int* id)
{
    for (menu_node const* i = nodes; i; i = i->next)
    {
        switch (i->type)
        {
         case SUBMENU_NODE:
          {
            submenu_node const* node = static_cast<submenu_node const*>(i);
            widget_id resolved = resolve_wx_menu_id(node->children, id);
            if (resolved)
                return resolved;
            --(*id);
            break;
          }
         case MENU_ITEM_NODE:
          {
            if (!(*id)--)
                return i;
            break;
          }
         case MENU_SEPARATOR_NODE:
            break;
        }
    }
    return 0;
}

widget_id static
resolve_wx_menu_bar_id(menu_container const& spec, int* id)
{
    for (menu_node const* i = spec.children; i; i = i->next)
    {
        assert(i->type == SUBMENU_NODE);
        submenu_node const* node = static_cast<submenu_node const*>(i);
        widget_id resolved = resolve_wx_menu_id(node->children, id);
        if (resolved)
            return resolved;
    }
    return 0;
}

void static
handle_menu(wx_opengl_window::impl_data& impl, wxCommandEvent& event)
{
    int id = event.GetId();
    widget_id target = resolve_wx_menu_bar_id(impl.ui.menu_bar, &id);
    if (target)
    {
        menu_item_selection_event event(target);
        issue_event(impl.ui, event);
    }
    update_window(impl);
}

wx_opengl_window::wx_opengl_window(
    alia__shared_ptr<ui_controller> const& controller,
    alia__shared_ptr<style_tree> const& alia_style,
    wxWindow* parent,
    wxWindowID id,
    int const* attrib_list,
    wxPoint const& pos,
    wxSize const& size,
    long style,
    wxString const& name,
    wxPalette const& palette)
  : wxGLCanvas(parent, id, const_cast<int*>(attrib_list), pos, size,
        style | wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE, name, palette)
{
    impl_ = new impl_data;
    impl_data& impl = *impl_;

    impl_->window = this;

    opengl_surface* surface = new opengl_surface;
    surface->set_opengl_context(impl.alia_gl_context);

    impl.wx_gl_context = new wxGLContext(this);

    impl.vsync_disabled = false;

    impl.last_menu_bar_update = 0;

    impl.wheel_movement = 0;

    wxScreenDC dc;
    wxSize ppi = dc.GetPPI();

    initialize_ui(
        impl.ui, controller,
        alia__shared_ptr<alia::surface>(surface),
        make_vector<float>(ppi.GetWidth(), ppi.GetHeight()),
        alia__shared_ptr<os_interface>(new wx_os_interface),
        alia_style);

    update();
}
wx_opengl_window::~wx_opengl_window()
{
    delete impl_->wx_gl_context;
    delete impl_;
}
void wx_opengl_window::update()
{
    update_window(*impl_);
}
void wx_opengl_window::on_paint(wxPaintEvent& event)
{
    handle_paint(*impl_);
}
void wx_opengl_window::on_erase_background(wxEraseEvent& event)
{
}
void wx_opengl_window::on_size(wxSizeEvent& event)
{
    update_window(*impl_);
}
void wx_opengl_window::on_mouse(wxMouseEvent& event)
{
    handle_mouse(*impl_, event);
}
void wx_opengl_window::on_set_focus(wxFocusEvent& event)
{
    wx_opengl_window::impl_data& impl = *impl_;
    process_focus_gain(impl.ui, get_time(impl));
    update_window(impl);
}
void wx_opengl_window::on_kill_focus(wxFocusEvent& event)
{
    wx_opengl_window::impl_data& impl = *impl_;
    process_focus_loss(impl.ui, get_time(impl));
    update_window(impl);
}
void wx_opengl_window::on_idle(wxIdleEvent& event)
{
    wx_opengl_window::impl_data& impl = *impl_;
    if (process_timer_requests(impl.ui, get_time(impl)))
        update_window(impl);
    else
        Sleep(1);
    if (has_timer_requests(impl.ui))
        event.RequestMore();
}
void wx_opengl_window::on_key_down(wxKeyEvent& event)
{
    handle_key_down(*impl_, event);
}
void wx_opengl_window::on_char(wxKeyEvent& event)
{
    handle_char(*impl_, event);
}
void wx_opengl_window::on_key_up(wxKeyEvent& event)
{
    handle_key_up(*impl_, event);
}
void wx_opengl_window::on_menu(wxCommandEvent& event)
{
    handle_menu(*impl_, event);
}
void wx_opengl_window::on_sys_color_change(wxSysColourChangedEvent& event)
{
}

alia::ui_system& wx_opengl_window::ui()
{ return impl_->ui; }

struct wx_frame::impl_data
{
    alia__shared_ptr<app_window_controller> controller;

    // position/size of the window when it's not maximized or full screen
    box<2,int> normal_rect;

    // This window is in control of the menu bar. (The app should ensure that
    // it stays alive as long as the menu bar is active.)
    wxWindow* menu_bar_controller;
};

BEGIN_EVENT_TABLE(wx_frame, wxFrame)
    EVT_MENU(-1, wx_frame::on_menu)
    EVT_SIZE(wx_frame::on_size)
    EVT_MOVE(wx_frame::on_move)
    EVT_CLOSE(wx_frame::on_close)
END_EVENT_TABLE()

wx_frame::wx_frame(
    alia__shared_ptr<app_window_controller> const& controller,
    wxWindow* parent,
    wxWindowID id,
    wxString const& title,
    wxPoint const& pos,
    wxSize const& size,
    long style,
    wxString const& name)
  : wxFrame(parent, id, title, pos, size, style, name)
{
    impl_ = new wx_frame::impl_data;
    impl_data& impl = *impl_;

    controller->window = this;
    impl.controller = controller;

    impl.normal_rect =
        make_box(
            make_vector<int>(pos.x, pos.y),
            make_vector<int>(size.GetWidth(), size.GetHeight()));
}

wx_frame::~wx_frame()
{
    delete impl_;
}

app_window_state wx_frame::state() const
{
    app_window_state state;
    state.flags = NO_FLAGS;
    if (this->IsMaximized())
        state.flags |= APP_WINDOW_MAXIMIZED;
    if (this->IsFullScreen())
        state.flags |= APP_WINDOW_FULL_SCREEN;
    state.position = impl_->normal_rect.corner;
    state.size = impl_->normal_rect.size;
    return state;
}

bool wx_frame::is_full_screen() const
{
    return this->IsFullScreen();
}

void wx_frame::set_full_screen(bool fs)
{
    this->ShowFullScreen(fs);
}

void static
build_wx_menu(wxMenu* wx_menu, menu_node const* nodes, int* next_id)
{
    for (menu_node const* i = nodes; i; i = i->next)
    {
        switch (i->type)
        {
         case SUBMENU_NODE:
          {
            submenu_node const* node = static_cast<submenu_node const*>(i);

            wxMenu* submenu = new wxMenu;
            build_wx_menu(submenu, node->children, next_id);

            int id = (*next_id)++;
            wx_menu->Append(id,
                wxString(get(node->label).c_str(), wxConvUTF8),
                submenu);
            if (!node->enabled)
                wx_menu->Enable(id, false);

            break;
          }

         case MENU_ITEM_NODE:
          {
            menu_item_node const* node = static_cast<menu_item_node const*>(i);

            int id = (*next_id)++;

            if (node->checked)
            {
                wx_menu->AppendCheckItem(id,
                    wxString(get(node->label).c_str(), wxConvUTF8));
                wx_menu->Check(id, get(node->checked));
            }
            else
            {
                wx_menu->Append(id,
                    wxString(get(node->label).c_str(), wxConvUTF8));
            }

            if (!node->enabled)
                wx_menu->Enable(id, false);

            break;
          }

         case MENU_SEPARATOR_NODE:
          {
            wx_menu->AppendSeparator();
            break;
          }
        }
    }
}

static wxMenuBar*
build_wx_menu_bar(menu_container const& spec)
{
    wxMenuBar* bar = new wxMenuBar;

    int next_id = 0;

    for (menu_node const* i = spec.children; i; i = i->next)
    {
        assert(i->type == SUBMENU_NODE);
        submenu_node const* node = static_cast<submenu_node const*>(i);
        wxMenu* wx_menu = new wxMenu;
        build_wx_menu(wx_menu, node->children, &next_id);
        bar->Append(wx_menu,
            wxString(get(node->label).c_str(), wxConvUTF8));
    }

    return bar;
}

static void
fix_wx_menu_bar(wxMenuBar* bar, menu_container const& spec)
{
    int n = 0;
    for (menu_node const* i = spec.children; i; i = i->next)
    {
        submenu_node const* node = static_cast<submenu_node const*>(i);
        if (!node->enabled)
            bar->EnableTop(n, false);
    }
}

void wx_frame::update_menu_bar(
    wxWindow* controller, menu_container const& menu_bar)
{
    this->SetMenuBar(build_wx_menu_bar(menu_bar));
    fix_wx_menu_bar(this->GetMenuBar(), menu_bar);
    impl_->menu_bar_controller = controller;
}

void wx_frame::on_menu(wxCommandEvent& event)
{
    impl_->menu_bar_controller->GetEventHandler()->ProcessEvent(event);
}

void wx_frame::close()
{
    this->Close();
}

void wx_frame::on_size(wxSizeEvent& event)
{
    if (!this->IsMaximized() && !this->IsFullScreen())
    {
        vector<2,int> size;
        this->GetSize(&size[0], &size[1]);
        impl_->normal_rect.size = size;
    }
    event.Skip();
}

void wx_frame::on_move(wxMoveEvent& event)
{
    if (!this->IsMaximized() && !this->IsFullScreen())
    {
        vector<2,int> p;
        this->GetPosition(&p[0], &p[1]);
        impl_->normal_rect.corner = p;
    }
    event.Skip();
}

void wx_frame::on_close(wxCloseEvent& event)
{
    // Send shut down events to any children that have alia UIs.
    wxList& children = this->GetChildren();
    for (wxList::Node* i = children.GetFirst(); i; i = i->GetNext())
    {
        wx_opengl_window* gl_window =
            dynamic_cast<wx_opengl_window*>(i->GetData());
        if (gl_window)
        {
            shutdown_event event;
            issue_event(gl_window->ui(), event);
        }
    }
    event.Skip();
}

wx_frame*
create_wx_framed_window(
    string const& title,
    alia__shared_ptr<app_window_controller> const& controller,
    alia__shared_ptr<style_tree> const& style,
    app_window_state const& initial_state,
    int const* gl_canvas_attribs)
{
    wx_frame* frame = new wx_frame(
        controller, 0, wxID_ANY, title.c_str(),
        initial_state.position ?
            wxPoint(
                get(initial_state.position)[0],
                get(initial_state.position)[1]) :
            wxDefaultPosition,
        wxSize(initial_state.size[0], initial_state.size[1]));

    wx_opengl_window* contents = new wx_opengl_window(
        controller, style, frame, wxID_ANY, gl_canvas_attribs,
        wxDefaultPosition,
        wxSize(initial_state.size[0], initial_state.size[1]));

    // Create a sizer, and make sure the content window fills it.
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(contents, 1, wxEXPAND, 0);
    frame->SetSizer(sizer);

    // Show the frame.
    if (initial_state.flags & APP_WINDOW_FULL_SCREEN)
    {
        // This is the only sequence of commands I've found that creates a
        // full screen window without flickering and without causing a weird
        // blank window when the user switches back to windowed mode.
        frame->Freeze();
        frame->Show(true);
        frame->ShowFullScreen(true);
        frame->Thaw();
    }
    else
        frame->Show(true);

    return frame;
}

}
