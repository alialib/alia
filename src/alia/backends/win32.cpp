#include <alia/backends/win32.hpp>
#include <alia/ui_system.hpp>
#include <alia/backends/opengl.hpp>
#include <alia/millisecond_clock.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <gl\gl.h>
#include "wglext.h"

#include <alia/lua_styling.hpp>

namespace alia {

struct timer_request
{
    ui_time_type trigger_time;
    routable_widget_id id;
    counter_type frame_issued;
};
typedef std::vector<timer_request> timer_request_list;

struct native_window::impl_data
{
    ui_system ui;

    opengl_context gl_ctx;

    // win32 resource handles
    HINSTANCE hinstance;
    HWND hwnd;
    HDC dc;
    HGLRC rc;

    // If this is a popup window, then this is the window's parent.
    // Otherwise, it's 0.
    impl_data* parent;

    // If this window has a child popup, then this is it.
    impl_data* popup;

    bool is_full_screen;
    // If the window is full screen, this stores the normal placement of it.
    WINDOWPLACEMENT normal_placement;

    millisecond_clock clock;

    timer_request_list timer_requests;
    counter_type timer_event_counter;

    bool update_needed;

    bool close;

    impl_data()
      : dc(0), rc(0), hwnd(0), hinstance(0), parent(0), popup(0),
        is_full_screen(false), timer_event_counter(0), update_needed(false),
        close(false)
    {}
};

static vector<2,unsigned> get_display_size(HDC dc)
{
    return make_vector<unsigned>(
        GetDeviceCaps(dc, HORZRES),
        GetDeviceCaps(dc, VERTRES));
}

struct win32_opengl_surface : opengl_surface
{
    win32_opengl_surface(native_window::impl_data* impl)
      : impl_(impl), ppi_known_(false)
    {}
    vector<2,float> ppi() const
    {
        if (!ppi_known_)
        {
            HDC hdc = GetDC(NULL);
            if (hdc)
            {
                ppi_[0] = float(GetDeviceCaps(hdc, LOGPIXELSX));
                ppi_[1] = float(GetDeviceCaps(hdc, LOGPIXELSY));
                ReleaseDC(NULL, hdc);
            }
            else
            {
                ppi_[0] = 96;
                ppi_[1] = 96;
            }
            ppi_known_ = true;
        }
        return ppi_;
    }
    vector<2,unsigned> display_size() const
    {
        return get_display_size(impl_->dc);
    }
    //popup_interface* open_popup(
    //    ui_controller* controller,
    //    vector<2,int> const& primary_position,
    //    vector<2,int> const& boundary,
    //    vector<2,int> const& minimum_size);
    //void close_popups();
    void request_refresh();
    void request_timer_event(routable_widget_id const& id, unsigned ms);
    string get_clipboard_text();
    void set_clipboard_text(string const& text);
 private:
    native_window::impl_data* impl_;
    mutable bool ppi_known_;
    mutable vector<2,float> ppi_;
};

static void set_cursor(mouse_cursor cursor)
{
    HCURSOR hcursor;
    switch (cursor)
    {
     case DEFAULT_CURSOR:
     default:
        hcursor = LoadCursor(0, IDC_ARROW);
        break;
     case CROSS_CURSOR:
        hcursor = LoadCursor(0, IDC_CROSS);
        break;
     case BUSY_CURSOR:
        hcursor = LoadCursor(0, IDC_WAIT);
        break;
     case BLANK_CURSOR:
        hcursor = 0;
        break;
     case IBEAM_CURSOR:
        hcursor = LoadCursor(0, IDC_IBEAM);
        break;
     case NO_ENTRY_CURSOR:
        hcursor = LoadCursor(0, IDC_NO);
        break;
     case HAND_CURSOR:
        hcursor = LoadCursor(0, IDC_HAND);
        break;
     case LEFT_RIGHT_ARROW_CURSOR:
        hcursor = LoadCursor(0, IDC_SIZEWE);
        break;
     case UP_DOWN_ARROW_CURSOR:
        hcursor = LoadCursor(0, IDC_SIZENS);
        break;
     case FOUR_WAY_ARROW_CURSOR:
        hcursor = LoadCursor(0, IDC_SIZEALL);
        break;
    }
    SetCursor(hcursor);
}

static bool is_wgl_extension_supported(char const* extension_name)
{
    PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT =
        (PFNWGLGETEXTENSIONSSTRINGEXTPROC)
        wglGetProcAddress("wglGetExtensionsStringEXT");

    if (_wglGetExtensionsStringEXT)
    {
        return alia::is_opengl_extension_in_list(
            _wglGetExtensionsStringEXT(), extension_name);
    }
    else
        return false;
}

static void disable_vsync()										// All Setup For OpenGL Goes Here
{
    if (is_wgl_extension_supported("WGL_EXT_swap_control"))
    {
        PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT =
            (PFNWGLSWAPINTERVALEXTPROC)
            wglGetProcAddress("wglSwapIntervalEXT");
        wglSwapIntervalEXT(0);
    }
}

static void paint_window(native_window::impl_data& impl)
{
    if (!wglMakeCurrent(impl.dc, impl.rc))
        return;

    opengl_surface* surface =
        static_cast<opengl_surface*>(impl.ui.surface.get());

    RECT rect;
    GetClientRect(impl.hwnd, &rect);
    surface->set_size(
        alia::make_vector<unsigned>(rect.right, rect.bottom));

    surface->initialize_render_state();

    render_ui(impl.ui);
}

static key_code translate_key_code(WPARAM code)
{
    // numbers
    if (code >= 0x30 && code <= 0x39)
    {
        return key_code(code);
    }
    // letters
    if (code >= 0x41 && code <= 0x5a)
    {
        return key_code(code + 0x20);
    }

    switch (code)
    {
     case VK_BACK:
        return KEY_BACKSPACE;
     case VK_TAB:
        return KEY_TAB;
     case VK_CLEAR:
        return KEY_CLEAR;
     case VK_OEM_PLUS:
        return KEY_PLUS;
     case VK_OEM_MINUS:
        return KEY_MINUS;
     case VK_RETURN:
        return KEY_ENTER;
     case VK_PAUSE:
        return KEY_PAUSE;
     case VK_ESCAPE:
        return KEY_ESCAPE;
     case VK_SPACE:
        return KEY_SPACE;
     case VK_PRIOR:
        return KEY_PAGEUP;
     case VK_NEXT:
        return KEY_PAGEDOWN;
     case VK_END:
        return KEY_END;
     case VK_HOME:
        return KEY_HOME;
     case VK_UP:
        return KEY_UP;
     case VK_DOWN:
        return KEY_DOWN;
     case VK_LEFT:
        return KEY_LEFT;
     case VK_RIGHT:
        return KEY_RIGHT;
     case VK_PRINT:
        return KEY_PRINT;
     case VK_SNAPSHOT:
        return KEY_PRINT_SCREEN;
     case VK_INSERT:
        return KEY_INSERT;
     case VK_DELETE:
        return KEY_DELETE;
     case VK_HELP:
        return KEY_HELP;
     case VK_NUMPAD0:
        return KEY_NUMPAD_0;
     case VK_NUMPAD1:
        return KEY_NUMPAD_1;
     case VK_NUMPAD2:
        return KEY_NUMPAD_2;
     case VK_NUMPAD3:
        return KEY_NUMPAD_3;
     case VK_NUMPAD4:
        return KEY_NUMPAD_4;
     case VK_NUMPAD5:
        return KEY_NUMPAD_5;
     case VK_NUMPAD6:
        return KEY_NUMPAD_6;
     case VK_NUMPAD7:
        return KEY_NUMPAD_7;
     case VK_NUMPAD8:
        return KEY_NUMPAD_8;
     case VK_NUMPAD9:
        return KEY_NUMPAD_9;
     case VK_MULTIPLY:
        return KEY_NUMPAD_MULTIPLY;
     case VK_ADD:
        return KEY_NUMPAD_ADD;
     case VK_SUBTRACT:
        return KEY_NUMPAD_SUBTRACT;
     case VK_DECIMAL:
        return KEY_NUMPAD_PERIOD;
     case VK_DIVIDE:
        return KEY_NUMPAD_DIVIDE;
     case VK_F1:
        return KEY_F1;
     case VK_F2:
        return KEY_F2;
     case VK_F3:
        return KEY_F3;
     case VK_F4:
        return KEY_F4;
     case VK_F5:
        return KEY_F5;
     case VK_F6:
        return KEY_F6;
     case VK_F7:
        return KEY_F7;
     case VK_F8:
        return KEY_F8;
     case VK_F9:
        return KEY_F9;
     case VK_F10:
        return KEY_F10;
     case VK_F11:
        return KEY_F11;
     case VK_F12:
        return KEY_F12;
     case VK_F13:
        return KEY_F13;
     case VK_F14:
        return KEY_F14;
     case VK_F15:
        return KEY_F15;
     case VK_F16:
        return KEY_F16;
     case VK_F17:
        return KEY_F17;
     case VK_F18:
        return KEY_F18;
     case VK_F19:
        return KEY_F19;
     case VK_F20:
        return KEY_F20;
     case VK_F21:
        return KEY_F21;
     case VK_F22:
        return KEY_F22;
     case VK_F23:
        return KEY_F23;
     case VK_F24:
        return KEY_F24;
    }

    return KEY_UNKNOWN;
}

static key_event_info get_key_event_info(WPARAM wparam)
{
    key_modifiers mods = KMOD_NONE;
    if ((GetKeyState(VK_SHIFT) & 0x8000) != 0)
        mods |= KMOD_SHIFT;
    if ((GetKeyState(VK_LCONTROL) & 0x8000) != 0 ||
        (GetKeyState(VK_RCONTROL) & 0x8000) != 0)
    {
        mods |= KMOD_CTRL;
    }
    if ((GetKeyState(VK_MENU) & 0x8000) != 0)
        mods |= KMOD_ALT;
    if ((GetKeyState(VK_LWIN) & 0x8000) != 0 ||
        (GetKeyState(VK_RWIN) & 0x8000) != 0)
    {
        mods |= KMOD_WIN;
    }
    return key_event_info(translate_key_code(wparam), mods);
}

static native_window::impl_data& get_window_data(HWND hwnd)
{
    return *(native_window::impl_data*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

static inline ui_time_type get_time(native_window::impl_data& impl)
{
    return impl.clock.get_tick_count();
}

static void close_popup(native_window::impl_data& impl);

static void destroy_window(native_window::impl_data& impl)
{
    close_popup(impl);
    // If this is a popup, clear parent's pointer to this popup.
    if (impl.parent)
    {
        impl.parent->popup = 0;
        impl.parent = 0;
    }

    // Delete windows resources.
    if (impl.rc)
    {
        wglMakeCurrent(NULL,NULL);
        wglDeleteContext(impl.rc);
        impl.rc = 0;
    }
    if (impl.dc)
    {
        ReleaseDC(impl.hwnd, impl.dc);
        impl.dc = 0;
    }
    if (impl.hwnd)
    {
        DestroyWindow(impl.hwnd);
        impl.hwnd = 0;
    }
}

static void close_popup(native_window::impl_data& impl)
{
    if (impl.popup)
    {
        assert(impl.popup->parent == &impl);
        destroy_window(*impl.popup);
        // Child should have cleared this.
        assert(!impl.popup);
    }
}

static void update_window(HWND hwnd)
{
    native_window::impl_data& impl = get_window_data(hwnd);

    impl.update_needed = false;

    if (impl.close)
    {
        destroy_window(impl);
        return;
    }

    RECT rect;
    GetClientRect(impl.hwnd, &rect);

    // Don't update if the window has zero size.
    // (This seems to happen if the window is minimized.)
    if (rect.bottom == rect.top || rect.left == rect.right)
        return;

    refresh_and_layout(impl.ui,
        alia::make_vector<unsigned>(rect.right, rect.bottom),
        get_time(impl));

    opengl_surface* surface =
        static_cast<opengl_surface*>(impl.ui.surface.get());
    surface->set_size(
        alia::make_vector<unsigned>(rect.right, rect.bottom));

    optional<mouse_cursor> cursor = update_mouse_cursor(impl.ui);
    if (cursor)
        set_cursor(get(cursor));

    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

    if (impl.popup)
        update_window(impl.popup->hwnd);
}

static void reset_mouse_tracking(HWND hwnd)
{
    TRACKMOUSEEVENT e;
    e.cbSize = sizeof(TRACKMOUSEEVENT);
    e.dwFlags = TME_LEAVE | TME_HOVER;
    e.hwndTrack = hwnd;
    e.dwHoverTime = HOVER_DEFAULT;
    TrackMouseEvent(&e);
}

static bool any_mouse_buttons_pressed()
{
    return
        (GetKeyState(VK_LBUTTON) & 0x8000) != 0 ||
        (GetKeyState(VK_RBUTTON) & 0x8000) != 0 ||
        (GetKeyState(VK_MBUTTON) & 0x8000) != 0;
}

static bool is_popup(native_window::impl_data& impl)
{
    return impl.parent != 0;
}

static void on_mouse_button_press(HWND hwnd)
{
    SetCapture(hwnd);
    native_window::impl_data& impl = get_window_data(hwnd);
    close_popup(impl);
}

static void on_mouse_button_release(HWND hwnd)
{
    if (!any_mouse_buttons_pressed())
        ReleaseCapture();
}

static void process_timer_requests(native_window::impl_data& impl)
{
    ++impl.timer_event_counter;
    if (!impl.timer_requests.empty())
    {
        bool processed_any = false;
        while (1)
        {
            unsigned now = impl.clock.get_tick_count();
            // Ideally, the list would be stored sorted, but it has to be
            // sorted relative to the current tick count (to handle wrapping),
            // and the list is generally not very long anyway.
            timer_request_list::iterator next_event =
                impl.timer_requests.end();
            for (timer_request_list::iterator
                i = impl.timer_requests.begin();
                i != impl.timer_requests.end(); ++i)
            {
                if (i->frame_issued != impl.timer_event_counter &&
                    int(now - i->trigger_time) >= 0 &&
                    (next_event == impl.timer_requests.end() ||
                    int(next_event->trigger_time - i->trigger_time) >= 0))
                {
                    next_event = i;
                }
            }
            if (next_event == impl.timer_requests.end())
                break;

            processed_any = true;

            timer_request request = *next_event;
            impl.timer_requests.erase(next_event);

            timer_event e(request.id.id, request.trigger_time, now);
            issue_targeted_event(impl.ui, e, request.id);

            update_window(impl.hwnd);
        }
    }
}

static inline ui_time_type get_time(HWND hwnd)
{
    return get_time(get_window_data(hwnd));
}

LRESULT CALLBACK wndproc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam)
{
    switch (msg)
    {
     case WM_NCCREATE:
      {
	LPCREATESTRUCT cs = (LPCREATESTRUCT) lparam;
	SetWindowLongPtr(hwnd, GWLP_USERDATA, __int3264(cs->lpCreateParams));
	break;
      }

     case WM_MOUSEACTIVATE:
        return is_popup(get_window_data(hwnd)) ? MA_NOACTIVATE : MA_ACTIVATE;

     case WM_ACTIVATE:
      {
        if (HIWORD(wparam))
        {
            // Activation.
        }
        else
        {
            // Deactivation.
            close_popup(get_window_data(hwnd));
        }
        break;
      }

     case WM_SETFOCUS:
      {
        process_focus_gain(get_window_data(hwnd).ui, get_time(hwnd));
        update_window(hwnd);
        return 0;
      }

     case WM_KILLFOCUS:
      {
        process_focus_loss(get_window_data(hwnd).ui, get_time(hwnd));
        update_window(hwnd);
        return 0;
      }

     //case WM_TIMER:
     //   process_timer_requests(get_window_data(hwnd));
     //   SetTimer(hwnd, 0, 1, 0);
     //   return 0;

     case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

     case WM_CHAR:
      {
        native_window::impl_data& impl = get_window_data(hwnd);
        // TODO: Actual UTF8 string.
        if (wparam < 0x80)
        {
            char character = char(wparam);
            utf8_string utf8;
            utf8.begin = &character;
            utf8.end = utf8.begin + 1;
            bool acknowledged = false;
            if (impl.popup)
            {
                acknowledged = process_text_input(
                    impl.popup->ui, get_time(impl), utf8);
            }
            if (!acknowledged)
            {
                acknowledged = process_text_input(
                    impl.ui, get_time(impl), utf8);
            }
        }
        update_window(hwnd);
        break;
      }
 
    case WM_KEYDOWN:
      {
        native_window::impl_data& impl = get_window_data(hwnd);
        bool acknowledged = false;
        if (impl.popup)
        {
            acknowledged =
                process_key_press(impl.popup->ui, get_time(impl),
                    get_key_event_info(wparam));
        }
        if (!acknowledged)
        {
            acknowledged =
                process_key_press(impl.ui, get_time(impl),
                    get_key_event_info(wparam));
        }
        if (!acknowledged)
        {
            switch (wparam)
            {
             case VK_TAB:
                if ((GetKeyState(VK_CONTROL) & 0x8000) == 0 &&
                    (GetKeyState(VK_MENU) & 0x8000) == 0 &&
                    (GetKeyState(VK_LWIN) & 0x8000) == 0 &&
                    (GetKeyState(VK_RWIN) & 0x8000) == 0)
                {
                    if ((GetKeyState(VK_SHIFT) & 0x8000) != 0)
                        regress_focus(impl.ui);
                    else
                        advance_focus(impl.ui);
                }
                break;
            }
            update_window(hwnd);
        }
        else
        {
            update_window(hwnd);
            return 0;
        }
        break;
      }
    case WM_KEYUP:
      {
        native_window::impl_data& impl = get_window_data(hwnd);
        bool acknowledged = false;
        if (impl.popup)
        {
            acknowledged = process_key_release(
                impl.popup->ui, get_time(impl), get_key_event_info(wparam));
        }
        if (!acknowledged)
        {
            acknowledged =
                process_key_release(impl.ui, get_time(impl),
                    get_key_event_info(wparam));
        }
        update_window(hwnd);
        if (acknowledged)
            return 0;
        break;
      }

     case WM_MOUSELEAVE:
      {
        process_mouse_leave(get_window_data(hwnd).ui, get_time(hwnd));
        update_window(hwnd);
        return 0;
      }
     case WM_MOUSEMOVE:
      {
        process_mouse_move(get_window_data(hwnd).ui, get_time(hwnd),
            make_vector<int>(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)));
        reset_mouse_tracking(hwnd);
        update_window(hwnd);
        return 0;
      }

     case WM_MOUSEWHEEL:
      {
        native_window::impl_data& impl = get_window_data(hwnd);
        float movement = float(GET_WHEEL_DELTA_WPARAM(wparam)) / WHEEL_DELTA;
        if (impl.popup)
            process_mouse_wheel(impl.popup->ui, get_time(hwnd), movement);
        else
            process_mouse_wheel(impl.ui, get_time(hwnd), movement);
        update_window(hwnd);
        return 0;
      }

     case WM_LBUTTONDOWN:
        process_mouse_press(get_window_data(hwnd).ui, get_time(hwnd),
            make_vector<int>(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)),
            LEFT_BUTTON);
        on_mouse_button_press(hwnd);
        update_window(hwnd);
        return 0;
     case WM_LBUTTONUP:
        process_mouse_release(get_window_data(hwnd).ui, get_time(hwnd),
            make_vector<int>(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)),
            LEFT_BUTTON);
        on_mouse_button_release(hwnd);
        update_window(hwnd);
        return 0;
     case WM_LBUTTONDBLCLK:
        process_double_click(get_window_data(hwnd).ui, get_time(hwnd),
            make_vector<int>(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)),
            LEFT_BUTTON);
        on_mouse_button_press(hwnd);
        update_window(hwnd);
        return 0;

     case WM_RBUTTONDOWN:
        process_mouse_press(get_window_data(hwnd).ui, get_time(hwnd),
            make_vector<int>(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)),
            RIGHT_BUTTON);
        on_mouse_button_press(hwnd);
        update_window(hwnd);
        return 0;
     case WM_RBUTTONUP:
        process_mouse_release(get_window_data(hwnd).ui, get_time(hwnd),
            make_vector<int>(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)),
            RIGHT_BUTTON);
        on_mouse_button_release(hwnd);
        update_window(hwnd);
        return 0;
     case WM_RBUTTONDBLCLK:
        process_double_click(get_window_data(hwnd).ui, get_time(hwnd),
            make_vector<int>(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)),
            RIGHT_BUTTON);
        on_mouse_button_press(hwnd);
        update_window(hwnd);
        return 0;

     case WM_MBUTTONDOWN:
        process_mouse_press(get_window_data(hwnd).ui, get_time(hwnd),
            make_vector<int>(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)),
            MIDDLE_BUTTON);
        on_mouse_button_press(hwnd);
        update_window(hwnd);
        return 0;
     case WM_MBUTTONUP:
        process_mouse_release(get_window_data(hwnd).ui, get_time(hwnd),
            make_vector<int>(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)),
            MIDDLE_BUTTON);
        on_mouse_button_release(hwnd);
        update_window(hwnd);
        return 0;
     case WM_MBUTTONDBLCLK:
        process_double_click(get_window_data(hwnd).ui, get_time(hwnd),
            make_vector<int>(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)),
            MIDDLE_BUTTON);
        on_mouse_button_press(hwnd);
        update_window(hwnd);
        return 0;

     case WM_ERASEBKGND:
        return 1;

     case WM_WINDOWPOSCHANGED:
      {
        close_popup(get_window_data(hwnd));
        update_window(hwnd);
        return 0;
      }

     case WM_PAINT:
      {
        native_window::impl_data& impl = get_window_data(hwnd);
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        paint_window(impl);
        SwapBuffers(impl.dc);
        EndPaint(hwnd, &ps);
        return 0;
      }
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static void throw_win32_error(string const& prefix)
{
    DWORD error_code = GetLastError();
    if (error_code != 0)
    {
        LPTSTR error_message = NULL;
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_IGNORE_INSERTS,  
            NULL,
            error_code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&error_message,
            0,
            NULL);

        if (error_message)
        {
            string message = prefix + "\n" + error_message;
            LocalFree(error_message);
            throw native_error(message);
        }
    }
    throw native_error(prefix);
}

static void throw_window_creation_error(string const& fn_name)
{
    throw_win32_error("unable to create window: " + fn_name + " failed");
}

static void enter_full_screen(native_window::impl_data& impl)
{
    MONITORINFO mi = { sizeof(MONITORINFO) };
    impl.normal_placement.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(impl.hwnd, &impl.normal_placement) &&
        GetMonitorInfo(MonitorFromWindow(impl.hwnd,
                       MONITOR_DEFAULTTOPRIMARY), &mi))
    {
        DWORD style = GetWindowLong(impl.hwnd, GWL_STYLE);
        SetWindowLong(impl.hwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(impl.hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        impl.is_full_screen = true;
    }
}

static void exit_full_screen(native_window::impl_data& impl)
{
    DWORD style = GetWindowLong(impl.hwnd, GWL_STYLE);
    SetWindowLong(impl.hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(impl.hwnd, &impl.normal_placement);
    SetWindowPos(impl.hwnd, NULL, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
        SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    impl.is_full_screen = false;
}

static void create_window(
    native_window::impl_data* impl,
    native_window::impl_data* parent,
    string const& title,
    alia__shared_ptr<ui_controller> const& controller,
    native_window::state_data const& initial_state)
{
    impl->hinstance = GetModuleHandle(NULL);

    impl->parent = parent;
    bool is_popup = parent != 0;
    if (parent)
    {
        assert(!parent->popup);
        parent->popup = impl;
    }

    static bool already_registered = false;
    if (!already_registered)
    {
        WNDCLASS wc;
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
        wc.lpfnWndProc = (WNDPROC) wndproc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = sizeof(native_window::impl_data*);
        wc.hInstance = impl->hinstance;
        wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
        wc.hCursor = 0;
        wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = "alia_gl";
        if (!RegisterClass(&wc))
            throw_window_creation_error("RegisterClass");
        already_registered = true;
    }

    if (is_popup)
    {
        impl->hwnd =
            CreateWindowEx(
                WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
	        "alia_gl",
	        title.c_str(),
	        WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
	        initial_state.position ?
	            get(initial_state.position)[0] : CW_USEDEFAULT,
	        initial_state.position ?
	            get(initial_state.position)[1] : CW_USEDEFAULT,
	        initial_state.size[0], initial_state.size[1],
	        parent->hwnd,
	        NULL,
	        impl->hinstance,
	        impl);
    }
    else
    {
        impl->hwnd =
            CreateWindowEx(
                WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
	        "alia_gl",
	        title.c_str(),
	        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
	        initial_state.position ?
	            get(initial_state.position)[0] : CW_USEDEFAULT,
	        initial_state.position ?
	            get(initial_state.position)[1] : CW_USEDEFAULT,
	        initial_state.size[0], initial_state.size[1],
	        NULL,
	        NULL,
	        impl->hinstance,
	        impl);
    }
    if (!impl->hwnd)
        throw_window_creation_error("CreateWindowEx");

    impl->dc = GetDC(impl->hwnd);
    if (!impl->dc)
        throw_window_creation_error("GetDC");

    static PIXELFORMATDESCRIPTOR pfd =
     {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW |
        PFD_SUPPORT_OPENGL |
        PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,
        0, 0, 0, 0, 0, 0, 0, 0,
        0,
        0, 0, 0, 0,
        8,
        0,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
     };

    int pixel_format = ChoosePixelFormat(impl->dc, &pfd);
    if (!pixel_format)
        throw_window_creation_error("ChoosePixelFormat");

    if(!SetPixelFormat(impl->dc, pixel_format, &pfd))
        throw_window_creation_error("SetPixelFormat");

    impl->rc = wglCreateContext(impl->dc);
    if (!impl->rc)
        throw_window_creation_error("wglCreateContext");

    if (!wglMakeCurrent(impl->dc, impl->rc))
        throw_window_creation_error("wglMakeCurrent");

    disable_vsync();

    win32_opengl_surface* surface = new alia::win32_opengl_surface(impl);
    surface->set_opengl_context(impl->gl_ctx);

    impl->ui.controller = controller;
    impl->ui.surface.reset(surface);

    if (!is_popup)
    {
        if (initial_state.flags & FULL_SCREEN)
        {
            enter_full_screen(*impl);
            ShowWindow(impl->hwnd, SW_SHOW);
        }
        else if (initial_state.flags & MAXIMIZED)
            ShowWindow(impl->hwnd, SW_MAXIMIZE);
        else
            ShowWindow(impl->hwnd, SW_SHOWNORMAL);
        //SetForegroundWindow(impl->hwnd);
        //SetFocus(impl->hwnd);
    }
    else
        ShowWindow(impl->hwnd, SW_SHOWNA);

    reset_mouse_tracking(impl->hwnd);

    RECT rect;
    GetClientRect(impl->hwnd, &rect);

    refresh_and_layout(impl->ui,
        alia::make_vector<unsigned>(rect.right, rect.bottom),
        get_time(*impl));

    ui_event initial_visibility(NO_CATEGORY, INITIAL_VISIBILITY_EVENT);
    issue_event(impl->ui, initial_visibility);

    // Start the flow of WM_TIMER messages.
    //SetTimer(impl->hwnd, 0, 0, 0);
}

void native_window::initialize(
    string const& title, window_controller* controller,
    state_data const& initial_state)
{
    controller->window = this;
    impl_ = new impl_data;
    impl_->ui.style.reset(new ui_style);
    read_lua_style_file(&impl_->ui.style->styles, "style.lua");
    create_window(impl_, 0, title, alia__shared_ptr<ui_controller>(controller),
        initial_state);
}

native_window::~native_window()
{
    if (impl_)
    {
        destroy_window(*impl_);
        delete impl_;
    }
}

alia::ui_system& native_window::ui()
{ return impl_->ui; }

native_window::state_data native_window::state() const
{
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(impl_->hwnd, &wp))
    {
        native_window::state_data state;
        state.flags = NO_FLAGS;
        if ((wp.showCmd & SW_MAXIMIZE) != 0)
            state.flags |= MAXIMIZED;
        if (impl_->is_full_screen)
            state.flags |= FULL_SCREEN;
        state.position = make_vector<int>(
            wp.rcNormalPosition.left, wp.rcNormalPosition.top);
        state.size = make_vector<int>(
            wp.rcNormalPosition.right - wp.rcNormalPosition.left,
            wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
        return state;
    }
    else
        return native_window::state_data();
}

bool native_window::is_full_screen() const
{
    return impl_->is_full_screen;
}

void native_window::set_full_screen(bool fs)
{
    if (fs && !impl_->is_full_screen)
    {
        enter_full_screen(*impl_);
    }
    else if (!fs && impl_->is_full_screen)
    {
        exit_full_screen(*impl_);
    }
}

bool native_window::has_idle_work()
{
    return impl_->update_needed || !impl_->timer_requests.empty();
}

void native_window::do_idle_work()
{
    process_timer_requests(*impl_);
    if (impl_->update_needed)
        update_window(impl_->hwnd);
    else
        Sleep(1);
}

void native_window::do_message_loop()
{
    while(1)
    {
        if (this->has_idle_work())
        {
            MSG msg;
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                    break;
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            this->do_idle_work();
        }
        else
        {
            MSG msg;
            if (!GetMessage(&msg, NULL, 0, 0))
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

//struct popup_window : popup_interface
//{
//    popup_window(
//        native_window::impl_data* parent,
//        ui_controller* controller,
//        vector<2,int> const& primary_position,
//        vector<2,int> const& boundary,
//        vector<2,int> const& minimum_size);
//    ~popup_window();
//
//    alia::ui_system& ui() { return impl_->ui; }
//
//    bool is_open() const
//    {
//        return impl_->hwnd != 0;
//    }
//    void close()
//    {
//        impl_->close = true;
//    }
//
// private:
//    native_window::impl_data* impl_;
//};
//
//static vector<2,int> client_to_screen(HWND hwnd, vector<2,int> const& p)
//{
//    POINT q;
//    q.x = p[0];
//    q.y = p[1];
//    ClientToScreen(hwnd, &q);
//    return make_vector<int>(q.x, q.y);
//}
//
//static vector<2,int> screen_to_client(HWND hwnd, vector<2,int> const& p)
//{
//    POINT q;
//    q.x = p[0];
//    q.y = p[1];
//    ScreenToClient(hwnd, &q);
//    return make_vector<int>(q.x, q.y);
//}
//
//popup_window::popup_window(
//    native_window::impl_data* parent,
//    ui_controller* controller,
//    vector<2,int> const& primary_position,
//    vector<2,int> const& boundary,
//    vector<2,int> const& minimum_size)
//{
//    impl_ = new native_window::impl_data;
//
//    impl_->ui.style = parent->ui.style;
//
//    alia__shared_ptr<ui_controller> controller_ptr(controller);
//
//    layout_vector size =
//        measure_initial_ui(controller_ptr, parent->ui.style,
//            parent->ui.surface);
//    for (unsigned i = 0; i != 2; ++i)
//    {
//        if (size[i] < minimum_size[i])
//            size[i] = minimum_size[i];
//    }
//
//    vector<2,int> lower_display_boundary = screen_to_client(parent->hwnd,
//        make_vector<int>(0, 0));
//    vector<2,int> upper_display_boundary = screen_to_client(parent->hwnd,
//        vector<2,int>(get_display_size(parent->dc)));
//
//    vector<2,int> position, actual_size;
//    for (unsigned i = 0; i != 2; ++i)
//    {
//        if (primary_position[i] + size[i] <= upper_display_boundary[i] ||
//            boundary[i] - lower_display_boundary[i] <
//            upper_display_boundary[i] - primary_position[i])
//        {
//            position[i] = primary_position[i];
//            actual_size[i] = (std::min)(size[i],
//                upper_display_boundary[i] - primary_position[i]);
//        }
//        else
//        {
//            actual_size[i] = (std::min)(size[i],
//                boundary[i] - lower_display_boundary[i]);
//            position[i] = boundary[i] - actual_size[i];
//        }
//    }
//
//    create_window(impl_, parent, "popup", controller_ptr,
//        native_window::state_data(
//            client_to_screen(parent->hwnd, position),
//            actual_size));
//}
//popup_window::~popup_window()
//{
//    destroy_window(*impl_);
//}
//
//popup_interface*
//win32_opengl_surface::open_popup(
//    ui_controller* controller,
//    vector<2,int> const& primary_position,
//    vector<2,int> const& boundary,
//    vector<2,int> const& minimum_size)
//{
//    return new popup_window(
//        impl_, controller, primary_position, boundary, minimum_size);
//}
//
//void win32_opengl_surface::close_popups()
//{
//    close_popup(*impl_);
//}

void win32_opengl_surface::request_refresh()
{
    impl_->update_needed = true;
}

void win32_opengl_surface::request_timer_event(
    routable_widget_id const& id, ui_time_type trigger_time)
{
    // If an event already exists for that ID, then reschedule it.
    for (timer_request_list::iterator i = impl_->timer_requests.begin();
        i != impl_->timer_requests.end(); ++i)
    {
        if (i->id.id == id.id)
        {
            i->id = id;
            i->trigger_time = trigger_time;
            i->frame_issued = impl_->timer_event_counter;
            return;
        }
    }
    // Otherwise, add a new event.
    timer_request rq;
    rq.id = id;
    rq.trigger_time = trigger_time;
    rq.frame_issued = impl_->timer_event_counter;
    impl_->timer_requests.push_back(rq);
}

string win32_opengl_surface::get_clipboard_text()
{
    if (OpenClipboard(0))
    {
        HANDLE clip = GetClipboardData(CF_TEXT); 
        char const* text = (char const*) GlobalLock(clip);
        string result = text ? text : "";
        GlobalUnlock(clip);
        CloseClipboard();
        return result;
    }
    else
        return "";
}

void win32_opengl_surface::set_clipboard_text(string const& text)
{
    size_t const required_length = text.length() + 1;
    HGLOBAL mem =  GlobalAlloc(GMEM_MOVEABLE, required_length);
    if (mem)
    {
        memcpy(GlobalLock(mem), text.c_str(), required_length);
        GlobalUnlock(mem);
        if (OpenClipboard(0))
        {
            EmptyClipboard();
            SetClipboardData(CF_TEXT, mem);
            CloseClipboard();
        }
    }
}

void show_error_message(string const& caption, string const& message)
{
    MessageBox(0, message.c_str(), caption.c_str(),
        MB_OK | MB_ICONEXCLAMATION);
}

}
