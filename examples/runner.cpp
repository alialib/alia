// #include <alia/ui/backends/glfw.hpp>
// #include <alia/ui/backends/sdl.hpp>
// #include <alia/ui/backends/emscripten.hpp>
#include <alia/ui/backends/wx.hpp>

#include <alia/ui/context.hpp>

#include <alia/ui/backends/wx.hpp>
#include <wx/glcanvas.h>
#include <wx/msgdlg.h>

using namespace alia;

void
my_ui(ui_context ctx);

struct application : public wxGLApp
{
    application();

    bool
    OnInit();
    int
    OnRun();

 private:
    int return_code_;
};

application::application()
{
    int attribs[] = {WX_GL_DOUBLEBUFFER, 0};
    if (!this->InitGLVisual(attribs))
    {
        wxMessageBox("OpenGL not available");
        return_code_ = -1;
    }
    else
        return_code_ = 0;
}

bool
application::OnInit()
{
    try
    {
        // style_tree_ptr style = parse_style_file("alia.style");

        // alia__shared_ptr<app_window_controller> controller_ptr(new
        // controller);

        create_wx_framed_window(
            "alia test",
            my_ui,
            app_window_state{std::nullopt, make_vector<int>(850, 1000)});
    }
    catch (std::exception& e)
    {
        wxMessageBox(
            std::string("An error occurred during application"
                        " initialization.\n\n")
            + e.what());
        return_code_ = -1;
    }
    catch (...)
    {
        wxMessageBox(
            "An unknown error occurred during application"
            " initialization.");
        return_code_ = -1;
    }
    return true;
}

int
application::OnRun()
{
    if (return_code_ != 0)
    {
        ExitMainLoop();
        return return_code_;
    }

    return wxGLApp::OnRun();
}

wxIMPLEMENT_APP(application);

// int
// main(void)
// {
//     // sdl_window the_window(
//     //     "alia test", make_vector<unsigned>(900, 1200), my_ui);
//     // the_window.do_main_loop();

//     glfw_window the_window(
//         "alia test", make_vector<unsigned>(900, 1200), my_ui);
//     the_window.do_main_loop();

//     // emscripten_canvas the_canvas(my_ui);
//     // the_canvas.do_main_loop();
// }
