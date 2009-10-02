#ifndef ALIA_WX_MAIN_HPP
#define ALIA_WX_MAIN_HPP

// This file should be included exactly once, by the main UI source file for
// the application.

//#ifdef NDEBUG
//#define ALIA_WX_NDEBUG
//#else
//#define NDEBUG
//#endif
#include <wx/glcanvas.h>
//#ifndef ALIA_WX_NDEBUG
//#undef NDEBUG
//#endif

namespace alia { namespace wx {

// This is the entry point for the application.  It should initialize the
// application and create the initial application windows.
void on_init();

class application : public wxGLApp
{
 public:
    bool OnInit();
    int OnRun();

 private:
    int return_code_;
};

}}

#ifndef ALIA_WX_NO_IMPLEMENT
IMPLEMENT_APP(alia::wx::application)
#endif

#endif
