#include <alia/wx/popup_window.hpp>
#include <alia/wx/manager.hpp>
#include <alia/wx/callback.hpp>
#include <alia/exception.hpp>
#include <boost/bind.hpp>
#include <wx/popupwin.h>
#include <wx/evtloop.h>

namespace alia { namespace wx {

struct popup_window::impl_data
{
    wx_popup_wrapper* window;
    bool open, was_dismissed;
    opengl_surface* parent;
    point2i initial_position, boundary;
};

class popup_window::wx_popup_wrapper
  : public wxPopupTransientWindow
{
 public:
    wx_popup_wrapper(popup_window* owner, wxWindow *parent, int style)
      : wxPopupTransientWindow(parent, style)
      , owner(owner)
    {}
    ~wx_popup_wrapper()
    {
        if (owner)
            owner->impl_->window = 0;
    }
    void on_size(wxSizeEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&popup_window::handle_resize, owner));
    }
    void OnDismiss()
    {
        invoke_callback_without_update(
            boost::bind(&popup_window::handle_dismiss, owner));
    }
    void on_key_down(wxKeyEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_key_down,
                owner->surface.get(), &event));
    }
    void on_key_up(wxKeyEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_key_up,
                owner->surface.get(), &event));
    }
    void on_char(wxKeyEvent& event)
    {
        invoke_callback_without_update(
            boost::bind(&opengl_surface::handle_char,
                owner->surface.get(), &event));
    }
    popup_window* owner;
 private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(popup_window::wx_popup_wrapper, wxPopupTransientWindow)
    EVT_SIZE(popup_window::wx_popup_wrapper::on_size)
    EVT_KEY_DOWN(popup_window::wx_popup_wrapper::on_key_down)
    EVT_KEY_UP(popup_window::wx_popup_wrapper::on_key_up)
    EVT_CHAR(popup_window::wx_popup_wrapper::on_char)
END_EVENT_TABLE()

popup_window::popup_window(opengl_surface* parent, controller* controller,
    point2i const& initial_position, point2i const& boundary,
    vector2i const& minimum_size, vector2i const& maximum_size)
  : context_holder(controller)
{
    impl_ = new impl_data;
    impl_->window = 0;
    impl_->open = false;
    impl_->was_dismissed = false;
    impl_->parent = parent;
    impl_->initial_position = initial_position;
    impl_->boundary = boundary;
    this->minimum_size = minimum_size;
    vector2i client_to_screen;
    parent->get_wx_window()->GetScreenPosition(
        &client_to_screen[0], &client_to_screen[1]);
    for (int i = 0; i < 2; ++i)
    {
        if (maximum_size[i] < 0)
        {
            this->maximum_size[i] = (std::max)(
                parent->get_display_size()[i] -
                    (initial_position[i] + client_to_screen[i]),
                (boundary[i] + client_to_screen[i]));
        }
        else
            this->maximum_size[i] = maximum_size[i];
    }
    this->fit_to_contents = true;
}

popup_window::~popup_window()
{
    if (impl_->window)
    {
        impl_->window->owner = 0;
        impl_->window->Show(false);
        impl_->window->Destroy();
    }
    if (impl_->parent)
        impl_->parent->remove_popup(this);
    delete impl_;
}

void popup_window::open()
{
    if (!impl_->window)
    {
        impl_->window = new wx_popup_wrapper(this,
            impl_->parent->get_wx_window(), wxNO_BORDER);

        context_holder::create(impl_->window);

        point2i position = impl_->initial_position, boundary = impl_->boundary;
        vector2i client_to_screen;
        impl_->parent->get_wx_window()->GetScreenPosition(
            &client_to_screen[0], &client_to_screen[1]);
        position += client_to_screen;
        boundary += client_to_screen;
        vector2i display_size = impl_->parent->get_display_size();
        vector2i const& size = initial_size;
        for (int i = 0; i < 2; ++i)
        {
            if (position[i] + size[i] > display_size[i] &&
                impl_->boundary[i] >= 0 &&
                boundary[i] > display_size[i] / 2 &&
                boundary[i] < display_size[i])
            {
                position[i] = boundary[i] - size[i];
            }
        }
        impl_->window->Move(wxPoint(position[0], position[1]),
	    wxSIZE_NO_ADJUSTMENTS);

        //canvas().process_input_event(canvas::FOCUS_GAIN);
        //if (canvas().no_id_has_focus())
        //    canvas().set_initial_focus();
    }

    impl_->open = true;
    impl_->window->Popup(impl_->parent->get_wx_window());
}

void popup_window::close()
{
    assert(impl_->window);
    impl_->window->Dismiss();
    handle_dismiss();
}

void popup_window::update()
{
    if (!impl_->window)
        open();
    context_holder::update();
}

void popup_window::orphan()
{
    impl_->parent = 0;
}

bool popup_window::was_dismissed() const
{
    return impl_->was_dismissed;
}

void popup_window::handle_resize()
{
    context_holder::surface->get_wx_window()->SetSize(
        impl_->window->GetClientSize());
}

void popup_window::handle_dismiss()
{
    impl_->open = false;
    impl_->was_dismissed = true;
}

void popup_window::resize(vector2i const& new_size)
{
    impl_->window->SetClientSize(new_size[0], new_size[1]);
}

bool popup_window::is_open() const
{
    return impl_->open;
}

}}
