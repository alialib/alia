#define CATCH_CONFIG_MAIN

// Ask Catch to dump memory leaks under Windows.
#ifdef _WIN32
#define CATCH_CONFIG_WINDOWS_CRTDBG
#endif

// Disable coloring because it doesn't seem to work properly on Windows.
#define CATCH_CONFIG_COLOUR_NONE

// Allowing Catch to support nullptr causes duplicate definitions for some
// things.
#define CATCH_CONFIG_CPP11_NO_NULLPTR

// The Catch "main" code triggers these in Visual C++.
#if defined(_MSC_VER)
#pragma warning(disable : 4244)
#pragma warning(disable : 4702)
#endif

#include <catch2/catch.hpp>
