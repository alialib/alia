#define ALIA_WX_NO_IMPLEMENT
#include <alia/wx/main.hpp>
#include <alia/wx/manager.hpp>
#include <alia/wx/wx.hpp>
#include <exception>

namespace alia { namespace wx {

application::application()
{
    int attribs[] = { WX_GL_DOUBLEBUFFER, 0 };
    if (!InitGLVisual(attribs))
    {
        wxMessageBox("OpenGL not available");
        return_code_ = -1;
    }
    else
        return_code_ = 0;
}

bool application::OnInit()
{
    try
    {
        if (!return_code_)
            on_init();
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

int application::OnExit()
{
    return wxGLApp::OnExit();
}

}}
