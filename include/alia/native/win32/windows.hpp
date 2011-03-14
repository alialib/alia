#ifndef ALIA_WIN32_WINDOWS_HPP
#define ALIA_WIN32_WINDOWS_HPP

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
  #define NOMINMAX
#endif
#include <windows.h>

#ifdef KEY_EXECUTE
  #undef KEY_EXECUTE
#endif
#ifdef DOUBLE_CLICK
  #undef DOUBLE_CLICK
#endif

#ifdef ABSOLUTE
    #undef ABSOLUTE
#endif
#ifdef RELATIVE
    #undef RELATIVE
#endif

#endif
