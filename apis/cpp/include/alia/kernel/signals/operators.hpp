#pragma once

#include <alia/kernel/actions/core.hpp>
#include <alia/kernel/signals/application.hpp>
#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/core.hpp>

// This file defines arithmetic, comparison, and bitwise operators for signals.

namespace alia {

#define ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(op)                                \
    template<view_signal A, view_signal B>                                    \
    auto operator op(A const& a, B const& b)                                  \
    {                                                                         \
        return lazy_apply([](auto a, auto b) { return a op b; }, a, b);       \
    }

ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(+)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(-)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(*)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(/)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(^)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(%)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(&)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(|)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(<<)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(>>)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(==)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(!=)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(<)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(<=)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(>)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(>=)

#undef ALIA_DEFINE_BINARY_SIGNAL_OPERATOR

#define ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(op)                        \
    template<view_signal A, class B>                                          \
        requires(!signal_type<B>)                                             \
    auto operator op(A const& a, B const& b)                                  \
    {                                                                         \
        return lazy_apply(                                                    \
            [](auto a, auto b) { return a op b; }, a, value(b));              \
    }                                                                         \
    template<class A, view_signal B>                                          \
        requires(!signal_type<A> && !action_type<A>)                          \
    auto operator op(A const& a, B const& b)                                  \
    {                                                                         \
        return lazy_apply(                                                    \
            [](auto a, auto b) { return a op b; }, value(a), b);              \
    }

ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(+)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(-)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(*)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(/)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(^)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(%)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(&)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(|)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(<<)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(>>)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(==)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(!=)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(<)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(<=)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(>)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(>=)

#undef ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR

#define ALIA_DEFINE_UNARY_SIGNAL_OPERATOR(op)                                 \
    template<view_signal A>                                                   \
    auto operator op(A const& a)                                              \
    {                                                                         \
        return lazy_apply([](auto a) { return op a; }, a);                    \
    }

ALIA_DEFINE_UNARY_SIGNAL_OPERATOR(-)
ALIA_DEFINE_UNARY_SIGNAL_OPERATOR(!)
ALIA_DEFINE_UNARY_SIGNAL_OPERATOR(*)

#undef ALIA_DEFINE_UNARY_SIGNAL_OPERATOR

} // namespace alia
