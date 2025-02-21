#include <alia/ui/backends/wx.hpp>

#include <fstream>

#include <wx/clipbrd.h>
#include <wx/stopwatch.h>
#include <wx/utils.h>

#include <alia/ui/events.hpp>
#include <alia/ui/utilities.hpp>

#pragma warning(push, 0)

#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkShader.h"
#include "include/core/SkSurface.h"

#include "include/gpu/GrBackendSurface.h"
#include "include/gpu/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/gl/GrGLInterface.h"
#include "include/gpu/gl/GrGLTypes.h"

// #include "icu/SkLoadICU.h"

#if defined(_WIN32) && defined(SK_USING_THIRD_PARTY_ICU)
bool
SkLoadICU();
#else
inline bool
SkLoadICU()
{
    return true;
}
#endif // defined(_WIN32) && defined(SK_USING_THIRD_PARTY_ICU)

#pragma warning(pop)

#include <alia/ui/events.hpp>
#include <alia/ui/layout/system.hpp>
#include <alia/ui/system/api.hpp>
#include <alia/ui/system/input_processing.hpp>
#include <alia/ui/system/object.hpp>
#include <alia/ui/system/os_interface.hpp>
#include <alia/ui/system/window_interface.hpp>
#include <alia/ui/utilities/rendering.hpp>
#include <alia/ui/utilities/skia.hpp>

FILE*
get_console()
{
    static FILE* console = nullptr;
    if (!console)
    {
        AllocConsole();
        freopen_s(&console, "CONOUT$", "w", stdout);
    }
    return console;
}

namespace alia {

struct wx_opengl_window::impl_data
{
    ui_system ui;

    // TODO: Bundle this up into a Skia structure?
    std::unique_ptr<GrDirectContext> skia_graphics_context;
    std::unique_ptr<SkSurface> skia_surface;

    wxGLContext* wx_gl_context;
    wx_opengl_window* window;
    int wheel_movement; // accumulates fractional mouse wheel movement
    bool vsync_disabled;
    counter_type last_menu_bar_update;
    // this is used to hold key info between events for the same key press
    wxKeyEvent last_key_down;
};

void
reset_skia(wx_opengl_window::impl_data& impl, vector<2, unsigned> size)
{
    GrGLFramebufferInfo framebuffer_info;
    framebuffer_info.fFBOID = 0;
    framebuffer_info.fFormat = GL_RGBA8;

    auto backend_render_target = GrBackendRenderTargets::MakeGL(
        size[0], size[1], 0, 0, framebuffer_info);

    impl.skia_surface.reset();
    auto surface = SkSurfaces::WrapBackendRenderTarget(
        impl.skia_graphics_context.get(),
        backend_render_target,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        nullptr,
        nullptr);
    if (!surface.get())
        throw alia::exception("Skia surface creation failed");
    impl.skia_surface.reset(surface.release());
}

void
init_skia(wx_opengl_window::impl_data& impl, vector<2, unsigned> size)
{
    static bool globally_initialized = false;
    if (!globally_initialized)
    {
        if (!SkLoadICU())
        {
            throw alia::exception("SkLoadICU failed");
        }
        globally_initialized = true;
    }

    impl.wx_gl_context->SetCurrent(*impl.window);

    impl.skia_graphics_context.reset(
        GrDirectContexts::MakeGL(nullptr, GrContextOptions()).release());
    reset_skia(impl, size);
}

// ENUM TRANSLATION

static key_code
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
            return key_code::BACKSPACE;
        case WXK_TAB:
            return key_code::TAB;
        case WXK_CLEAR:
            return key_code::CLEAR;
        case WXK_RETURN:
            return key_code::ENTER;
        case WXK_PAUSE:
            return key_code::PAUSE;
        case WXK_ESCAPE:
            return key_code::ESCAPE;
        case WXK_SPACE:
            return key_code::SPACE;
        case WXK_PAGEUP:
            return key_code::PAGE_UP;
        case WXK_PAGEDOWN:
            return key_code::PAGE_DOWN;
        case WXK_END:
            return key_code::END;
        case WXK_HOME:
            return key_code::HOME;
        case WXK_UP:
            return key_code::UP;
        case WXK_DOWN:
            return key_code::DOWN;
        case WXK_LEFT:
            return key_code::LEFT;
        case WXK_RIGHT:
            return key_code::RIGHT;
        case WXK_PRINT:
            return key_code::PRINT_SCREEN;
        case WXK_INSERT:
            return key_code::INSERT;
        case WXK_DELETE:
            return key_code::DEL;
        case WXK_HELP:
            return key_code::HELP;
        case WXK_F1:
            return key_code::F1;
        case WXK_F2:
            return key_code::F2;
        case WXK_F3:
            return key_code::F3;
        case WXK_F4:
            return key_code::F4;
        case WXK_F5:
            return key_code::F5;
        case WXK_F6:
            return key_code::F6;
        case WXK_F7:
            return key_code::F7;
        case WXK_F8:
            return key_code::F8;
        case WXK_F9:
            return key_code::F9;
        case WXK_F10:
            return key_code::F10;
        case WXK_F11:
            return key_code::F11;
        case WXK_F12:
            return key_code::F12;
        case WXK_F13:
            return key_code::F13;
        case WXK_F14:
            return key_code::F14;
        case WXK_F15:
            return key_code::F15;
        case WXK_F16:
            return key_code::F16;
        case WXK_F17:
            return key_code::F17;
        case WXK_F18:
            return key_code::F18;
        case WXK_F19:
            return key_code::F19;
        case WXK_F20:
            return key_code::F20;
        case WXK_F21:
            return key_code::F21;
        case WXK_F22:
            return key_code::F22;
        case WXK_F23:
            return key_code::F23;
        case WXK_F24:
            return key_code::F24;

            // TODO: Use separate key codes for numpad keys.

        case WXK_NUMPAD_ENTER:
            return key_code::ENTER;
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
            return key_code::EQUAL;
        case WXK_NUMPAD_UP:
            return key_code::UP;
        case WXK_NUMPAD_DOWN:
            return key_code::DOWN;
        case WXK_NUMPAD_LEFT:
            return key_code::LEFT;
        case WXK_NUMPAD_RIGHT:
            return key_code::RIGHT;
        case WXK_NUMPAD_INSERT:
            return key_code::INSERT;
        case WXK_NUMPAD_DELETE:
            return key_code::DEL;
        case WXK_NUMPAD_HOME:
            return key_code::HOME;
        case WXK_NUMPAD_END:
            return key_code::END;
    }

    // Return ASCII characters untranslated.
    if (code < 0x80)
        return key_code(code);

    return key_code::UNKNOWN;
}

static modded_key
get_modded_key(wxKeyEvent const& event)
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
    return modded_key{translate_key_code(event.GetKeyCode()), mods};
}

static wxCursor
translate_mouse_cursor(mouse_cursor cursor)
{
    switch (cursor)
    {
        case mouse_cursor::NONE:
            return wxCursor(wxCURSOR_BLANK);
        case mouse_cursor::DEFAULT:
        default:
            return wxCursor(wxCURSOR_ARROW);
        case mouse_cursor::CROSSHAIR:
            return wxCursor(wxCURSOR_CROSS);
        case mouse_cursor::WAIT:
            return wxCursor(wxCURSOR_WAIT);
        case mouse_cursor::TEXT:
            return wxCursor(wxCURSOR_IBEAM);
        case mouse_cursor::NOT_ALLOWED:
            return wxCursor(wxCURSOR_NO_ENTRY);
        case mouse_cursor::POINTER:
            return wxCursor(wxCURSOR_HAND);
        case mouse_cursor::EW_RESIZE:
            return wxCursor(wxCURSOR_SIZEWE);
        case mouse_cursor::NS_RESIZE:
            return wxCursor(wxCURSOR_SIZENS);
        case mouse_cursor::NESW_RESIZE:
            return wxCursor(wxCURSOR_SIZENESW);
        case mouse_cursor::NWSE_RESIZE:
            return wxCursor(wxCURSOR_SIZENWSE);
        case mouse_cursor::MOVE:
            return wxCursor(wxCURSOR_SIZING);
        case mouse_cursor::ZOOM_IN:
            return wxCursor(wxCURSOR_MAGNIFIER);
    }
}

// EXTERNAL INTERFACE

struct wx_external_interface : default_external_interface
{
    wx_external_interface(wx_opengl_window::impl_data& impl)
        : default_external_interface(impl.ui), impl_(impl)
    {
    }

    void
    schedule_animation_refresh() override
    {
        fprintf(get_console(), "schedule_animation_refresh\n");
        impl_.window->Refresh(false);
    }

    wx_opengl_window::impl_data& impl_;
};

// OS INTERFACE

struct wx_os_interface : os_interface
{
    wx_os_interface(wx_opengl_window::impl_data& impl) : impl_(impl)
    {
    }

    wx_opengl_window::impl_data& impl_;

    std::optional<std::string>
    get_clipboard_text() override;

    void
    set_clipboard_text(std::string text) override;
};

std::optional<std::string>
wx_os_interface::get_clipboard_text()
{
    if (wxTheClipboard->Open())
    {
        if (wxTheClipboard->IsSupported(wxDF_TEXT))
        {
            wxTextDataObject data;
            wxTheClipboard->GetData(data);
            return std::string(data.GetText().c_str());
        }
        wxTheClipboard->Close();
    }
    return std::nullopt;
}

void
wx_os_interface::set_clipboard_text(std::string text)
{
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(text.c_str()));
        wxTheClipboard->Flush();
        wxTheClipboard->Close();
    }
}

// OPENGL WINDOW

#define INVOKE_CALLBACK(callback)                                             \
    try                                                                       \
    {                                                                         \
        callback                                                              \
    }                                                                         \
    catch (std::exception & e)                                                \
    {                                                                         \
        show_error(e.what());                                                 \
    }                                                                         \
    catch (...)                                                               \
    {                                                                         \
        show_error("An unknown error has occurred.");                         \
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
// EVT_SCROLLWIN_THUMBTRACK(wx_opengl_window::on_scroll)
END_EVENT_TABLE()

// static void
// set_cursor(wx_opengl_window::impl_data& impl, mouse_cursor cursor)
// {
//     // wxCURSOR_BLANK doesn't seem to work on Windows, so instead just hide
//     // the mouse cursor. (This also means we have to make sure the cursor is
//     // shown again when we don't want a blank cursor.)
//     // #ifdef __WXMSW__
//     //     if (cursor == mouse_cursor::NONE)
//     //     {
//     //         while (ShowCursor(false) >= 0)
//     //             ;
//     //     }
//     //     else
//     //     {
//     //         impl.window->SetCursor(translate_mouse_cursor(cursor));
//     //         while (ShowCursor(true) < 0)
//     //             ;
//     //     }
//     // #else
//     impl.window->SetCursor(translate_mouse_cursor(cursor));
//     // #endif
// }

void
render_ui(wx_opengl_window::impl_data& impl)
{
    // TODO: Track this ourselves.
    vector<2, int> size;
    impl.window->GetClientSize(&size[0], &size[1]);

    impl.ui.surface_size = make_vector<unsigned>(size[0], size[1]);

    impl.wx_gl_context->SetCurrent(*impl.window);

    auto& canvas = *impl.skia_surface->getCanvas();
    canvas.resetMatrix();
    canvas.clipRect(SkRect::MakeWH(SkScalar(size[0]), SkScalar(size[1])));

    // TODO: Don't clear automatically.
    {
        SkPaint paint;
        paint.setColor(as_skcolor(impl.ui.theme.background.base.main));
        canvas.drawPaint(paint);
    }

    {
        render_event event;
        event.canvas = impl.skia_surface->getCanvas();
        dispatch_event(impl.ui, event, RENDER_EVENT);
    }

    impl.skia_graphics_context->flush();

    impl.window->SwapBuffers();
}

void
update_ui(wx_opengl_window::impl_data& impl)
{
    // TODO: Track this ourselves.
    int width, height;
    impl.window->GetClientSize(&width, &height);

    refresh_system(impl.ui);
    update(impl.ui);

    resolve_layout(
        impl.ui.layout,
        make_box(
            make_layout_vector(0, 0),
            make_layout_vector(layout_scalar(width), layout_scalar(height))));

    render_ui(impl);
}

static void
update_window(wx_opengl_window::impl_data& impl)
{
    // vector<2, int> size;
    // impl.window->GetClientSize(&size[0], &size[1]);
    // impl.ui.surface_size = make_vector<unsigned>(size[0], size[1]);

    // mouse_cursor cursor;
    update_ui(impl);
    // set_cursor(impl, cursor);

    // If the menu bar has changed, find the parent frame, test if it's an
    // alia frame, and if so, ask it to update its menu bar.
    // if (impl.ui.menu_bar.last_change != impl.last_menu_bar_update)
    // {
    //     wxWindow* frame = impl.window;
    //     while (!frame->IsTopLevel())
    //         frame = frame->GetParent();
    //     wx_frame* alia_frame = dynamic_cast<wx_frame*>(frame);
    //     if (alia_frame)
    //         alia_frame->update_menu_bar(impl.window, impl.ui.menu_bar);
    //     impl.last_menu_bar_update = impl.ui.menu_bar.last_change;
    // }

    // impl.window->Refresh(false);
    // impl.window->Update();
}

static void
handle_paint(wx_opengl_window::impl_data& impl)
{
    fprintf(get_console(), "handle_paint\n");

    vector<2, int> size;
    impl.window->GetClientSize(&size[0], &size[1]);
    impl.ui.surface_size = make_vector<unsigned>(size[0], size[1]);

    // This is required even though the DC is not actually used.
    wxPaintDC dc(impl.window);

    // if (!impl.vsync_disabled)
    // {
    //     disable_vsync();
    //     impl.vsync_disabled = true;
    // }

    update_ui(impl);
}

static mouse_button
translate_button(int wx_button)
{
    switch (wx_button)
    {
        case wxMOUSE_BTN_LEFT:
            return mouse_button::LEFT;
        case wxMOUSE_BTN_MIDDLE:
            return mouse_button::MIDDLE;
        case wxMOUSE_BTN_RIGHT:
            return mouse_button::RIGHT;
        default:
            return mouse_button(-1);
    }
}

static void
handle_mouse(wx_opengl_window::impl_data& impl, wxMouseEvent& event)
{
    // if (event.GetEventType() == wxEVT_MOUSEWHEEL)
    // {
    //     impl.wheel_movement += event.GetWheelRotation();
    //     int lines = impl.wheel_movement / event.GetWheelDelta();
    //     impl.wheel_movement -= lines * event.GetWheelDelta();
    //     if (lines != 0)
    //     {
    //         process_scroll(impl.ui, make_vector<int>(0, lines));
    //         update_window(impl);
    //     }
    //     return;
    // }

    // Get the current mouse position.
    vector<2, int> position = make_vector<int>(event.GetX(), event.GetY());

    // Determine if the mouse is in the surface.
    {
        vector<2, int> client_size;
        impl.window->GetClientSize(&client_size[0], &client_size[1]);
        if (impl.window->HasCapture()
            || (!event.Leaving()
                && is_inside(
                    box<2, int>(make_vector(0, 0), client_size), position)))
        {
            process_mouse_motion(impl.ui, vector2d(position));
        }
        else
            process_mouse_loss(impl.ui);
    }

    if (event.ButtonDClick())
    {
        process_double_click(
            impl.ui,
            /* TODO: position, */ translate_button(event.GetButton()));
        if (!impl.window->HasCapture())
            impl.window->CaptureMouse();
    }
    else if (event.ButtonDown())
    {
        process_mouse_press(
            impl.ui,
            /* TODO: position, */ translate_button(event.GetButton()));
        impl.window->SetFocus();
        if (!impl.window->HasCapture())
            impl.window->CaptureMouse();
    }
    else if (event.ButtonUp())
    {
        process_mouse_release(
            impl.ui,
            /* TODO: position, */ translate_button(event.GetButton()));
        // TODO: fix this mess
        if (!event.LeftIsDown() && !event.MiddleIsDown()
            && !event.RightIsDown() && impl.window->HasCapture())
        {
            impl.window->ReleaseMouse();
        }
    }

    // update_window(impl);
    impl.window->Refresh(false);
}

static void
handle_scroll(wx_opengl_window::impl_data& impl, wxScrollWinEvent& event)
{
    process_scroll(
        impl.ui,
        event.GetOrientation() == wxVERTICAL
            ? make_vector<double>(0, event.GetPosition())
            : make_vector<double>(event.GetPosition(), 0));
    impl.window->Refresh(false);
}

static void
handle_key_down(wx_opengl_window::impl_data& impl, wxKeyEvent& event)
{
    // ui_time_type time = get_time(impl);
    modded_key info = get_modded_key(event);

    // If ALT or CTRL is pressed, assume there's no text equivalent and just
    // process it as a normal key press.
    if (event.AltDown() || event.ControlDown())
    {
        // Try processing it as a focused key press.
        bool acknowledged = process_focused_key_press(impl.ui, info);

        // Try processing it as a background key press.
        if (!acknowledged)
            acknowledged = process_background_key_press(impl.ui, info);

        if (!acknowledged)
            event.Skip();
    }
    else
    {
        impl.last_key_down = event;
        event.Skip();
    }
}

static void
handle_char(wx_opengl_window::impl_data& impl, wxKeyEvent& event)
{
    // ui_time_type time = get_time(impl);
    modded_key info = get_modded_key(event);

    if (!event.AltDown() && !event.ControlDown())
    {
        // Try processing it as a focused key press.
        bool acknowledged = process_focused_key_press(impl.ui, info);

        // TODO: Try processing it as text.
        // if (!acknowledged)
        // {
        //     wxChar unicode = event.GetUnicodeKey();
        //     if (unicode != 0 && unicode != '\t') // don't count TAB as text
        //     {
        //         wxChar buffer[2] = {unicode, 0};
        //         wxString string(buffer);
        //         wxCharBuffer char_buffer = string.utf8_str();
        //         utf8_string utf8;
        //         utf8.begin = char_buffer.data();
        //         utf8.end = char_buffer.data() + char_buffer.length();
        //         acknowledged
        //             = process_text_input(impl.ui, get_time(impl), utf8);
        //     }
        // }

        // Try processing it as a background key press.
        if (!acknowledged)
            acknowledged = process_background_key_press(impl.ui, info);

        update_window(impl);
        if (!acknowledged)
            event.Skip();
    }
    else
        event.Skip();
}

static void
handle_key_up(wx_opengl_window::impl_data& impl, wxKeyEvent& event)
{
    bool acknowledged = process_key_release(impl.ui, get_modded_key(event));
    update_window(impl);
    if (!acknowledged)
        event.Skip();
}

// static widget_id
// resolve_wx_menu_id(menu_node const* nodes, int* id)
// {
//     for (menu_node const* i = nodes; i; i = i->next)
//     {
//         switch (i->type)
//         {
//             case SUBMENU_NODE: {
//                 submenu_node const* node = static_cast<submenu_node
//                 const*>(i); widget_id resolved =
//                 resolve_wx_menu_id(node->children, id); if (resolved)
//                     return resolved;
//                 --(*id);
//                 break;
//             }
//             case MENU_ITEM_NODE: {
//                 if (!(*id)--)
//                     return i;
//                 break;
//             }
//             case MENU_SEPARATOR_NODE:
//                 break;
//         }
//     }
//     return 0;
// }

// static widget_id
// resolve_wx_menu_bar_id(menu_container const& spec, int* id)
// {
//     for (menu_node const* i = spec.children; i; i = i->next)
//     {
//         assert(i->type == SUBMENU_NODE);
//         submenu_node const* node = static_cast<submenu_node const*>(i);
//         widget_id resolved = resolve_wx_menu_id(node->children, id);
//         if (resolved)
//             return resolved;
//     }
//     return 0;
// }

// static void
// handle_menu(wx_opengl_window::impl_data& impl, wxCommandEvent& event)
// {
//     int id = event.GetId();
//     widget_id target = resolve_wx_menu_bar_id(impl.ui.menu_bar, &id);
//     if (target)
//     {
//         menu_item_selection_event event(target);
//         issue_event(impl.ui, event);
//     }
//     update_window(impl);
// }

struct wx_window_interface : window_interface
{
    wx_window_interface(wx_opengl_window::impl_data& impl) : impl_(impl)
    {
    }

    void
    set_mouse_cursor(mouse_cursor cursor) override
    {
        impl_.window->SetCursor(translate_mouse_cursor(cursor));
    }

    wx_opengl_window::impl_data& impl_;
};

wx_opengl_window::wx_opengl_window(
    std::function<void(ui_context)> controller,
    wxWindow* parent,
    wxWindowID id,
    wxGLAttributes const& canvas_attribs,
    wxGLContextAttrs const& context_attribs,
    wxPoint const& pos,
    wxSize const& size,
    long style,
    wxString const& name)
    : wxGLCanvas(
          parent,
          canvas_attribs,
          id,
          pos,
          size,
          style | wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE,
          name)
{
    impl_ = new impl_data;
    impl_data& impl = *impl_;

    impl.window = this;

    impl.wx_gl_context = new wxGLContext(this, NULL, &context_attribs);

    impl.vsync_disabled = false;

    impl.last_menu_bar_update = 0;

    impl.wheel_movement = 0;

    // wxScreenDC dc;
    //  wxSize ppi = dc.GetPPI();

    initialize(
        impl_->ui,
        controller,
        new wx_external_interface(*impl_),
        std::make_shared<wx_os_interface>(*impl_),
        std::make_shared<wx_window_interface>(*impl_));

    // TODO: Do this in a better way.
    int width, height;
    impl_->window->GetClientSize(&width, &height);
    impl_->ui.surface_size = make_vector<unsigned>(width, height);

    init_skia(*impl_, make_vector<unsigned>(width, height));

    update_ui(*impl_);
}
wx_opengl_window::~wx_opengl_window()
{
    impl_->wx_gl_context->SetCurrent(*impl_->window);
    impl_->skia_surface.reset();
    impl_->skia_graphics_context.reset();

    delete impl_->wx_gl_context;
    delete impl_;
}
void
wx_opengl_window::update()
{
    update_window(*impl_);
}
void
wx_opengl_window::on_paint(wxPaintEvent&)
{
    fprintf(get_console(), "[%d] on_paint\n", impl_->ui.tick_count);
    handle_paint(*impl_);
}
void
wx_opengl_window::on_erase_background(wxEraseEvent&)
{
}
void
wx_opengl_window::on_size(wxSizeEvent&)
{
    int width, height;
    impl_->window->GetClientSize(&width, &height);
    reset_skia(*impl_, make_vector<unsigned>(width, height));
    update_window(*impl_);
}
void
wx_opengl_window::on_mouse(wxMouseEvent& event)
{
    fprintf(get_console(), "[%d] on_mouse\n", impl_->ui.tick_count);
    handle_mouse(*impl_, event);
}
void
wx_opengl_window::on_scroll(wxScrollWinEvent& event)
{
    fprintf(get_console(), "[%d] on_scroll\n", impl_->ui.tick_count);
    handle_scroll(*impl_, event);
}
void
wx_opengl_window::on_set_focus(wxFocusEvent&)
{
    // wx_opengl_window::impl_data& impl = *impl_;
    // TODO: process_focus_gain(impl.ui);
    // update_window(impl);
}
void
wx_opengl_window::on_kill_focus(wxFocusEvent&)
{
    // wx_opengl_window::impl_data& impl = *impl_;
    // TODO: process_focus_loss(impl.ui);
    // update_window(impl);
}

void
wx_opengl_window::on_idle(wxIdleEvent& event)
{
    wx_opengl_window::impl_data& impl = *impl_;
    impl.ui.tick_count = impl.ui.external->get_tick_count();

    fprintf(get_console(), "[%d] on_idle\n", impl.ui.tick_count);

    invoke_ready_callbacks(
        impl.ui.scheduler,
        impl.ui.tick_count,
        [&](std::function<void()> const& callback, millisecond_count) {
            callback();
            this->Refresh(false);
        });

    // if (hack_refresh_needed)
    // {
    //     this->Refresh(false);
    //     hack_refresh_needed = false;
    //     event.RequestMore();
    // }

    if (has_scheduled_callbacks(impl.ui.scheduler))
    {
        fprintf(get_console(), "has_scheduled_callbacks\n");
        event.RequestMore();
    }
}
void
wx_opengl_window::on_key_down(wxKeyEvent& event)
{
    handle_key_down(*impl_, event);
}
void
wx_opengl_window::on_char(wxKeyEvent& event)
{
    handle_char(*impl_, event);
}
void
wx_opengl_window::on_key_up(wxKeyEvent& event)
{
    handle_key_up(*impl_, event);
}
void
wx_opengl_window::on_menu(wxCommandEvent&)
{
    // TODO: handle_menu(*impl_, event);
}
void
wx_opengl_window::on_sys_color_change(wxSysColourChangedEvent&)
{
}

alia::ui_system&
wx_opengl_window::ui()
{
    return impl_->ui;
}

struct wx_frame::impl_data
{
    // alia__shared_ptr<app_window_controller> controller;

    // position/size of the window when it's not maximized or full screen
    box<2, int> normal_rect;

    // This window is in control of the menu bar. (The app should ensure that
    // it stays alive as long as the menu bar is active.)
    wxWindow* menu_bar_controller;
};

BEGIN_EVENT_TABLE(wx_frame, wxFrame)
// EVT_MENU(-1, wx_frame::on_menu)
// EVT_SIZE(wx_frame::on_size)
// EVT_MOVE(wx_frame::on_move)
EVT_CLOSE(wx_frame::on_close)
END_EVENT_TABLE()

wx_frame::wx_frame(
    std::function<void(ui_context)>,
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

    // controller->window = this;
    // impl.controller = controller;

    impl.normal_rect = make_box(
        make_vector<int>(pos.x, pos.y),
        make_vector<int>(size.GetWidth(), size.GetHeight()));
}

wx_frame::~wx_frame()
{
    delete impl_;
}

app_window_state
wx_frame::state() const
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

bool
wx_frame::is_full_screen() const
{
    return this->IsFullScreen();
}

void
wx_frame::set_full_screen(bool fs)
{
    this->ShowFullScreen(fs);
}

// static void
// build_wx_menu(wxMenu* wx_menu, menu_node const* nodes, int* next_id)
// {
//     for (menu_node const* i = nodes; i; i = i->next)
//     {
//         switch (i->type)
//         {
//             case SUBMENU_NODE: {
//                 submenu_node const* node = static_cast<submenu_node
//                 const*>(i);

//                 wxMenu* submenu = new wxMenu;
//                 build_wx_menu(submenu, node->children, next_id);

//                 int id = (*next_id)++;
//                 wx_menu->Append(
//                     id,
//                     wxString(get(node->label).c_str(), wxConvUTF8),
//                     submenu);
//                 if (!node->enabled)
//                     wx_menu->Enable(id, false);

//                 break;
//             }

//             case MENU_ITEM_NODE: {
//                 menu_item_node const* node
//                     = static_cast<menu_item_node const*>(i);

//                 int id = (*next_id)++;

//                 if (node->checked)
//                 {
//                     wx_menu->AppendCheckItem(
//                         id, wxString(get(node->label).c_str(), wxConvUTF8));
//                     wx_menu->Check(id, get(node->checked));
//                 }
//                 else
//                 {
//                     wx_menu->Append(
//                         id, wxString(get(node->label).c_str(), wxConvUTF8));
//                 }

//                 if (!node->enabled)
//                     wx_menu->Enable(id, false);

//                 break;
//             }

//             case MENU_SEPARATOR_NODE: {
//                 wx_menu->AppendSeparator();
//                 break;
//             }
//         }
//     }
// }

// static wxMenuBar*
// build_wx_menu_bar(menu_container const& spec)
// {
//     wxMenuBar* bar = new wxMenuBar;

//     int next_id = 0;

//     for (menu_node const* i = spec.children; i; i = i->next)
//     {
//         assert(i->type == SUBMENU_NODE);
//         submenu_node const* node = static_cast<submenu_node const*>(i);
//         wxMenu* wx_menu = new wxMenu;
//         build_wx_menu(wx_menu, node->children, &next_id);
//         bar->Append(wx_menu, wxString(get(node->label).c_str(),
//         wxConvUTF8));
//     }

//     return bar;
// }

// static void
// fix_wx_menu_bar(wxMenuBar* bar, menu_container const& spec)
// {
//     int n = 0;
//     for (menu_node const* i = spec.children; i; i = i->next)
//     {
//         submenu_node const* node = static_cast<submenu_node const*>(i);
//         if (!node->enabled)
//             bar->EnableTop(n, false);
//     }
// }

// void
// wx_frame::update_menu_bar(wxWindow* controller, menu_container const&
// menu_bar)
// {
//     this->SetMenuBar(build_wx_menu_bar(menu_bar));
//     fix_wx_menu_bar(this->GetMenuBar(), menu_bar);
//     impl_->menu_bar_controller = controller;
// }

void
wx_frame::on_menu(wxCommandEvent& event)
{
    impl_->menu_bar_controller->GetEventHandler()->ProcessEvent(event);
}

void
wx_frame::close()
{
    this->Close();
}

void
wx_frame::on_size(wxSizeEvent& event)
{
    if (!this->IsMaximized() && !this->IsFullScreen())
    {
        vector<2, int> size;
        this->GetSize(&size[0], &size[1]);
        impl_->normal_rect.size = size;
    }
    event.Skip();
}

void
wx_frame::on_move(wxMoveEvent& event)
{
    if (!this->IsMaximized() && !this->IsFullScreen())
    {
        vector<2, int> p;
        this->GetPosition(&p[0], &p[1]);
        impl_->normal_rect.corner = p;
    }
    event.Skip();
}

void
wx_frame::on_exit(wxCommandEvent&)
{
    this->Close(true);
}

void
wx_frame::on_close(wxCloseEvent& event)
{
    // TODO: Send shut down events to any children that have alia UIs?
    // wxList& children = this->GetChildren();
    // for (wxList::Node* i = children.GetFirst(); i; i = i->GetNext())
    // {
    //     wx_opengl_window* gl_window
    //         = dynamic_cast<wx_opengl_window*>(i->GetData());
    //     if (gl_window)
    //     {
    //         shutdown_event event;
    //         issue_event(gl_window->ui(), event);
    //     }
    // }
    event.Skip();
}

wx_frame*
create_wx_framed_window(
    std::string const& title,
    std::function<void(ui_context)> controller,
    app_window_state const& initial_state)
{
    wx_frame* frame = new wx_frame(
        controller,
        0,
        wxID_ANY,
        title.c_str(),
        initial_state.position
            ? wxPoint(
                  (*initial_state.position)[0], (*initial_state.position)[1])
            : wxDefaultPosition,
        wxSize(initial_state.size[0], initial_state.size[1]));

    wxGLAttributes canvas_attribs;
    canvas_attribs.PlatformDefaults().RGBA().DoubleBuffer().EndList();

    wxGLContextAttrs context_attribs;
    context_attribs.CoreProfile()
        .OGLVersion(3, 2)
        .ForwardCompatible()
        .EndList();

    new wx_opengl_window(
        controller,
        frame,
        wxID_ANY,
        canvas_attribs,
        context_attribs,
        wxDefaultPosition,
        wxSize(initial_state.size[0], initial_state.size[1]));

    // // Create a sizer, and make sure the content window fills it.
    // wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    // sizer->Add(contents, 1, wxEXPAND, 0);
    // frame->SetSizer(sizer);

    // Show the frame.
    // if (initial_state.flags & APP_WINDOW_FULL_SCREEN)
    {
        // This is the only sequence of commands I've found that creates a
        // full screen window without flickering and without causing a weird
        // blank window when the user switches back to windowed mode.
        // frame->Freeze();
        // frame->Show(true);
        frame->ShowFullScreen(true);
        // frame->Thaw();
    }
    // else
    // {
    //     frame->Show(true);
    // }

    return frame;
}

} // namespace alia
