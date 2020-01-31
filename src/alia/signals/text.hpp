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

inline char const*
make_printf_friendly(std::string const& x)
{
    return x.c_str();
}

struct printf_format_error : exception
{
    printf_format_error() : exception("printf format error")
    {
    }
};

template<class... Args>
std::string
invoke_snprintf(std::string const& format, Args const&... args)
{
    int length
        = std::snprintf(0, 0, format.c_str(), make_printf_friendly(args)...);
    if (length < 0)
        throw printf_format_error();
    std::string s;
    if (length > 0)
    {
        s.resize(length);
        std::snprintf(
            &s[0], length + 1, format.c_str(), make_printf_friendly(args)...);
    }
    return s;
}

template<class... Args>
auto
printf(context ctx, readable<std::string> format, Args const&... args)
{
    return apply(ctx, ALIA_LAMBDIFY(invoke_snprintf), format, args...);
}

template<class... Args>
auto
printf(context ctx, char const* format, Args const&... args)
{
    return apply(ctx, ALIA_LAMBDIFY(invoke_snprintf), val(format), args...);
}

} // namespace alia

#endif
