#pragma once

#include <alia/kernel/signals/core.hpp>
#include <alia/kernel/signals/utilities.hpp>

#include <utility>

// This file defines function application over signals.

namespace alia {

// `lazy_apply(f, args...)` yields a signal to the result of lazily applying
// `f` to the values of `args`.

template<class Result, class Function, class Arg>
struct lazy_apply1_signal
    : lazy_signal<
          lazy_apply1_signal<Result, Function, Arg>,
          Result,
          view_caps<signal_move_activated>>
{
    lazy_apply1_signal(Function f, Arg arg)
        : f_(std::move(f)), arg_(std::move(arg))
    {
    }
    id_view
    value_id() const override
    {
        return arg_.value_id();
    }
    bool
    has_value() const override
    {
        return arg_.has_value();
    }
    Result
    move_out() const override
    {
        return f_(forward_signal(arg_));
    }

 private:
    Function f_;
    Arg arg_;
};

template<class Function, view_signal Arg>
auto
lazy_apply(Function f, Arg arg)
{
    return lazy_apply1_signal<decltype(f(forward_signal(arg))), Function, Arg>(
        std::move(f), std::move(arg));
}

template<class Result, class Function, class Arg0, class Arg1>
struct lazy_apply2_signal
    : lazy_signal<
          lazy_apply2_signal<Result, Function, Arg0, Arg1>,
          Result,
          view_caps<signal_move_activated>>
{
    lazy_apply2_signal(Function f, Arg0 arg0, Arg1 arg1)
        : f_(std::move(f)), arg0_(std::move(arg0)), arg1_(std::move(arg1))
    {
    }
    id_view
    value_id() const override
    {
        pair_ = {arg0_.value_id(), arg1_.value_id()};
        return alia_id_view_make_pair(&pair_, pair_.left, pair_.right);
    }
    bool
    has_value() const override
    {
        return arg0_.has_value() && arg1_.has_value();
    }
    Result
    move_out() const override
    {
        return f_(forward_signal(arg0_), forward_signal(arg1_));
    }

 private:
    Function f_;
    Arg0 arg0_;
    Arg1 arg1_;
    mutable alia_id_pair pair_{};
};

template<class Function, view_signal Arg0, view_signal Arg1>
auto
lazy_apply(Function f, Arg0 arg0, Arg1 arg1)
{
    return lazy_apply2_signal<
        decltype(f(forward_signal(arg0), forward_signal(arg1))),
        Function,
        Arg0,
        Arg1>(std::move(f), std::move(arg0), std::move(arg1));
}

} // namespace alia
