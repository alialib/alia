// When using Catch, warnings seem to inevitably creep in about parentheses.
#ifdef __clang__
#pragma clang diagnostic ignored "-Wlogical-op-parentheses"
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

#include <catch.hpp>
