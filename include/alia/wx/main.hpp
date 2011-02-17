#ifndef ALIA_WX_MAIN_HPP
#define ALIA_WX_MAIN_HPP

// This file should be included exactly once, by the main UI source file for
// the application.

#ifdef _M_X64
    #ifdef NDEBUG
        #define ALIA_WX_NDEBUG
    #else
        #define NDEBUG
    #endif
#endif

#include <wx/glcanvas.h>

#ifdef _M_X64
    #ifndef ALIA_WX_NDEBUG
        #undef NDEBUG
    #endif
#endif

namespace alia { namespace wx {

// This is the entry point for the application.  It should initialize the
// application and create the initial application windows.
void on_init();

class application : public wxGLApp
{
 public:
    application();
    bool OnInit();
    int OnRun();
    int OnExit();

 private:
    int return_code_;
};

}}

#ifndef ALIA_WX_NO_IMPLEMENT
IMPLEMENT_APP(alia::wx::application)
#endif

#endif
