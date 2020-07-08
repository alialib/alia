// When using Catch, warnings seem to inevitably creep in about parentheses.
#ifdef __clang__
#pragma clang diagnostic ignored "-Wlogical-op-parentheses"
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

#ifndef ALIA_LOWERCASE_MACROS
#define ALIA_STRICT_MACROS
#endif

#include <catch2/catch.hpp>
