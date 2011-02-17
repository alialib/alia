#ifndef ALIA_WX_HPP
#define ALIA_WX_HPP

#ifdef _M_X64
    #ifdef NDEBUG
        #define ALIA_WX_NDEBUG
    #else
        #define NDEBUG
    #endif
#endif

#include <wx/wx.h>
#include <wx/glcanvas.h>

#ifdef _MSC_VER

//#ifdef NDEBUG
//
//#pragma comment (lib, "wxbase28.lib")
//#pragma comment (lib, "wxmsw28_core.lib")
//#pragma comment (lib, "wxmsw28_gl.lib")
//#pragma comment (lib, "wxmsw28_aui.lib")
//
//#else
//
//#pragma comment (lib, "wxbase28d.lib")
//#pragma comment (lib, "wxmsw28d_core.lib")
//#pragma comment (lib, "wxmsw28d_gl.lib")
//#pragma comment (lib, "wxmsw28d_aui.lib")
//
//#endif

//#ifdef _M_X64
//#pragma comment (lib, "wxbase28.lib")
//#pragma comment (lib, "wxmsw28_core.lib")
//#pragma comment (lib, "wxmsw28_gl.lib")
//#pragma comment (lib, "wxmsw28_aui.lib")
//#endif

#pragma comment (lib, "uuid.lib")
#pragma comment (lib, "comctl32.lib")
#pragma comment (lib, "wsock32.lib")
#pragma comment (lib, "rpcrt4.lib")
#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "comdlg32.lib")
#pragma comment (lib, "advapi32.lib")
#pragma comment (lib, "ole32.lib")
#pragma comment (lib, "shell32.lib")
#pragma comment (lib, "oleaut32.lib")

#endif

#ifdef WIN32

// Used to clean up some of the mess left behind by the Windows header files.
#include <alia/native/win32/windows.hpp>

#endif

#ifdef _M_X64
    #ifndef ALIA_WX_NDEBUG
        #undef NDEBUG
    #endif
#endif

#endif
