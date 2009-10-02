#define ALIA_WX_NO_IMPLEMENT
#include <alia/wx/main.hpp>
#include <alia/wx/manager.hpp>
#include <alia/wx/wx.hpp>
#include <exception>
//#include <alia/generic_text_renderer.hpp>

namespace alia { namespace wx {

bool application::OnInit()
{
    try
    {
        int attribs[] = { WX_GL_DOUBLEBUFFER, 0 };
        InitGLVisual(attribs);

        //alia::generic_text_renderer::set_font_dir(
        //    boost::filesystem::path(wxTheApp->argv[0],
        //    boost::filesystem::native).branch_path() / "fonts");

        on_init();
        return_code_ = 0;
    }
    catch (std::exception& e)
    {
        wxMessageBox(std::string("An error occurred during application"
            " initialization.\n\n") + e.what());
        return_code_ = -1;
    }
    catch (...)
    {
        wxMessageBox("An unknown error occurred during application"
            " initialization.");
        return_code_ = -1;
    }
    return true;
}

int application::OnRun()
{
    if (return_code_ == 0 && manager::get_instance().get_window_count() > 0)
        return wxGLApp::OnRun();
    else
        return return_code_;
}

}}
