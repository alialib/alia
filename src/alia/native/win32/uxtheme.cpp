#ifdef WIN32

#include <alia/native/win32/uxtheme.hpp>

namespace alia { namespace native {

#define LOAD_FUNCTION(lib, f) lib.load_function(f, #f)

theme_dll::theme_dll()
  : lib("uxtheme.dll")
{
    LOAD_FUNCTION(lib, OpenThemeData);
    LOAD_FUNCTION(lib, CloseThemeData);
    LOAD_FUNCTION(lib, DrawThemeBackground);
    LOAD_FUNCTION(lib, DrawThemeText);
    LOAD_FUNCTION(lib, GetThemeBackgroundContentRect);
    LOAD_FUNCTION(lib, GetThemePartSize);
    LOAD_FUNCTION(lib, GetThemeColor);
    LOAD_FUNCTION(lib, GetThemeFont);
    LOAD_FUNCTION(lib, GetThemeSysColor);
    LOAD_FUNCTION(lib, GetThemeSysFont);
    LOAD_FUNCTION(lib, GetCurrentThemeName);
}

}}

#endif
