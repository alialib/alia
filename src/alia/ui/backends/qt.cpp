#if 0

#include <alia/qt.hpp>
#include <alia/ui_system.hpp>
#include <alia/opengl.hpp>

#include <QtGui>
#include <QGLWidget>
#include <QScreen>

#include <alia/lua_styling.hpp>

namespace alia {

class sleeper_thread : public QThread
{
 public:
    static void msleep(unsigned long msecs) {
        QThread::msleep(msecs);
    }
};

struct timer_request
{
    ui_time_type trigger_time;
    routable_widget_id id;
    counter_type frame_issued;
};
typedef std::vector<timer_request> timer_request_list;

struct qt_gl_window : public QGLWidget
{
    qt_gl_window(qt_window::impl_data& impl, QGLFormat const& format,
        QWidget* parent, Qt::WindowFlags flags)
      : QGLWidget(format, parent, 0, flags)
      , impl(impl)
    {}

    // Qt callbacks
    void paintGL();
    bool event(QEvent* event);

    qt_window::impl_data& impl;
};

struct qt_window::impl_data
{
    ui_system ui;

    opengl_context gl_ctx;

    qt_gl_window* window;

    // If this is a popup window, then this is the window's parent.
    // Otherwise, it's 0.
    impl_data* parent;

    // If this window has a child popup, then this is it.
    impl_data* popup;

    millisecond_clock clock;

    timer_request_list timer_requests;
    counter_type timer_event_counter;

    bool update_needed;

    bool close_requested;

    // When returning from full screen, we need to know if the widget was
    // maximized and should be restored to that state.
    bool was_maximized;

    impl_data()
      : parent(0), popup(0), timer_event_counter(0), update_needed(false),
        close_requested(false), was_maximized(false)
    {}
};

static vector<2,unsigned> get_display_size(qt_window::impl_data& impl)
{
    QSize size = QApplication::desktop()->screenGeometry(impl.window).size();
    return make_vector(unsigned(size.width()), unsigned(size.height()));
}

struct qt_opengl_surface : opengl_surface
{
    qt_opengl_surface(qt_window::impl_data* impl)
      : impl_(impl)
    {}
    vector<2,float> ppi() const
    {
        return make_vector(
            float(impl_->window->physicalDpiX()),
            float(impl_->window->physicalDpiY()));
    }
    vector<2,unsigned> display_size() const
    {
        return get_display_size(*impl_);
    }
    popup_interface* open_popup(
        ui_controller* controller,
        vector<2,int> const& primary_position,
        vector<2,int> const& boundary,
        vector<2,int> const& minimum_size);
    void close_popups();
    void request_refresh();
    void request_timer_event(routable_widget_id const& id, unsigned ms);
    string get_clipboard_text();
    void set_clipboard_text(string const& text);
 private:
    qt_window::impl_data* impl_;
};

static void set_cursor(qt_window::impl_data& impl, mouse_cursor cursor)
{
    QCursor qcursor;
    switch (cursor)
    {
     case DEFAULT_CURSOR:
     default:
        qcursor.setShape(Qt::ArrowCursor);
        break;
     case CROSS_CURSOR:
        qcursor.setShape(Qt::CrossCursor);
        break;
     case BUSY_CURSOR:
        qcursor.setShape(Qt::BusyCursor);
        break;
     case BLANK_CURSOR:
        qcursor.setShape(Qt::BlankCursor);
        break;
     case IBEAM_CURSOR:
        qcursor.setShape(Qt::IBeamCursor);
        break;
     case NO_ENTRY_CURSOR:
        qcursor.setShape(Qt::ForbiddenCursor);
        break;
     case HAND_CURSOR:
        qcursor.setShape(Qt::OpenHandCursor);
        break;
     case LEFT_RIGHT_ARROW_CURSOR:
        qcursor.setShape(Qt::SizeHorCursor);
        break;
     case UP_DOWN_ARROW_CURSOR:
        qcursor.setShape(Qt::SizeVerCursor);
        break;
     case FOUR_WAY_ARROW_CURSOR:
        qcursor.setShape(Qt::SizeAllCursor);
        break;
    }
    impl.window->setCursor(qcursor);
}

static void paint_window(qt_window::impl_data& impl)
{
    opengl_surface* surface =
        static_cast<opengl_surface*>(impl.ui.surface.get());

    QSize size = impl.window->size();
    surface->set_size(
        alia::make_vector<unsigned>(size.width(), size.height()));

    surface->initialize_render_state();

    render_ui(impl.ui);
}

static key_code translate_key_code(int code)
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
     case Qt::Key_Backspace:
        return KEY_BACKSPACE;
     case Qt::Key_Tab:
        return KEY_TAB;
     case Qt::Key_Clear:
        return KEY_CLEAR;
     case Qt::Key_Return:
        return KEY_ENTER;
     case Qt::Key_Pause:
        return KEY_PAUSE;
     case Qt::Key_Escape:
        return KEY_ESCAPE;
     case Qt::Key_Space:
        return KEY_SPACE;
     case Qt::Key_PageUp:
        return KEY_PAGEUP;
     case Qt::Key_PageDown:
        return KEY_PAGEDOWN;
     case Qt::Key_End:
        return KEY_END;
     case Qt::Key_Home:
        return KEY_HOME;
     case Qt::Key_Up:
        return KEY_UP;
     case Qt::Key_Down:
        return KEY_DOWN;
     case Qt::Key_Left:
        return KEY_LEFT;
     case Qt::Key_Right:
        return KEY_RIGHT;
     case Qt::Key_Print:
        return KEY_PRINT_SCREEN;
     case Qt::Key_Insert:
        return KEY_INSERT;
     case Qt::Key_Delete:
        return KEY_DELETE;
     case Qt::Key_Help:
        return KEY_HELP;
     case Qt::Key_F1:
        return KEY_F1;
     case Qt::Key_F2:
        return KEY_F2;
     case Qt::Key_F3:
        return KEY_F3;
     case Qt::Key_F4:
        return KEY_F4;
     case Qt::Key_F5:
        return KEY_F5;
     case Qt::Key_F6:
        return KEY_F6;
     case Qt::Key_F7:
        return KEY_F7;
     case Qt::Key_F8:
        return KEY_F8;
     case Qt::Key_F9:
        return KEY_F9;
     case Qt::Key_F10:
        return KEY_F10;
     case Qt::Key_F11:
        return KEY_F11;
     case Qt::Key_F12:
        return KEY_F12;
     case Qt::Key_F13:
        return KEY_F13;
     case Qt::Key_F14:
        return KEY_F14;
     case Qt::Key_F15:
        return KEY_F15;
     case Qt::Key_F16:
        return KEY_F16;
     case Qt::Key_F17:
        return KEY_F17;
     case Qt::Key_F18:
        return KEY_F18;
     case Qt::Key_F19:
        return KEY_F19;
     case Qt::Key_F20:
        return KEY_F20;
     case Qt::Key_F21:
        return KEY_F21;
     case Qt::Key_F22:
        return KEY_F22;
     case Qt::Key_F23:
        return KEY_F23;
     case Qt::Key_F24:
        return KEY_F24;
    }

    return KEY_UNKNOWN;
}

static key_event_info get_key_event_info(QKeyEvent const& event)
{
    key_modifiers mods = KMOD_NONE;
    if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0)
        mods |= KMOD_SHIFT;
    if ((QApplication::keyboardModifiers() & Qt::ControlModifier) != 0)
        mods |= KMOD_CTRL;
    if ((QApplication::keyboardModifiers() & Qt::AltModifier) != 0)
        mods |= KMOD_ALT;
    if ((QApplication::keyboardModifiers() & Qt::MetaModifier) != 0)
        mods |= KMOD_META;
    return key_event_info(translate_key_code(event.key()), mods);
}

static inline ui_time_type get_time(qt_window::impl_data& impl)
{
    return impl.clock.get_tick_count();
}

static void close_popup(qt_window::impl_data& impl);

static void destroy_window(qt_window::impl_data& impl)
{
    close_popup(impl);
    // If this is a popup, clear parent's pointer to this popup.
    if (impl.parent)
    {
        impl.parent->popup = 0;
        impl.parent = 0;
    }

    if (impl.window)
    {
        impl.window->close();
        impl.window = 0;
    }
}

static void close_popup(qt_window::impl_data& impl)
{
    if (impl.popup)
    {
        assert(impl.popup->parent == &impl);
        destroy_window(*impl.popup);
        // Child should have cleared this.
        assert(!impl.popup);
    }
}

static void update_window(qt_window::impl_data& impl)
{
    if (impl.close_requested)
    {
        destroy_window(impl);
        return;
    }

    QSize size = impl.window->size();

    // Don't update if the window has zero size.
    // (This seems to happen if the window is minimized.)
    if (size.width() == 0 || size.height() == 0)
        return;

    refresh_and_layout(impl.ui,
        alia::make_vector<unsigned>(size.width(), size.height()),
        get_time(impl));

    opengl_surface* surface =
        static_cast<opengl_surface*>(impl.ui.surface.get());
    surface->set_size(
        alia::make_vector<unsigned>(size.width(), size.height()));

    optional<mouse_cursor> cursor = update_mouse_cursor(impl.ui);
    if (cursor)
        set_cursor(impl, get(cursor));

    impl.window->update();

    if (impl.popup)
        update_window(*impl.popup);

    impl.update_needed = false;
}

static bool is_popup(qt_window::impl_data& impl)
{
    return impl.parent != 0;
}

static void process_timer_requests(qt_window::impl_data& impl)
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
            {
                if (!processed_any)
                {
                    sleeper_thread::msleep(1);
                }
                break;
            }

            processed_any = true;

            timer_request request = *next_event;
            impl.timer_requests.erase(next_event);

            timer_event e(request.id.id, request.trigger_time, now);
            issue_targeted_event(impl.ui, e, request.id);

            update_window(impl);
        }
    }
}

void qt_gl_window::paintGL()
{
    paint_window(impl);
}

bool qt_gl_window::event(QEvent* event)
{
    switch (event->type())
    {
     case QEvent::KeyPress:
      {
        QKeyEvent* e = static_cast<QKeyEvent*>(event);
        bool acknowledged = false;
        // Get UTF8 text.
        QByteArray ba = e->text().toUtf8();
        utf8_string utf8;
        utf8.begin = ba.data();
        utf8.end = utf8.begin + ba.size();
        // Get key event info.
        key_event_info info = get_key_event_info(*e);
        if (impl.popup)
        {
            if (!acknowledged && !is_empty(utf8))
            {
                acknowledged =
                    process_text_input(impl.popup->ui, get_time(impl), utf8);
            }
            if (!acknowledged)
            {
                acknowledged =
                    process_key_press(impl.popup->ui, get_time(impl), info);
            }
        }
        if (!acknowledged && !is_empty(utf8))
        {
            acknowledged =
                process_text_input(impl.ui, get_time(impl), utf8);
        }
        if (!acknowledged)
        {
            acknowledged =
                process_key_press(impl.ui, get_time(impl), info);
        }
        if (!acknowledged && info.code == KEY_TAB)
        {
            if (info.mods == KMOD_SHIFT)
            {
                regress_focus(impl.ui);
                acknowledged = true;
            }
            else if (info.mods == 0)
            {
                advance_focus(impl.ui);
                acknowledged = true;
            }
        }
        update_window(impl);
        return acknowledged;
      }
     case QEvent::KeyRelease:
      {
        QKeyEvent* e = static_cast<QKeyEvent*>(event);
        bool acknowledged = false;
        key_event_info info = get_key_event_info(*e);
        if (impl.popup)
        {
            acknowledged =
                process_key_release(impl.popup->ui, get_time(impl), info);
        }
        if (!acknowledged)
        {
            acknowledged =
                process_key_release(impl.ui, get_time(impl), info);
        }
        update_window(impl);
        return acknowledged;
      }
     case QEvent::MouseButtonPress:
      {
        QMouseEvent* e = static_cast<QMouseEvent*>(event);
        mouse_button button;
        switch (e->button())
        {
         case Qt::LeftButton:
            button = LEFT_BUTTON;
            break;
         case Qt::MiddleButton:
            button = MIDDLE_BUTTON;
            break;
         case Qt::RightButton:
            button = RIGHT_BUTTON;
            break;
         default:
            return false;
        }
        process_mouse_press(impl.ui, get_time(impl),
            make_vector<int>(e->x(), e->y()), button);
        update_window(impl);
        return true;
      }
     case QEvent::MouseButtonDblClick:
      {
        QMouseEvent* e = static_cast<QMouseEvent*>(event);
        mouse_button button;
        switch (e->button())
        {
         case Qt::LeftButton:
            button = LEFT_BUTTON;
            break;
         case Qt::MiddleButton:
            button = MIDDLE_BUTTON;
            break;
         case Qt::RightButton:
            button = RIGHT_BUTTON;
            break;
         default:
            return false;
        }
        process_double_click(impl.ui, get_time(impl),
            make_vector<int>(e->x(), e->y()), button);
        update_window(impl);
        return true;
      }
     case QEvent::MouseButtonRelease:
      {
        QMouseEvent* e = static_cast<QMouseEvent*>(event);
        mouse_button button;
        switch (e->button())
        {
         case Qt::LeftButton:
            button = LEFT_BUTTON;
            break;
         case Qt::MiddleButton:
            button = MIDDLE_BUTTON;
            break;
         case Qt::RightButton:
            button = RIGHT_BUTTON;
            break;
         default:
            return false;
        }
        process_mouse_release(impl.ui, get_time(impl),
            make_vector<int>(e->x(), e->y()), button);
        update_window(impl);
        return true;
      }
     case QEvent::MouseMove:
      {
        QMouseEvent* e = static_cast<QMouseEvent*>(event);
        process_mouse_move(impl.ui, get_time(impl),
            make_vector<int>(e->x(), e->y()));
        update_window(impl);
        return true;
      }
     case QEvent::Leave:
        process_mouse_leave(impl.ui, get_time(impl));
        update_window(impl);
        break;
     case QEvent::Wheel:
      {
        QWheelEvent* e = static_cast<QWheelEvent*>(event);
        float movement = float(e->delta()) / 120;
        if (impl.popup)
            process_mouse_wheel(impl.popup->ui, get_time(impl), movement);
        else
            process_mouse_wheel(impl.ui, get_time(impl), movement);
        update_window(impl);
        return 0;
      }
        break;
     case QEvent::FocusIn:
        process_focus_gain(impl.ui, get_time(impl));
        update_window(impl);
        break;
     case QEvent::FocusOut:
        process_focus_loss(impl.ui, get_time(impl));
        update_window(impl);
        break;
     case QEvent::Resize:
        update_window(impl);
        break;
     case QEvent::Timer:
        process_timer_requests(impl);
        if (impl.update_needed)
            update_window(impl);
        break;
    }
    return QGLWidget::event(event);
}

static void throw_qt_error(string const& prefix)
{
    throw qt_error(prefix);
}

static void throw_window_creation_error(string const& fn_name)
{
    throw_qt_error("unable to create window: " + fn_name + " failed");
}

void create_window(
    qt_window::impl_data* impl,
    qt_window::impl_data* parent,
    string const& title,
    alia__shared_ptr<ui_controller> const& controller,
    qt_window::state_data const& initial_state)
{
    impl->parent = parent;
    bool is_popup = parent != 0;
    if (parent)
    {
        assert(!parent->popup);
        parent->popup = impl;
    }

    impl->ui.style.reset(new ui_style);
    read_lua_style_file(&impl->ui.style->styles, "style.lua");

    qt_opengl_surface* surface = new alia::qt_opengl_surface(impl);
    surface->set_opengl_context(impl->gl_ctx);

    impl->ui.controller = controller;
    impl->ui.surface.reset(surface);

    QGLFormat format;
    format.setSwapInterval(0);
    impl->window = new qt_gl_window(*impl, format,
        parent ? parent->window : 0, is_popup ? Qt::Popup : Qt::Window);

    impl->window->setWindowTitle(title.c_str());

    impl->window->setMouseTracking(true);

    if (initial_state.position)
    {
        impl->window->move(
            get(initial_state.position)[0],
            get(initial_state.position)[1]);
    }
    impl->window->resize(initial_state.size[0], initial_state.size[1]);

    if (!is_popup)
    {
        if (initial_state.flags & FULL_SCREEN)
        {
            impl->window->showFullScreen();
            impl->was_maximized = (initial_state.flags & MAXIMIZED) != 0;
        }
        else if (initial_state.flags & MAXIMIZED)
        {
            impl->window->showMaximized();
        }
        else
        {
            impl->window->showNormal();
        }
    }
    else
    {
        impl->window->show();
    }

    QSize size = impl->window->size();

    refresh_and_layout(impl->ui,
        alia::make_vector<unsigned>(size.width(), size.height()),
        get_time(*impl));

    ui_event initial_visibility(NO_CATEGORY, INITIAL_VISIBILITY_EVENT);
    issue_event(impl->ui, initial_visibility);

    impl->window->startTimer(0);
}

void qt_window::initialize(
    string const& title, window_controller* controller,
    state_data const& initial_state)
{
    controller->window = this;
    impl_ = new qt_window::impl_data;
    create_window(impl_, 0, title, alia__shared_ptr<ui_controller>(controller),
        initial_state);
}

qt_window::~qt_window()
{
    if (impl_)
    {
        destroy_window(*impl_);
        delete impl_;
    }
}

alia::ui_system& qt_window::ui()
{ return impl_->ui; }

qt_window::state_data qt_window::state() const
{
    qt_window::state_data state;
    state.flags = NO_FLAGS;
    if (impl_->window->isMaximized() ||
        impl_->window->isFullScreen() && impl_->was_maximized)
    {
        state.flags |= MAXIMIZED;
    }
    if (impl_->window->isFullScreen())
    {
        state.flags |= FULL_SCREEN;
    }
    QRect rect = impl_->window->rect();
    state.position = make_vector<int>(rect.left(), rect.top());
    state.size = make_vector<int>(rect.width(), rect.height());
    return state;
}

bool qt_window::is_full_screen() const
{
    return impl_->window->isFullScreen();
}

void qt_window::set_full_screen(bool fs)
{
    if (!impl_->window->isFullScreen())
    {
        impl_->was_maximized = impl_->window->isMaximized();
        impl_->window->showFullScreen();
    }
    else
    {
        if (impl_->was_maximized)
            impl_->window->showMaximized();
        else
            impl_->window->showNormal();
    }
}

struct qt_popup_window : popup_interface
{
    qt_popup_window(
        qt_window::impl_data* parent,
        ui_controller* controller,
        vector<2,int> const& primary_position,
        vector<2,int> const& boundary,
        vector<2,int> const& minimum_size);
    ~qt_popup_window();

    alia::ui_system& ui() { return impl_->ui; }

    bool is_open() const
    {
        return impl_->window != 0;
    }
    void close()
    {
        impl_->close_requested = true;
    }

 private:
    qt_window::impl_data* impl_;
};

static vector<2,int> client_to_screen(QWidget* widget, vector<2,int> const& p)
{
    QPoint q = widget->mapToGlobal(QPoint(p[0], p[1]));
    return make_vector<int>(q.x(), q.y());
}

static vector<2,int> screen_to_client(QWidget* widget, vector<2,int> const& p)
{
    QPoint q = widget->mapFromGlobal(QPoint(p[0], p[1]));
    return make_vector<int>(q.x(), q.y());
}

qt_popup_window::qt_popup_window(
    qt_window::impl_data* parent,
    ui_controller* controller,
    vector<2,int> const& primary_position,
    vector<2,int> const& boundary,
    vector<2,int> const& minimum_size)
{
    impl_ = new qt_window::impl_data;

    impl_->ui.style = parent->ui.style;

    alia__shared_ptr<ui_controller> controller_ptr(controller);

    layout_vector size =
        measure_initial_ui(controller_ptr, parent->ui.style,
            parent->ui.surface);
    for (unsigned i = 0; i != 2; ++i)
    {
        if (size[i] < minimum_size[i])
            size[i] = minimum_size[i];
    }

    vector<2,int> lower_display_boundary = screen_to_client(parent->window,
        make_vector<int>(0, 0));
    vector<2,int> upper_display_boundary = screen_to_client(parent->window,
        vector<2,int>(get_display_size(*parent)));

    vector<2,int> position, actual_size;
    for (unsigned i = 0; i != 2; ++i)
    {
        if (primary_position[i] + size[i] <= upper_display_boundary[i] ||
            boundary[i] - lower_display_boundary[i] <
            upper_display_boundary[i] - primary_position[i])
        {
            position[i] = primary_position[i];
            actual_size[i] = (std::min)(size[i],
                upper_display_boundary[i] - primary_position[i]);
        }
        else
        {
            actual_size[i] = (std::min)(size[i],
                boundary[i] - lower_display_boundary[i]);
            position[i] = boundary[i] - actual_size[i];
        }
    }

    create_window(impl_, parent, "popup", controller_ptr,
        qt_window::state_data(
            client_to_screen(parent->window, position),
            actual_size));
}
qt_popup_window::~qt_popup_window()
{
    destroy_window(*impl_);
}

popup_interface*
qt_opengl_surface::open_popup(
    ui_controller* controller,
    vector<2,int> const& primary_position,
    vector<2,int> const& boundary,
    vector<2,int> const& minimum_size)
{
    return new qt_popup_window(
        impl_, controller, primary_position, boundary, minimum_size);
}

void qt_opengl_surface::close_popups()
{
    close_popup(*impl_);
}

void qt_opengl_surface::request_refresh()
{
    impl_->update_needed = true;
}

void qt_opengl_surface::request_timer_event(
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

string qt_opengl_surface::get_clipboard_text()
{
    QClipboard* clipboard = QApplication::clipboard();
    QByteArray ba = clipboard->text().toUtf8();
    return string(ba.data());
}

void qt_opengl_surface::set_clipboard_text(string const& text)
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(text.c_str());
}

}

#endif
