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
        return make_id_pair(pair_, arg0_.value_id(), arg1_.value_id());
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

// `lazy_bidirectional_apply(forward, reverse, arg)`, where `arg` is a
// bidirectional signal, yields another bidirectional signal with a value
// mapping in both directions. The resulting signal's value is the result of
// applying `forward` to the value of `arg`, and writing to the that signal
// applies `reverse` and writes the result back to `arg`.
// The applications in both directions are done lazily, on demand.
template<class Result, class Forward, class Reverse, class Arg>
struct lazy_bidirectional_apply_signal
    : lazy_signal<
          lazy_bidirectional_apply_signal<Result, Forward, Reverse, Arg>,
          Result,
          binding_caps<signal_move_activated>>
{
    lazy_bidirectional_apply_signal(Forward forward, Reverse reverse, Arg arg)
        : forward_(std::move(forward)),
          reverse_(std::move(reverse)),
          arg_(std::move(arg))
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
        return forward_(forward_signal(arg_));
    }
    bool
    ready_to_write() const override
    {
        return arg_.ready_to_write();
    }
    id_view
    write(Result value) const override
    {
        return arg_.write(reverse_(std::move(value)));
    }

 private:
    Forward forward_;
    Reverse reverse_;
    Arg arg_;
};

template<class Forward, class Reverse, binding_signal Arg>
auto
lazy_bidirectional_apply(Forward forward, Reverse reverse, Arg arg)
{
    return lazy_bidirectional_apply_signal<
        decltype(forward(forward_signal(arg))),
        Forward,
        Reverse,
        Arg>(std::move(forward), std::move(reverse), std::move(arg));
}

} // namespace alia
