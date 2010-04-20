#include <alia/wx/window.hpp>
#include <alia/wx/manager.hpp>
#include <alia/wx/callback.hpp>
#include <alia/wx/menu.hpp>
#include <alia/wx/context_holder.hpp>
#include <alia/wx/impl.hpp>
#include <alia/menu/spec_differ.hpp>
#include <alia/menu/spec_builder.hpp>
#include <alia/menu/selection_dispatcher.hpp>
#include <boost/bind.hpp>

namespace alia { namespace wx {

namespace impl {

class wx_frame_wrapper;

class frame
  : public top_level_window
  , public context_holder
{
 public:
    wx::window* wrapper;
    wx_frame_wrapper* window;
    alia::menu_spec menu_spec;
    bool initially_maximized;
    bool initially_full_screen;
    bool resizable;
    bool is_open;
    point2i initial_position;
    std::string title;

    frame(controller* controller);
    ~frame();

    void create();

    // implementation of top_level_window/context_holder...
    void update();
    void close();
    void resize(vector2i const& new_size);
    void adjust_font_sizes();
    void update_color_scheme();

    void handle_menu(int id);
};

class wx_frame_wrapper : public wxFrame
{
 public:
    wx_frame_wrapper(frame* owner, wxWindow* parent, wxWindowID id,
        const wxString& title, const wxPoint& pos, const wxSize& size,
        long style)
      : wxFrame(parent, id, title, pos, size, style)
      , owner(owner)
    {}
    ~wx_frame_wrapper()
    {
        if (owner)
        {
            window_controller* wc =
                static_cast<window_controller*>(owner->context.controller);
            wc->on_close();
            owner->window = 0;
            // TODO: maybe manage window ownership differently
            delete owner->wrapper;
        }
    }
    void on_menu(wxCommandEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&frame::handle_menu, owner, event.GetId()));
    }
    frame* owner;
 private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wx_frame_wrapper, wxFrame)
    EVT_MENU(-1, wx_frame_wrapper::on_menu)
END_EVENT_TABLE()

frame::frame(controller* controller)
  : context_holder(controller)
{
    is_open = false;
    window = 0;
    resizable = true;
    initially_maximized = false;
    initially_full_screen = false;
    initial_position = point2i(-1, -1);

    manager::get_instance().register_(this);
}

frame::~frame()
{
    if (window)
    {
        window_controller* wc =
            static_cast<window_controller*>(context.controller);
        wc->on_close();
        window->Show(false);
        window->Destroy();
        window->owner = 0;
        window = 0;
    }

    manager::get_instance().unregister(this);
}

void frame::create()
{
    window = new wx_frame_wrapper(
        this, 0, -1,
        wxString(title.c_str()),
        wxPoint(initial_position[0], initial_position[1]),
        wxSize(initial_size[0], initial_size[1]),
        (wxDEFAULT_FRAME_STYLE & ~wxRESIZE_BORDER) |
            (resizable ? wxRESIZE_BORDER : 0));

    context_holder::create(window);

    if (initially_maximized)
        window->Maximize();

    window->SetMinSize(wxSize(minimum_size[0], minimum_size[1]));

    update();

    window->Show(true);
    if (initially_full_screen)
        window->ShowFullScreen(true);
}

void frame::close()
{
    if (window)
    {
        window->Show(false);
        window->Close();
    }
}

void frame::update()
{
    if (!window)
        return;

    context_holder::update();

    // Check to see if the menu bar has changed, and if so, recreate it.
    bool is_different;
    window_controller* wc =
        static_cast<window_controller*>(context.controller);
    {
    menu_spec_differ differ(menu_spec);
    wc->do_menu_bar(differ);
    differ.end();
    is_different = differ.is_different();
    }
    if (is_different)
    {
        {
        menu_spec = alia::menu_spec();
        menu_spec_builder builder(&menu_spec);
        wc->do_menu_bar(builder);
        }
        window->SetMenuBar(build_wx_menu_bar(menu_spec));
        fix_wx_menu_bar(window->GetMenuBar(), menu_spec);
    }
}

void frame::handle_menu(int id)
{
    window_controller* wc =
        static_cast<window_controller*>(context.controller);
    menu_selection_dispatcher dispatcher(id);
    wc->do_menu_bar(dispatcher);
    manager::get_instance().update();
}

void frame::resize(vector2i const& new_size)
{ window->SetClientSize(new_size[0], new_size[1]); }

void frame::adjust_font_sizes()
{ context_holder::adjust_font_sizes(); }

void frame::update_color_scheme()
{ context_holder::update_color_scheme(); }

}

window::window(std::string const& title, window_controller* controller)
{
    impl_ = new impl::frame(controller);
    impl_->wrapper = this;
    impl_->title = title;
    controller->window = this;
}

window::~window()
{
    delete impl_;
}

point2i window::get_position() const
{
    if (impl_->window)
    {
        point2i p;
        impl_->window->GetPosition(&p[0], &p[1]);
        return p;
    }
    else
        return impl_->initial_position;
}
void window::set_position(point2i const& position)
{
    if (impl_->window)
        impl_->window->Move(position[0], position[1]);
    else
        impl_->initial_position = position;
}

vector2i window::get_size() const
{
    if (impl_->window)
    {
        vector2i s;
        impl_->window->GetClientSize(&s[0], &s[1]);
        return s;
    }
    else
        return impl_->initial_size;
}
void window::set_size(vector2i const& new_size)
{
    if (impl_->window)
        impl_->window->SetClientSize(new_size[0], new_size[1]);
    else
        impl_->initial_size = new_size;
}

bool window::is_maximized() const
{
    return impl_->window ? impl_->window->IsMaximized() :
        impl_->initially_maximized;
}
void window::set_maximized(bool maximized)
{
    if (impl_->window)
        impl_->window->Maximize(maximized);
    else
        impl_->initially_maximized = maximized;
}

bool window::is_fit_to_contents() const
{
    return impl_->fit_to_contents;
}
void window::set_fit_to_contents(bool fit_to_contents)
{
    impl_->fit_to_contents = fit_to_contents;
}

bool window::is_resizable() const
{
    return impl_->resizable;
}
void window::set_resizable(bool resizable)
{
    impl_->resizable = resizable;
}

vector2i const& window::get_minimum_size() const
{
    return impl_->minimum_size;
}
void window::set_minimum_size(vector2i const& minimum_size)
{
    impl_->minimum_size = minimum_size;
}

vector2i const& window::get_maximum_size() const
{
    return impl_->maximum_size;
}
void window::set_maximum_size(vector2i const& maximum_size)
{
    impl_->maximum_size = maximum_size;
}

void window::create()
{
    impl_->create();
}

bool window::is_open() const
{
    return impl_->window && impl_->is_open;
}
void window::close()
{
    impl_->close();
}

void* window::get_wx_window() const
{
    return impl_->window;
}

bool window::is_full_screen() const
{
    return impl_->window ? impl_->window->IsFullScreen() :
        impl_->initially_full_screen;
}
void window::set_full_screen(bool fs)
{
    if (impl_->window)
        impl_->window->ShowFullScreen(fs);
    else
        impl_->initially_full_screen = fs;
}

context& window::get_context() const
{
    return impl_->context;
}

void window::set_artist(alia::artist* artist)
{
    impl_->set_artist(artist);
}

}}
