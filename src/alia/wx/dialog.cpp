#include <alia/wx/dialog.hpp>
#include <alia/wx/manager.hpp>
#include <alia/wx/callback.hpp>
#include <alia/wx/menu.hpp>
#include <alia/wx/context_holder.hpp>
#include <alia/wx/impl.hpp>
#include <boost/bind.hpp>

namespace alia { namespace wx {

namespace impl {

class wx_dialog_wrapper;

class dialog
  : public top_level_window
  , public context_holder
{
 public:
    wx::top_level_window* parent;
    wx::dialog* wrapper;
    wx_dialog_wrapper* window;
    bool resizable;
    bool is_open;
    point2i initial_position;
    std::string title;

    dialog(controller* controller);
    ~dialog();

    void create();
    void show_modal();

    // implementation of top_level_window/context_holder...
    void update();
    void close();
    void resize(vector2i const& new_size);
    void adjust_font_sizes();
    void update_color_scheme();
};

class wx_dialog_wrapper
  : public wxDialog
{
 public:
    wx_dialog_wrapper(dialog* owner, wxWindow* parent, wxWindowID id,
        const wxString& title, const wxPoint& pos, const wxSize& size,
        long style)
      : wxDialog(parent, id, title, pos, size, style)
      , owner(owner)
    {}
    ~wx_dialog_wrapper()
    {
        if (owner)
        {
            owner->window = 0;
            // TODO: maybe manage window ownership differently
            delete owner->wrapper;
            owner = 0;
        }
    }

    // provide the dialog class with access to IsEscapeKey()
    bool is_escape_key(wxKeyEvent const& event)
    { return IsEscapeKey(event); }

    dialog* owner;

 private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wx_dialog_wrapper, wxDialog)
END_EVENT_TABLE()

dialog::dialog(controller* controller)
  : context_holder(controller)
{
    is_open = false;
    window = 0;
    resizable = true;
    initial_position = point2i(-1, -1);

    manager::get_instance().register_(this);
}

dialog::~dialog()
{
    if (window)
    {
        window->Show(false);
        window->Destroy();
        window->owner = 0;
        window = 0;
    }

    manager::get_instance().unregister(this);
}

void dialog::create()
{
    window = new wx_dialog_wrapper(
        this,
        reinterpret_cast<wxWindow*>(
            parent ? parent->get_wx_window() : wxTheApp->GetTopWindow()),
        -1,
        wxString(title.c_str()),
        wxPoint(initial_position[0], initial_position[1]),
        wxSize(initial_size[0], initial_size[1]),
        ((title.empty() ? 0 : wxDEFAULT_DIALOG_STYLE) & ~wxRESIZE_BORDER) |
            (resizable ? wxRESIZE_BORDER : 0) | wxWANTS_CHARS);

    context_holder::create(window);

    window->SetMinSize(wxSize(minimum_size[0], minimum_size[1]));

    update();

    //if (impl_->initial_focus_id_.empty())
    //    get_canvas().set_focus(initial_focus_id_);
    //else
    //    get_canvas().set_initial_focus();

    window->Show(true);
}

void dialog::close()
{
    if (window)
    {
        if (window->IsModal())
        {
            window->EndModal(0);
        }
        else
        {
            window->Show(false);
            window->Close();
        }
    }
}

void dialog::update()
{
    if (!window)
        return;
    context_holder::update();
}

void dialog::resize(vector2i const& new_size)
{ window->SetClientSize(new_size[0], new_size[1]); }

void dialog::adjust_font_sizes()
{ context_holder::adjust_font_sizes(); }

void dialog::update_color_scheme()
{ context_holder::update_color_scheme(); }

void dialog::show_modal()
{
    if (!window)
        create();

    manager::get_instance().push_modal(this);

    is_open = true;
    window->ShowModal();
}

}

dialog::dialog(std::string const& title, controller* controller,
    top_level_window* parent)
{
    impl_ = new impl::dialog(controller);
    impl_->wrapper = this;
    impl_->title = title;
    impl_->parent = parent;
}
dialog::~dialog()
{
    delete impl_;
}

point2i dialog::get_position() const
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
void dialog::set_posiiton(point2i const& position)
{
    if (impl_->window)
        impl_->window->Move(position[0], position[1]);
    else
        impl_->initial_position = position;
}

vector2i dialog::get_size() const
{
    if (impl_->window)
    {
        vector2i s;
        impl_->window->GetPosition(&s[0], &s[1]);
        return s;
    }
    else
        return impl_->initial_size;
}
void dialog::set_size(vector2i const& new_size)
{
    if (impl_->window)
        impl_->window->SetSize(new_size[0], new_size[1]);
    else
        impl_->initial_size = new_size;
}

bool dialog::is_fit_to_contents() const
{
    return impl_->fit_to_contents;
}
void dialog::set_fit_to_contents(bool fit_to_contents)
{
    impl_->fit_to_contents = fit_to_contents;
}

bool dialog::is_resizable() const
{
    return impl_->resizable;
}
void dialog::set_resizable(bool resizable)
{
    impl_->resizable = resizable;
}

vector2i const& dialog::get_minimum_size() const
{
    return impl_->minimum_size;
}
void dialog::set_minimum_size(vector2i const& minimum_size)
{
    impl_->minimum_size = minimum_size;
}

vector2i const& dialog::get_maximum_size() const
{
    return impl_->maximum_size;
}
void dialog::set_maximum_size(vector2i const& maximum_size)
{
    impl_->maximum_size = maximum_size;
}

void dialog::create()
{
    impl_->create();
}

bool dialog::is_open() const
{
    return impl_->window && impl_->is_open;
}
void dialog::close()
{
    impl_->close();
}

void dialog::show_modal()
{
    impl_->show_modal();
}

void* dialog::get_wx_window() const
{
    return impl_->window;
}

//bool dialog::process_key(wxKeyEvent& event)
//{
//    if (impl_->window_->is_escape_key(event))
//    {
//        close(-1);
//        return true;
//    }
//    else
//        return false;
//}

}}
