#include <alia/ui/backends/win32.hpp>
#include <alia/ui/system.hpp>
#include <alia/ui/backends/opengl.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <gl\gl.h>
#include "wglext.h"

#include <alia/ui/utilities/styling.hpp>

alia::style_tree alia_style()
{
    alia::style_tree node_0;
    node_0.properties["default-padding"] = "4px";
    node_0.properties["disable-padding"] = "true";
    node_0.properties["focus-color"] = "#d8bd67";
    node_0.properties["font-famliy"] = "arial";
    node_0.properties["font-size"] = "13";
    node_0.properties["selected-background"] = "#3297fd";
    node_0.properties["selected-color"] = "#ffffff";
    node_0.properties["separator-color"] = "#666666";
    alia::style_tree node_1;
    node_1.properties["background"] = "#e4e4e4";
    node_1.properties["color"] = "#101010";
    node_0.substyles["accordion-header"] = node_1;
    alia::style_tree node_2;
    node_2.properties["background"] = "#c4c4c4";
    node_2.properties["color"] = "#101010";
    node_0.substyles["accordion-header.depressed"] = node_2;
    alia::style_tree node_3;
    node_3.properties["background"] = "#d4d4d4";
    node_3.properties["color"] = "#101010";
    node_0.substyles["accordion-header.hot"] = node_3;
    alia::style_tree node_4;
    node_4.properties["border-radius"] = "15px 45px 15px 45px";
    node_4.properties["size"] = "65px 65px";
    node_0.substyles["acheck-box"] = node_4;
    alia::style_tree node_5;
    node_5.properties["background"] = "#d4d4d4";
    node_5.properties["color"] = "#101010";
    node_0.substyles["astroid"] = node_5;
    alia::style_tree node_6;
    node_6.properties["background"] = "#fff";
    node_6.properties["color"] = "#2c2c2c";
    node_0.substyles["background"] = node_6;
    alia::style_tree node_7;
    node_7.properties["background"] = "#0060cc";
    node_7.properties["border-radius"] = "4px";
    node_7.properties["color"] = "#fff";
    node_0.substyles["button"] = node_7;
    alia::style_tree node_8;
    node_8.properties["background"] = "#fff";
    node_8.properties["color"] = "#2c2c2c";
    node_8.properties["padding"] = "8px 8px 8px 0px";
    node_0.substyles["content"] = node_8;
    alia::style_tree node_9;
    node_9.properties["background"] = "#e4e4e4";
    node_9.properties["border-color"] = "#c4c4c4";
    node_9.properties["color"] = "#101010";
    node_9.properties["size"] = "16px 16px";
    node_0.substyles["control"] = node_9;
    alia::style_tree node_10;
    node_10.properties["background"] = "#c4c4c4";
    node_10.properties["color"] = "#101010";
    node_0.substyles["control.depressed"] = node_10;
    alia::style_tree node_11;
    node_11.properties["background"] = "#d4d4d4";
    node_11.properties["color"] = "#101010";
    node_0.substyles["control.hot"] = node_11;
    alia::style_tree node_12;
    node_12.properties["background"] = "#f7f7f9";
    node_12.properties["border-color"] = "#e9e9e9";
    node_12.properties["border-radius"] = "4px";
    node_12.properties["border-width"] = "1px";
    node_0.substyles["demo"] = node_12;
    alia::style_tree node_13;
    node_13.properties["border-color"] = "#cccccc";
    node_13.properties["border-width"] = "1px";
    alia::style_tree node_14;
    node_14.properties["background"] = "#e4e4e4";
    node_14.properties["color"] = "#101010";
    node_14.properties["default-padding"] = "4px";
    node_13.substyles["item"] = node_14;
    alia::style_tree node_15;
    node_15.properties["background"] = "#c4c4c4";
    node_13.substyles["item.depressed"] = node_15;
    alia::style_tree node_16;
    node_16.properties["background"] = "#d4d4d4";
    node_13.substyles["item.hot"] = node_16;
    alia::style_tree node_17;
    node_17.properties["background"] = "#404040";
    node_17.properties["color"] = "#dddddd";
    node_13.substyles["item.selected"] = node_17;
    node_0.substyles["drop-down-list"] = node_13;
    alia::style_tree node_18;
    node_18.properties["background"] = "#f5f5f5";
    node_18.properties["border-color"] = "#e5e5e5";
    node_18.properties["border-width"] = "1px 0px 0px 0px";
    node_18.properties["color"] = "#333";
    alia::style_tree node_19;
    node_19.properties["color"] = "#559";
    node_18.substyles["link"] = node_19;
    alia::style_tree node_20;
    node_20.properties["color"] = "#113";
    node_20.properties["font-underline"] = "true";
    node_18.substyles["link.depressed"] = node_20;
    alia::style_tree node_21;
    node_21.properties["color"] = "#226";
    node_21.properties["font-underline"] = "true";
    node_18.substyles["link.hot"] = node_21;
    node_0.substyles["footer"] = node_18;
    alia::style_tree node_22;
    node_22.properties["font-bold"] = "true";
    node_22.properties["font-family"] = "helvetica";
    node_22.properties["font-size"] = "24";
    node_0.substyles["h1"] = node_22;
    alia::style_tree node_23;
    node_23.properties["font-bold"] = "true";
    node_23.properties["font-family"] = "helvetica";
    node_23.properties["font-size"] = "16";
    node_0.substyles["h2"] = node_23;
    alia::style_tree node_24;
    node_24.properties["font-bold"] = "true";
    node_24.properties["font-family"] = "helvetica";
    node_24.properties["font-size"] = "13";
    node_0.substyles["h3"] = node_24;
    alia::style_tree node_25;
    node_25.properties["background"] = "#eee";
    node_25.properties["border-color"] = "#d8ddd1";
    node_25.properties["border-width"] = "0px 0px 1px 0px";
    node_25.properties["color"] = "#333";
    node_25.properties["default-padding"] = "6px";
    alia::style_tree node_26;
    node_26.properties["color"] = "#559";
    node_25.substyles["link"] = node_26;
    alia::style_tree node_27;
    node_27.properties["color"] = "#113";
    node_27.properties["font-underline"] = "true";
    node_25.substyles["link.depressed"] = node_27;
    alia::style_tree node_28;
    node_28.properties["color"] = "#226";
    node_28.properties["font-underline"] = "true";
    node_25.substyles["link.hot"] = node_28;
    alia::style_tree node_29;
    node_29.properties["font-bold"] = "true";
    node_29.properties["font-family"] = "georgia";
    node_29.properties["font-size"] = "16";
    node_25.substyles["title"] = node_29;
    node_0.substyles["header"] = node_25;
    alia::style_tree node_30;
    node_30.properties["background"] = "#e4e4e4";
    node_30.properties["color"] = "#101010";
    node_0.substyles["item"] = node_30;
    alia::style_tree node_31;
    node_31.properties["background"] = "#c4c4c4";
    node_0.substyles["item.depressed"] = node_31;
    alia::style_tree node_32;
    node_32.properties["background"] = "#d4d4d4";
    node_0.substyles["item.hot"] = node_32;
    alia::style_tree node_33;
    node_33.properties["background"] = "#404040";
    node_33.properties["color"] = "#dddddd";
    node_0.substyles["item.selected"] = node_33;
    alia::style_tree node_34;
    node_34.properties["color"] = "#08c";
    node_0.substyles["link"] = node_34;
    alia::style_tree node_35;
    node_35.properties["color"] = "#003360";
    node_35.properties["font-underline"] = "true";
    node_0.substyles["link.depressed"] = node_35;
    alia::style_tree node_36;
    node_36.properties["color"] = "#005580";
    node_36.properties["font-underline"] = "true";
    node_0.substyles["link.hot"] = node_36;
    alia::style_tree node_37;
    node_37.properties["background"] = "#f5f5f5";
    node_37.properties["border-color"] = "#e9e9e9";
    node_37.properties["border-radius"] = "4px";
    node_37.properties["border-width"] = "1px";
    node_37.properties["margin"] = "8px 0px 8px 8px";
    alia::style_tree node_38;
    node_38.properties["color"] = "#333";
    node_38.properties["font-bold"] = "true";
    node_38.properties["font-size"] = "14";
    node_37.substyles["accordion-header"] = node_38;
    alia::style_tree node_39;
    node_39.properties["background"] = "#e4e4e4";
    node_37.substyles["accordion-header.depressed"] = node_39;
    alia::style_tree node_40;
    node_40.properties["background"] = "#eaeaea";
    node_37.substyles["accordion-header.hot"] = node_40;
    alia::style_tree node_41;
    node_41.properties["background"] = "#08c";
    node_41.properties["color"] = "#fff";
    node_37.substyles["accordion-header.selected"] = node_41;
    alia::style_tree node_42;
    node_42.properties["color"] = "#559";
    node_42.properties["default-padding"] = "4px 16px";
    node_37.substyles["link"] = node_42;
    alia::style_tree node_43;
    node_43.properties["color"] = "#113";
    node_43.properties["font-underline"] = "true";
    node_37.substyles["link.depressed"] = node_43;
    alia::style_tree node_44;
    node_44.properties["color"] = "#226";
    node_44.properties["font-underline"] = "true";
    node_37.substyles["link.hot"] = node_44;
    alia::style_tree node_45;
    node_45.properties["font-bold"] = "true";
    node_45.properties["font-size"] = "17";
    node_37.substyles["title"] = node_45;
    node_0.substyles["nav"] = node_37;
    alia::style_tree node_46;
    node_46.properties["disable-padding"] = "true";
    node_0.substyles["no-padding"] = node_46;
    alia::style_tree node_47;
    node_47.properties["background"] = "#00000000";
    node_0.substyles["node-expander.normal"] = node_47;
    alia::style_tree node_48;
    node_48.properties["border-radius"] = "50%";
    node_0.substyles["radio-button"] = node_48;
    alia::style_tree node_49;
    node_49.properties["background"] = "#666666";
    node_49.properties["color"] = "#cccccc";
    node_0.substyles["rulers"] = node_49;
    alia::style_tree node_50;
    node_50.properties["color"] = "#555";
    node_0.substyles["scrollbar"] = node_50;
    alia::style_tree node_51;
    node_51.properties["color"] = "#555";
    node_0.substyles["scrollbar.depressed"] = node_51;
    alia::style_tree node_52;
    node_52.properties["color"] = "#555";
    node_0.substyles["scrollbar.hot"] = node_52;
    alia::style_tree node_53;
    node_53.properties["thumb-color"] = "#333333";
    node_53.properties["track-color"] = "#777777";
    node_0.substyles["slider"] = node_53;
    alia::style_tree node_54;
    node_54.properties["thumb-color"] = "#111111";
    node_0.substyles["slider.depressed"] = node_54;
    alia::style_tree node_55;
    node_55.properties["thumb-color"] = "#222222";
    node_0.substyles["slider.hot"] = node_55;
    alia::style_tree node_56;
    node_56.properties["add-background-tab"] = "true";
    node_56.properties["default-padding"] = "7px 4px";
    alia::style_tree node_57;
    node_57.properties["border-color"] = "#ddd";
    node_57.properties["border-radius"] = "4px 4px 0px 0px";
    node_57.properties["border-width"] = "0px 0px 1px 0px";
    node_57.properties["color"] = "#08c";
    node_57.properties["default-padding"] = "4px 5px";
    node_57.properties["padding"] = "1px 1px 0px 1px";
    node_56.substyles["tab"] = node_57;
    alia::style_tree node_58;
    node_58.properties["background"] = "#ddd";
    node_56.substyles["tab.depressed"] = node_58;
    alia::style_tree node_59;
    node_59.properties["background"] = "#eee";
    node_56.substyles["tab.hot"] = node_59;
    alia::style_tree node_60;
    node_60.properties["border-width"] = "1px 1px 0px 1px";
    node_60.properties["color"] = "#2c2c2c";
    node_60.properties["padding"] = "0px 0px 1px 0px";
    node_56.substyles["tab.selected"] = node_60;
    node_0.substyles["tab-strip"] = node_56;
    alia::style_tree node_61;
    alia::style_tree node_62;
    node_62.properties["background"] = "#f7f7f9";
    node_62.properties["border-color"] = "#ececf0";
    node_62.properties["color"] = "#48484c";
    node_62.properties["default-padding"] = "1px 6px";
    node_62.properties["font-family"] = "Consolas";
    node_62.properties["font-size"] = "12";
    alia::style_tree node_63;
    node_63.properties["background"] = "#fbfbfc";
    node_63.properties["border-right-width"] = "1px";
    node_63.properties["color"] = "#bebec5";
    node_63.properties["font-size"] = "12";
    node_62.substyles["first-column"] = node_63;
    alia::style_tree node_64;
    node_64.properties["border-top-width"] = "0px";
    node_62.substyles["first-row"] = node_64;
    node_61.substyles["cell"] = node_62;
    node_0.substyles["table"] = node_61;
    alia::style_tree node_65;
    node_65.properties["font-bold"] = "true";
    node_65.properties["font-family"] = "arial";
    node_65.properties["font-size"] = "15";
    node_0.substyles["tree-heading"] = node_65;
    return node_0;
}

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

    bool is_full_screen;
    // If the window is full screen, this stores the normal placement of it.
    WINDOWPLACEMENT normal_placement;

    timer_request_list timer_requests;
    // This prevents timer requests from being serviced in the same frame that
    // they're requested and thus throwing the event handler into a loop.
    counter_type timer_event_counter;

    optional<ui_time_type> next_update;

    bool mouse_captured;

    impl_data()
      : dc(0), rc(0), hwnd(0), hinstance(0), is_full_screen(false),
        timer_event_counter(0), mouse_captured(false)
    {}
};

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
    void request_refresh(bool greedy);
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

static void disable_vsync()
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
    return GetTickCount();
}

static void destroy_window(native_window::impl_data& impl)
{
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

static void update_window(HWND hwnd)
{
    native_window::impl_data& impl = get_window_data(hwnd);

    impl.next_update = none;

    RECT rect;
    GetClientRect(impl.hwnd, &rect);

    // Don't update if the window has zero size.
    // (This seems to happen if the window is minimized.)
    if (rect.bottom == rect.top || rect.left == rect.right)
        return;

    opengl_surface* surface =
        static_cast<opengl_surface*>(impl.ui.surface.get());
    surface->set_size(
        alia::make_vector<unsigned>(rect.right, rect.bottom));

    mouse_cursor cursor;
    update_ui(impl.ui,
        alia::make_vector<unsigned>(rect.right, rect.bottom),
        get_time(impl), &cursor);
    // Only set the mouse cursor if it's inside the window or captured.
    if (impl.ui.input.mouse_inside_window || impl.mouse_captured)
        set_cursor(cursor);

    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
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

static void on_mouse_button_press(HWND hwnd)
{
    SetCapture(hwnd);
    get_window_data(hwnd).mouse_captured = true;
}

static bool any_mouse_buttons_pressed()
{
    return
        (GetKeyState(VK_LBUTTON) & 0x8000) != 0 ||
        (GetKeyState(VK_RBUTTON) & 0x8000) != 0 ||
        (GetKeyState(VK_MBUTTON) & 0x8000) != 0;
}

static void on_mouse_button_release(HWND hwnd)
{
    if (!any_mouse_buttons_pressed())
    {
        ReleaseCapture();
        get_window_data(hwnd).mouse_captured = false;
    }
}

static bool process_timer_requests(
    native_window::impl_data& impl, unsigned now)
{
    ++impl.timer_event_counter;
    bool processed_any = false;
    if (!impl.timer_requests.empty())
    {
        while (1)
        {
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

            {
                timer_event e(request.id.id, request.trigger_time, now);
                issue_targeted_event(impl.ui, e, request.id);
            }

            update_window(impl.hwnd);
        }
    }
    return processed_any;
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
            process_text_input(impl.ui, get_time(impl), utf8);
        }
        update_window(hwnd);
        break;
      }
 
    case WM_KEYDOWN:
      {
        native_window::impl_data& impl = get_window_data(hwnd);
        bool acknowledged =
            process_key_press(impl.ui, get_time(impl),
                get_key_event_info(wparam));
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
                    acknowledged = true;
                }
                break;
            }
        }
        update_window(hwnd);
        if (acknowledged)
            return 0;
        break;
      }
    case WM_KEYUP:
      {
        native_window::impl_data& impl = get_window_data(hwnd);
        bool acknowledged =
            process_key_release(impl.ui, get_time(impl),
                get_key_event_info(wparam));
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
        vector<2,int> mouse_position =
            make_vector<int>(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
        RECT rect;
        GetClientRect(hwnd, &rect);
        // Only process the movement if the mouse is captured or the movement
        // was within the client area of the window.
        if (get_window_data(hwnd).mouse_captured ||
            (mouse_position[0] >= 0 && mouse_position[0] < rect.right &&
             mouse_position[1] >= 0 && mouse_position[1] < rect.bottom))
        {
            process_mouse_move(get_window_data(hwnd).ui, get_time(hwnd),
                mouse_position);
        }
        reset_mouse_tracking(hwnd);
        update_window(hwnd);
        return 0;
      }

     case WM_MOUSEWHEEL:
      {
        native_window::impl_data& impl = get_window_data(hwnd);
        float movement = float(GET_WHEEL_DELTA_WPARAM(wparam)) / WHEEL_DELTA;
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
        0,
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

    if (initial_state.flags & FULL_SCREEN)
    {
        enter_full_screen(*impl);
        ShowWindow(impl->hwnd, SW_SHOW);
    }
    else if (initial_state.flags & MAXIMIZED)
        ShowWindow(impl->hwnd, SW_MAXIMIZE);
    else
        ShowWindow(impl->hwnd, SW_SHOWNORMAL);

    reset_mouse_tracking(impl->hwnd);

    RECT rect;
    GetClientRect(impl->hwnd, &rect);

    update_ui(impl->ui,
        alia::make_vector<unsigned>(rect.right, rect.bottom),
        get_time(*impl));
}

void native_window::initialize(
    string const& title, window_controller* controller,
    state_data const& initial_state)
{
    controller->window = this;
    impl_ = new impl_data;
    impl_->ui.style.reset(new ui_style);
    impl_->ui.style->styles = parse_style_file("alia.style");
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
    return impl_->next_update || !impl_->timer_requests.empty();
}

void native_window::do_idle_work()
{
    unsigned now = get_time(*impl_);
    bool did_something = false;
    if (process_timer_requests(*impl_, now))
        did_something = true;
    if (impl_->next_update)
    {
        if (int(now - get(impl_->next_update)) >= 0)
        {
            update_window(impl_->hwnd);
            did_something = true;
        }
    }
    //if (!did_something)
    //    Sleep(1);
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
            int r = GetMessage(&msg, NULL, 0, 0);
            if (r == 0 || r == -1) // -1 for error
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

void win32_opengl_surface::request_refresh(bool greedy)
{
    ui_time_type update_time =
        impl_->ui.millisecond_tick_count + (greedy ? 0 : 1);
    if (!impl_->next_update ||
        int(get(impl_->next_update) - update_time) > 0)
    {
        impl_->next_update = update_time;
    }
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
