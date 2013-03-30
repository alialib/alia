#include <alia/ui/backends/qt.hpp>
#include <alia/ui/system.hpp>
#include <alia/ui/backends/opengl.hpp>
#include <alia/ui/utilities.hpp>

#include <QtGui>
#include <QGLWidget>
#include <QScreen>

namespace alia {

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
    void timerEvent(QTimerEvent* event);

    qt_window::impl_data& impl;
};

struct qt_window::impl_data
{
    ui_system ui;

    opengl_context gl_ctx;

    qt_gl_window* window;

    QElapsedTimer timer;

    // When returning from full screen, we need to know if the widget was
    // maximized and should be restored to that state.
    bool was_maximized;

    impl_data() : was_maximized(false) {}
};

struct qt_os_interface : os_interface
{
    string get_clipboard_text();
    void set_clipboard_text(string const& text);
};

static unsigned get_time(qt_window::impl_data& impl)
{
    return impl.timer.elapsed();
}

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
     case OPEN_HAND_CURSOR:
        qcursor.setShape(Qt::OpenHandCursor);
        break;
     case POINTING_HAND_CURSOR:
        qcursor.setShape(Qt::PointingHandCursor);
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

static key_code translate_key_code(int code)
{
    // Translate letters to their lowercase equivalents.
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

    // Return ASCII characters untranslated.
    if (code < 0x80)
        return key_code(code);

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

static void destroy_window(qt_window::impl_data& impl)
{
    if (impl.window)
    {
        impl.window->close();
        impl.window = 0;
    }
}

static void update_window(qt_window::impl_data& impl)
{
    opengl_surface* surface =
        static_cast<opengl_surface*>(impl.ui.surface.get());
    QSize size = impl.window->size();

    mouse_cursor cursor;
    update_ui(impl.ui,
        alia::make_vector<unsigned>(size.width(), size.height()),
        get_time(impl), &cursor);
    set_cursor(impl, cursor);

    impl.window->update();
}

void qt_gl_window::paintGL()
{
    opengl_surface* surface =
        static_cast<opengl_surface*>(impl.ui.surface.get());

    QSize size = impl.window->size();
    surface->initialize_render_state(
        alia::make_vector<unsigned>(size.width(), size.height()));

    render_ui(impl.ui);
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
        update_window(impl);
        return acknowledged;
      }
     case QEvent::KeyRelease:
      {
        QKeyEvent* e = static_cast<QKeyEvent*>(event);
        bool acknowledged = false;
        key_event_info info = get_key_event_info(*e);
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
    }
    return QGLWidget::event(event);
}

void qt_gl_window::timerEvent(QTimerEvent* event)
{
    if (process_timer_requests(impl.ui, get_time(impl)))
        update_window(impl);
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
    string const& title,
    alia__shared_ptr<ui_controller> const& controller,
    qt_window::state_data const& initial_state)
{
    impl->timer.start();

    QGLFormat format;
    format.setSwapInterval(0);
    impl->window = new qt_gl_window(*impl, format, 0, Qt::Window);

    opengl_surface* surface = new opengl_surface;
    surface->set_opengl_context(impl->gl_ctx);

    initialize_ui(
        impl->ui,
        controller,
        alia__shared_ptr<alia::surface>(surface),
        make_vector(
            float(impl->window->physicalDpiX()),
            float(impl->window->physicalDpiY())),
        alia__shared_ptr<os_interface>(new qt_os_interface),
        parse_style_file("alia.style"));

    impl->window->setWindowTitle(title.c_str());

    impl->window->setMouseTracking(true);

    if (initial_state.position)
    {
        impl->window->move(
            get(initial_state.position)[0],
            get(initial_state.position)[1]);
    }
    impl->window->resize(initial_state.size[0], initial_state.size[1]);

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

    QSize size = impl->window->size();
    update_ui(impl->ui,
        alia::make_vector<unsigned>(size.width(), size.height()),
        get_time(*impl));

    impl->window->startTimer(1);
}

void qt_window::initialize(
    string const& title, window_controller* controller,
    state_data const& initial_state)
{
    controller->window = this;
    impl_ = new qt_window::impl_data;
    create_window(impl_, title, alia__shared_ptr<ui_controller>(controller),
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

string qt_os_interface::get_clipboard_text()
{
    QClipboard* clipboard = QApplication::clipboard();
    QByteArray ba = clipboard->text().toUtf8();
    return string(ba.data());
}

void qt_os_interface::set_clipboard_text(string const& text)
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(text.c_str());
}

}
