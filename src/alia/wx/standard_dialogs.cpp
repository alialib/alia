#include <alia/wx/standard_dialogs.hpp>
#include <alia/wx/wx.hpp>
#include <wx/colordlg.h>
#include <wx/msgdlg.h>
//#include <alia/standard_dialogs.hpp>
//#include <alia/standard_buttons.hpp>

namespace alia { namespace wx {

rgb8 rgb8_from_wx(wxColour const& c)
{
    return rgb8(c.Red(), c.Green(), c.Blue());
}

wxColour rgb8_to_wx(rgb8 const& c)
{
    return wxColour(c.r, c.g, c.b);
}

bool ask_for_color(rgb8* result, rgb8 const* initial)
{
    wxColourDialog dialog(wxTheApp->GetTopWindow());
    if (initial)
        dialog.GetColourData().SetColour(rgb8_to_wx(*initial));
    dialog.GetColourData().SetChooseFull(true);
    dialog.Centre();
    if (dialog.ShowModal() == wxID_OK)
    {
        *result = rgb8_from_wx(dialog.GetColourData().GetColour());
        return true;
    }
    else
        return false;
}

//class message_dialog_controller : public controller
//{
// public:
//    message_dialog_controller(std::string const& message,
//        artist::standard_icon icon)
//      : message(message), icon(icon)
//    {}
//
//    message_dialog_controller(std::string const& message)
//      : message(message)
//    {}
//
//    void do_pass(canvas& canvas)
//    {
//        do_message_dialog(canvas, message, icon ?
//            canvas.artist().get_icon(icon.get()) : NULL);
//    }
//
// private:
//    std::string const& message;
//    boost::optional<artist::standard_icon> icon;
//};
//
//void show_message(std::string const& message)
//{
//    dialog dlg(_title = "Message",
//        _controller = new message_dialog_controller(message),
//        _initial_size = size(300, 200));
//    dlg.set_initial_focus(ok_button_id);
//    dlg.show_modal();
//}
//
//void show_warning(std::string const& message)
//{
//    dialog dlg(_title = "Warning",
//        _controller = new message_dialog_controller(message,
//            artist::WARNING_ICON),
//        _initial_size = size(300, 200));
//    dlg.set_initial_focus(ok_button_id);
//    dlg.show_modal();
//}
//
//void show_error(std::string const& message)
//{
//    dialog dlg(_title = "Error",
//        _controller = new message_dialog_controller(message,
//            artist::ERROR_ICON),
//        _initial_size = size(300, 200));
//    dlg.set_initial_focus(ok_button_id);
//    dlg.show_modal();
//}

void show_error(std::string const& message)
{
    wxMessageBox(message.c_str());
}

}}
