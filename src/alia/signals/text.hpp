#ifndef ALIA_SIGNALS_TEXT_HPP
#define ALIA_SIGNALS_TEXT_HPP

#include <alia/signals/application.hpp>
#include <alia/signals/basic.hpp>

#include <cstdio>

namespace alia {

// The following implements a very minimal C++-friendly version of printf that
// works with signals.

template<class Value>
Value
make_printf_friendly(Value x)
{
    return x;
}

static inline char const*
make_printf_friendly(std::string const& x)
{
    return x.c_str();
}

template<class... Args>
std::string
invoke_snprintf(std::string const& format, Args const&... args)
{
    int length = snprintf(0, 0, format.c_str(), make_printf_friendly(args)...);
    if (length < 0)
        throw "printf format error";
    std::string s;
    if (length > 0)
    {
        s.resize(length);
        snprintf(
            &s[0], length + 1, format.c_str(), make_printf_friendly(args)...);
    }
    return s;
}

template<class... Args>
auto
printf(context ctx, input<std::string> format, Args const&... args)
{
    return apply(ctx, ALIA_LAMBDIFY(invoke_snprintf), format, args...);
}

template<class... Args>
auto
printf(context ctx, char const* format, Args const&... args)
{
    return apply(ctx, ALIA_LAMBDIFY(invoke_snprintf), value(format), args...);
}

} // namespace alia

#endif
