#include <alia/opengl/gl.hpp>
#include <alia/wx/opengl_surface.hpp>
#include <alia/wx/callback.hpp>
#include <alia/wx/context_holder.hpp>
#include <alia/wx/manager.hpp>
#include <alia/millisecond_clock.hpp>
#include <alia/wx/popup_window.hpp>
#include <alia/wx/menu.hpp>
#include <alia/cached_ascii_text.hpp>
#include <alia/wx/standard_dialogs.hpp>
#include <alia/opengl/utils.hpp>
#include <alia/opengl/context.hpp>
#include <alia/exception.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <alia/input_events.hpp>
#include <alia/menu/spec_builder.hpp>
#include <alia/menu/selection_dispatcher.hpp>
#include <alia/menu/interface.hpp>
#include <alia/transformations.hpp>

#include <wx/clipbrd.h>
#include <wx/utils.h>

#include <list>
#include <map>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

namespace alia { namespace wx {

static int gl_canvas_attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER,
    WX_GL_STENCIL_SIZE, 1, 0 };

struct timer_request
{
    unsigned time_requested, duration;
    region_id id;
};
typedef std::vector<timer_request> timer_request_list;

struct opengl_surface::impl_data
{
    context_holder* holder;
    gl_canvas_wrapper* canvas;
    int wheel_movement; // accumulates fractional mouse wheel movement
    timer_request_list timer_requests;
    std::list<popup_window*> popups;
    int* selected_menu_id_ptr;
    bool update_needed;
    mouse_cursor current_cursor, desired_cursor;
    bool vsync_disabled;
};

class opengl_surface::gl_canvas_wrapper : public wxGLCanvas
{
 public:
    gl_canvas_wrapper(opengl_surface* owner, wxWindow* parent)
      : wxGLCanvas(parent, wxID_ANY, gl_canvas_attribs, wxDefaultPosition,
            wxDefaultSize, wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE)
      , owner(owner)
    {
    }
    ~gl_canvas_wrapper()
    {
        if (owner)
            owner->impl_->canvas = 0;
    }
    void on_paint(wxPaintEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_paint, owner));
    }
    void on_erase_background(wxEraseEvent& event) {}
    void on_size(wxSizeEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_resize, owner));
    }
    void on_mouse(wxMouseEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_mouse, owner, event));
    }
    void on_set_focus(wxFocusEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_focus_gain, owner));
    }
    void on_kill_focus(wxFocusEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_focus_loss, owner));
    }
    void on_idle(wxIdleEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_idle, owner, &event));
    }
    void on_key_down(wxKeyEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_key_down, owner, &event));
    }
    void on_key_up(wxKeyEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_key_up, owner, &event));
    }
    void on_char(wxKeyEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_char, owner, &event));
    }
    void on_menu(wxCommandEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_menu, owner, event.GetId()));
    }
    void on_sys_color_change(wxSysColourChangedEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_sys_color_change, owner));
    }
    opengl_surface* owner;
 private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(opengl_surface::gl_canvas_wrapper, wxGLCanvas)
    EVT_PAINT(opengl_surface::gl_canvas_wrapper::on_paint)
    EVT_ERASE_BACKGROUND(
        opengl_surface::gl_canvas_wrapper::on_erase_background)
    EVT_SIZE(opengl_surface::gl_canvas_wrapper::on_size)
    EVT_MOUSE_EVENTS(opengl_surface::gl_canvas_wrapper::on_mouse)
    EVT_SET_FOCUS(opengl_surface::gl_canvas_wrapper::on_set_focus)
    EVT_KILL_FOCUS(opengl_surface::gl_canvas_wrapper::on_kill_focus)
    EVT_KEY_DOWN(opengl_surface::gl_canvas_wrapper::on_key_down)
    EVT_KEY_UP(opengl_surface::gl_canvas_wrapper::on_key_up)
    EVT_CHAR(opengl_surface::gl_canvas_wrapper::on_char)
    EVT_IDLE(opengl_surface::gl_canvas_wrapper::on_idle)
    EVT_MENU(-1, opengl_surface::gl_canvas_wrapper::on_menu)
    EVT_SYS_COLOUR_CHANGED(
        opengl_surface::gl_canvas_wrapper::on_sys_color_change)
END_EVENT_TABLE()

class opengl_surface::scoped_selected_menu_id_ptr
{
 public:
    scoped_selected_menu_id_ptr(opengl_surface* surface, int* new_ptr)
    {
        surface_ = surface;
        old_ptr_ = surface_->impl_->selected_menu_id_ptr;
        surface_->impl_->selected_menu_id_ptr = new_ptr;
    }

    ~scoped_selected_menu_id_ptr()
    {
        surface_->impl_->selected_menu_id_ptr = old_ptr_;
    }

 private:
    opengl_surface* surface_;
    int* old_ptr_;
};

static boost::scoped_ptr<wxGLContext> shared_wx_context;
static boost::scoped_ptr<opengl::context> shared_gl_context;

static std::map<font,boost::shared_ptr<native::ascii_text_renderer> >
    cached_text_renderers;

opengl_surface::opengl_surface()
{
    impl_ = new impl_data;
    impl_->holder = 0;
    impl_->canvas = 0;
    impl_->selected_menu_id_ptr = 0;
    impl_->wheel_movement = 0;
    impl_->vsync_disabled = false;
    impl_->current_cursor = (mouse_cursor)-1;
    impl_->update_needed = false;
}

opengl_surface::~opengl_surface()
{
    if (impl_->canvas)
    {
        impl_->canvas->owner = 0;
        impl_->canvas->Destroy();
    }
    orphan_popups();
    delete impl_;
}

void opengl_surface::set_holder(context_holder* holder)
{ impl_->holder = holder; }

void opengl_surface::create(wxWindow* parent)
{
    impl_->canvas = new gl_canvas_wrapper(this, parent);

    if (!shared_wx_context)
    {
        shared_wx_context.reset(new wxGLContext(impl_->canvas));
        shared_gl_context.reset(new opengl::context);
    }
    set_opengl_context(*shared_gl_context);

    update_size();
}

context& opengl_surface::get_context() const
{ return impl_->holder->context; }

context_holder& opengl_surface::get_holder() const
{ return *impl_->holder; }

void opengl_surface::update_mouse_cursor()
{
    if (impl_->desired_cursor == impl_->current_cursor)
        return;
    // update the mouse cursor
    wxCursor cursor;
    switch (impl_->desired_cursor)
    {
     case CROSS_CURSOR:
        cursor = wxCursor(wxCURSOR_CROSS);
        break;
     case BUSY_CURSOR:
        cursor = *wxHOURGLASS_CURSOR;
        break;
     case BLANK_CURSOR:
        cursor = wxCursor(wxCURSOR_BLANK);
        break;
     case NO_ENTRY_CURSOR:
        cursor = wxCursor(wxCURSOR_NO_ENTRY);
        break;
     case IBEAM_CURSOR:
        cursor = wxCursor(wxCURSOR_IBEAM);
        break;
     case HAND_CURSOR:
        cursor = wxCursor(wxCURSOR_HAND);
        break;
     case LEFT_RIGHT_ARROW_CURSOR:
        cursor = wxCursor(wxCURSOR_SIZEWE);
        break;
     case UP_DOWN_ARROW_CURSOR:
        cursor = wxCursor(wxCURSOR_SIZENS);
        break;
     case FOUR_WAY_ARROW_CURSOR:
        cursor = wxCursor(wxCURSOR_SIZING);
        break;
     default:
        cursor = *wxSTANDARD_CURSOR;
    }
    impl_->canvas->SetCursor(cursor);
    impl_->current_cursor = impl_->desired_cursor;
}

void opengl_surface::handle_paint()
{
    // Windows requires this
    wxPaintDC dc(impl_->canvas);

    set_current();

    update_size();

    initialize_render_state();

    event e(RENDER_CATEGORY, RENDER_EVENT, SCREEN_CULLING);
    issue_event(get_context(), e);

    if (!impl_->vsync_disabled)
    {
     #ifdef WGL_EXT_swap_control
        if (WGLEW_EXT_swap_control)
            wglSwapIntervalEXT(0);
     #endif
        impl_->vsync_disabled = true;
    }
    impl_->canvas->SwapBuffers();

    opengl::check_errors();

    update_mouse_cursor();
}

void opengl_surface::handle_resize()
{
    vector2i old_size = get_size();
    update_size();
    if (impl_->canvas->IsShownOnScreen() && get_size() != old_size)
        schedule_update();
}

void opengl_surface::handle_idle(wxIdleEvent* event)
{
    if (wx::manager::get_instance().exiting())
        return;

    if (!impl_->timer_requests.empty())
    {
        unsigned now = millisecond_clock::get_instance().get_tick_count();
        while (1)
        {
            // Ideally, the list would be stored sorted, but it has to be
            // sorted relative to the current tick count (to handle wrapping),
            // and the list is generally not very long anyway.
            timer_request_list::iterator next_event =
                impl_->timer_requests.end();
            unsigned next_event_time;
            for (timer_request_list::iterator
                i = impl_->timer_requests.begin();
                i != impl_->timer_requests.end(); ++i)
            {
                unsigned t = i->time_requested + i->duration;
                if (int(now - t) >= 0 &&
                    (next_event == impl_->timer_requests.end() ||
                    int(next_event_time - t) >= 0))
                {
                    next_event = i;
                    next_event_time = t;
                }
            }
            if (next_event == impl_->timer_requests.end())
                break;
            timer_event te(next_event->id, next_event->time_requested,
                next_event->duration, now);
            impl_->timer_requests.erase(next_event);
            issue_event(get_context(), te);
            schedule_update();
        }
        event->RequestMore();
    }

    idle_event e;
    issue_event(get_context(), e);
    if (e.request_more || impl_->update_needed)
        event->RequestMore();
    if (e.refresh || impl_->update_needed)
        schedule_update();

    wxMilliSleep(10);
}

static mouse_button translate_button(int wx_button)
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

void opengl_surface::handle_mouse(wxMouseEvent const& event)
{
    // Wheel events are treated specially because it seems they end up going to
    // the wrong window sometimes.  In particular, if there's an active popup,
    // other mouse events will go to the popup, but wheel events will go to the
    // parent surface.
    if (event.GetEventType() == wxEVT_MOUSEWHEEL)
    {
        impl_->wheel_movement += event.GetWheelRotation();
        int lines = impl_->wheel_movement / event.GetWheelDelta();
        impl_->wheel_movement -= lines * event.GetWheelDelta();
        if (lines == 0)
            return;
        if (!impl_->popups.empty() && impl_->popups.front()->is_open())
        {
            impl_->popups.front()->surface->handle_mouse(event);
        }
        else
        {
            process_scroll_wheel_movement(get_context(), lines);
            schedule_update();
        }
        return;
    }

    // Get the current mouse position.
    point2i position(event.GetX(), event.GetY());

    // Determine if the mouse is in the surface.
    {
        vector2i client_size;
        impl_->canvas->GetClientSize(&client_size[0], &client_size[1]);
        if (impl_->canvas->HasCapture() || !event.Leaving() &&
            is_inside(box2i(point2i(0, 0), client_size), position))
        {
            set_mouse_position(get_context(), position);
        }
        else
            clear_mouse_position(get_context());
    }

    if (impl_->popups.empty())
    {
        if (event.ButtonDClick())
        {
            process_double_click(get_context(),
                translate_button(event.GetButton()));
            if (!impl_->canvas->HasCapture())
                impl_->canvas->CaptureMouse();
        }
        else if (event.ButtonDown())
        {
            process_mouse_press(get_context(),
                translate_button(event.GetButton()));
            impl_->canvas->SetFocus();
            if (!impl_->canvas->HasCapture())
                impl_->canvas->CaptureMouse();
        }
        else if (event.ButtonUp())
        {
            process_mouse_release(get_context(),
                translate_button(event.GetButton()));
            // TODO: fix this mess
            if (!event.LeftIsDown() && !event.MiddleIsDown() &&
                !event.RightIsDown() && impl_->canvas->HasCapture())
            {
                impl_->canvas->ReleaseMouse();
            }
        }
    }

    schedule_update();
}

void opengl_surface::set_current()
{
    impl_->canvas->SetCurrent(*shared_wx_context);

    static bool glew_initialized = false;
    if (!glew_initialized)
    {
        GLenum err = glewInit();
        if (err != GLEW_OK)
        {
            throw exception("failed to initialize GLEW: " +
                std::string((char const*)(glewGetErrorString(err))));
        }
        glew_initialized = true;
    }
}

//bool opengl_surface::process_input_event(input_event const& event)
//{
//    if (inside_pass())
//        return false;
//
//    //update_ticks();
//    //set_current();
//    bool processed = get_context().process_input_event(event);
//    return processed;
//}

void opengl_surface::handle_focus_gain()
{
    //get_context().process_input_event(input_event(FOCUS_GAIN));
    ////wx::manager::get_instance().update();
}

void opengl_surface::handle_focus_loss()
{
    //set_mouse_position(false, geometry::point2i(0, 0));

    //if (impl_->canvas->HasCapture())
    //    impl_->canvas->ReleaseMouse();

    //// If there is a mouse button pressed, we have to simulate it being
    //// released when we lose focus, because we won't see the real release.
    //if (get_context().get_pressed_mouse_button() != NO_BUTTON)
    //{
    //    get_context().process_input_event(input_event(BUTTON_UP,
    //        get_pressed_mouse_button()));
    //}

    //get_context().process_input_event(input_event(FOCUS_LOSS));
    //schedule_update();
}

void opengl_surface::update_size()
{
    vector2i size;
    impl_->canvas->GetSize(&size[0], &size[1]);
    set_size(size);
}

native::ascii_text_renderer* opengl_surface::get_ascii_text_renderer(
    font const& font)
{
    native::ascii_text_renderer* renderer;
    std::map<alia::font,boost::shared_ptr<native::ascii_text_renderer> >
        ::iterator i = cached_text_renderers.find(font);
    if (i == cached_text_renderers.end())
    {
        renderer = new native::ascii_text_renderer(*this, font);
        cached_text_renderers[font].reset(renderer);
    }
    else
        renderer = i->second.get();
    return renderer;
}

vector2i opengl_surface::get_ascii_text_size(font const& font,
    char const* text)
{
    native::ascii_text_renderer* renderer = get_ascii_text_renderer(font);
    return vector2i(renderer->get_string_width(text),
        renderer->get_metrics().height);
}

void opengl_surface::draw_ascii_text(point2d const& location,
    rgba8 const& color, font const& font, char const* text)
{
    native::ascii_text_renderer* renderer = get_ascii_text_renderer(font);
    renderer->write(location, color, text);
}

void opengl_surface::cache_text(
    cached_text_ptr& data,
    font const& font,
    char const* text,
    int width,
    unsigned flags)
{
    native::ascii_text_renderer* renderer = get_ascii_text_renderer(font);
    data.reset(new cached_ascii_text(*this, *renderer, text, width, flags));
}

void opengl_surface::get_font_metrics(font_metrics* metrics,
    font const& font)
{
    native::ascii_text_renderer* renderer = get_ascii_text_renderer(font);
    *metrics = renderer->get_metrics();
}

void opengl_surface::request_timer_event(region_id id, unsigned ms)
{
    unsigned now;
    if (get_context().event->type == TIMER_EVENT)
        now = get_event<timer_event>(get_context()).now;
    else
        now = millisecond_clock::get_instance().get_tick_count();
    // If an event already exists for that ID, then reschedule it.
    for (timer_request_list::iterator i = impl_->timer_requests.begin();
        i != impl_->timer_requests.end(); ++i)
    {
        if (i->id == id)
        {
            i->time_requested = now;
            i->duration = ms;
            return;
        }
    }
    // Otherwise, add a new event.
    timer_request rq;
    rq.time_requested = now;
    rq.duration = ms;
    rq.id = id;
    impl_->timer_requests.push_back(rq);
}

void opengl_surface::set_mouse_cursor(mouse_cursor cursor)
{
    impl_->desired_cursor = cursor;
}

void opengl_surface::handle_key_down(wxKeyEvent* event)
{
    //if (impl_->holder->process_key(*event))
    //    return;

    key_event_info info;
    info.code = static_cast<key_code>(event->GetKeyCode());
    info.mods = event->GetModifiers();
    key_event e(KEY_DOWN_EVENT, info, get_context().focused_id);
    issue_event(get_context(), e);
    schedule_update();
    if (e.processed)
        return;

    event->Skip();
}

void opengl_surface::handle_char(wxKeyEvent* event)
{
    // Ignore char events that have modifiers (ALT-, CTRL-, etc).
    if (event->HasModifiers())
    {
        event->Skip();
        return;
    }

    char_event e(event->GetKeyCode(), get_context().focused_id);
    issue_event(get_context(), e);
    schedule_update();
    if (e.processed)
        return;

    event->Skip();
}

void opengl_surface::handle_key_up(wxKeyEvent* event)
{
    key_event_info info;
    info.code = static_cast<key_code>(event->GetKeyCode());
    info.mods = event->GetModifiers();
    key_event e(KEY_UP_EVENT, info, get_context().focused_id);
    issue_event(get_context(), e);
    schedule_update();
    if (e.processed)
        return;

    event->Skip();
}

vector2i opengl_surface::get_display_size() const
{
    vector2i s;
    wxDisplaySize(&s[0], &s[1]);
    return s;
}

point2i opengl_surface::get_location_on_display() const
{
    point2i p(0, 0);
    impl_->canvas->ClientToScreen(&p[0], &p[1]);
    return p;
}

popup_interface* opengl_surface::open_popup(controller* controller,
    point2i const& display_location, point2i const& boundary,
    vector2i const& minimum_size, vector2i const& maximum_size,
    bool right_aligned)
{
    context& ctx = get_context();
    point2d dl = transform_point(ctx.pass_state.transformation,
        point2d(display_location));
    point2d b = transform_point(ctx.pass_state.transformation,
        point2d(boundary));
    // TODO
    popup_window* popup = new popup_window(this, controller,
        point2i(int(dl[0] + 0.5), int(dl[1] + 0.5)),
        point2i(int(b[0] + 0.5), int(b[1] + 0.5)),
        minimum_size, maximum_size, right_aligned);
    impl_->popups.push_back(popup);
    return popup;
}

void opengl_surface::close()
{
    if (impl_->holder != NULL)
        impl_->holder->close();
}

std::string opengl_surface::get_clipboard_text()
{
    if (!wxTheClipboard->Open())
        return "";

    std::string text;

    if (wxTheClipboard->IsSupported(wxDF_TEXT))
    {
        wxTextDataObject data;
        wxTheClipboard->GetData(data);
        text = data.GetText().c_str();
    }

    wxTheClipboard->Close();

    return text;
}

void opengl_surface::set_clipboard_text(std::string const& text)
{
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(text.c_str()));
        wxTheClipboard->Close();
    }
}

void opengl_surface::show_popup_menu(menu_controller* controller)
{
    int selected_id = -1;
    scoped_selected_menu_id_ptr ssmip(this, &selected_id);

    menu_spec spec;
    {
    menu_spec_builder builder(&spec);
    controller->do_menu(builder);
    }

    wxMenu menu;
    build_wx_menu(&menu, spec.items);

    impl_->canvas->PopupMenu(&menu);

    if (selected_id != -1)
    {
        menu_selection_dispatcher dispatcher(selected_id);
        controller->do_menu(dispatcher);
    }
}

void opengl_surface::handle_menu(int id)
{
    if (impl_->selected_menu_id_ptr != NULL)
        *impl_->selected_menu_id_ptr = id;
}

void opengl_surface::update_popups()
{
    for (std::list<popup_window*>::const_iterator
        i = impl_->popups.begin(); i != impl_->popups.end(); )
    {
        if ((*i)->was_dismissed())
        {
            (*i)->orphan();
            i = impl_->popups.erase(i);
        }
        else
        {
            (*i)->update();
            ++i;
        }
    }
}
void opengl_surface::orphan_popups()
{
    for (std::list<popup_window*>::const_iterator
        i = impl_->popups.begin(); i != impl_->popups.end(); ++i)
    {
        (*i)->orphan();
    }
}
void opengl_surface::remove_popup(popup_window* popup)
{
    impl_->popups.remove(popup);
}

void opengl_surface::update()
{
    //if (get_context().is_inside_pass())
    //    return;

    impl_->update_needed = do_layout(get_context());
    // TODO: fix this
    //impl_->holder->adjust_to_content_size(get_context().content_size);

    update_hot_id(get_context());

    impl_->canvas->Refresh();
    // TODO: This shouldn't be here. Need a better way to deal with expensive
    // dragging.
    impl_->canvas->Update();

    update_popups();
}

void opengl_surface::on_close()
{
    delete this;
}

wxWindow* opengl_surface::get_wx_window()
{
    return impl_->canvas;
}

vector2d opengl_surface::get_ppi() const
{
    wxScreenDC dc;
    wxSize ppi = dc.GetPPI();
    return vector2d(ppi.GetWidth(), ppi.GetHeight());
}

void opengl_surface::beep()
{
    wxBell();
}

void opengl_surface::handle_sys_color_change()
{
    get_context().artist->on_system_theme_change();
    impl_->holder->update();
}

void opengl_surface::schedule_update()
{
    wx::manager::get_instance().update();
}

bool opengl_surface::ask_for_color(rgb8* result, rgb8 const* initial)
{
    return wx::ask_for_color(result, initial);
}

}}
