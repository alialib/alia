#pragma once

#include <alia/impl/events.hpp>
#include <alia/kernel/signals/core.hpp>
#include <alia/kernel/signals/utilities.hpp>
#include <alia/kernel/substrate.hpp>

#include <exception>
#include <stdint.h>
#include <type_traits>
#include <utility>

// This file defines function application over signals.

namespace alia {

// `lazy_apply(f, args...)` yields a signal to the result of lazily applying
// `f` to the values of `args`. The value ID is taken from the inputs, meaning
// that the resulting signal is as lazy as possible (i.e., `f` is not run until
// the signal value is actually read).

template<class Result, class Function, class Arg>
struct lazy_apply1_signal
    : lazy_signal<
          lazy_apply1_signal<Result, Function, Arg>,
          Result,
          view_caps<signal_move_activated, Arg::capabilities::presence>,
          typename Arg::value_id_type>
{
    lazy_apply1_signal(Function f, Arg arg)
        : f_(std::move(f)), arg_(std::move(arg))
    {
    }
    decltype(auto)
    value_id() const
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
          view_caps<
              signal_move_activated,
              signal_capability_level_intersection<
                  Arg0::capabilities::presence,
                  Arg1::capabilities::presence>>,
          std::
              pair<typename Arg0::value_id_type, typename Arg1::value_id_type>>
{
    lazy_apply2_signal(Function f, Arg0 arg0, Arg1 arg1)
        : f_(std::move(f)), arg0_(std::move(arg0)), arg1_(std::move(arg1))
    {
    }
    std::pair<typename Arg0::value_id_type, typename Arg1::value_id_type>
    value_id() const
    {
        return {arg0_.value_id(), arg1_.value_id()};
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

// `uncached_apply(f, args...)` is like `lazy_apply`, but the value ID is the
// result itself. This is intended for cheap functions (arithmetic,
// comparisons) where an input-shaped ID is more costly than computing the
// result.

template<class Result, class Function, class Arg>
    requires identifiable<Result>
struct uncached_apply1_signal
    : lazy_signal<
          uncached_apply1_signal<Result, Function, Arg>,
          Result,
          view_caps<signal_move_activated, Arg::capabilities::presence>,
          Result>
{
    uncached_apply1_signal(Function f, Arg arg)
        : f_(std::move(f)), arg_(std::move(arg))
    {
    }
    Result
    value_id() const
    {
        return this->move_out();
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
    requires identifiable<decltype(std::declval<Function>()(
        forward_signal(std::declval<Arg>())))>
auto
uncached_apply(Function f, Arg arg)
{
    return uncached_apply1_signal<
        decltype(f(forward_signal(arg))),
        Function,
        Arg>(std::move(f), std::move(arg));
}

template<class Result, class Function, class Arg0, class Arg1>
    requires identifiable<Result>
struct uncached_apply2_signal
    : lazy_signal<
          uncached_apply2_signal<Result, Function, Arg0, Arg1>,
          Result,
          view_caps<
              signal_move_activated,
              signal_capability_level_intersection<
                  Arg0::capabilities::presence,
                  Arg1::capabilities::presence>>,
          Result>
{
    uncached_apply2_signal(Function f, Arg0 arg0, Arg1 arg1)
        : f_(std::move(f)), arg0_(std::move(arg0)), arg1_(std::move(arg1))
    {
    }
    Result
    value_id() const
    {
        return this->move_out();
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
};

template<class Function, view_signal Arg0, view_signal Arg1>
    requires identifiable<decltype(std::declval<Function>()(
        forward_signal(std::declval<Arg0>()),
        forward_signal(std::declval<Arg1>())))>
auto
uncached_apply(Function f, Arg0 arg0, Arg1 arg1)
{
    return uncached_apply2_signal<
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
          binding_caps<
              signal_move_activated,
              signal_writable,
              Arg::capabilities::presence>,
          typename Arg::value_id_type>
{
    lazy_bidirectional_apply_signal(Forward forward, Reverse reverse, Arg arg)
        : forward_(std::move(forward)),
          reverse_(std::move(reverse)),
          arg_(std::move(arg))
    {
    }
    decltype(auto)
    value_id() const
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
    void
    write(Result value) const override
    {
        arg_.write(reverse_(std::move(value)));
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

// `apply(ctx, f, args...)` eagerly applies `f` to the values of `args` and
// caches the result in the substrate. If any argument is empty, the result is
// also empty. The result is recomputed on refresh when any argument's value ID
// changes (or if an argument transitions to or from the empty state). The
// returned signal's ID is a simple revision counter.

namespace detail {

enum class apply_status
{
    // not computed since the last input change
    UNCOMPUTED,
    // the function threw an exception
    // TODO: Reconsider this behavior.
    FAILED,
    // up-to-date cached result
    READY,
    // result was moved out; recompute on the next refresh
    MOVED
};

template<class Value>
struct apply_result_data
{
    apply_status status = apply_status::UNCOMPUTED;
    // identity of the cached result - Incremented whenever inputs change.
    uint32_t version = 0;
    Value value{};
    std::exception_ptr error;
};

template<class Value>
void
reset_apply_result(apply_result_data<Value>& data)
{
    ++data.version;
    data.status = apply_status::UNCOMPUTED;
    data.error = nullptr;
}

template<class Result, class Arg>
void
process_apply_arg(
    alia_context* ctx,
    apply_result_data<Result>& data,
    bool& args_ready,
    Arg const& arg)
{
    auto cache = use_object<captured_id<typename Arg::value_id_type>>(ctx);
    if (!is_refresh_event(*ctx))
        return;

    if (!signal_has_value(arg))
    {
        reset_apply_result(data);
        args_ready = false;
        cache->clear(substrate_allocator(ctx));
        return;
    }

    auto const id = arg.value_id();
    if (cache->empty() || !cache->matches(id))
    {
        reset_apply_result(data);
        cache->capture(id, substrate_allocator(ctx));
    }
}

template<class Result>
void
process_apply_args(alia_context*, apply_result_data<Result>&, bool&)
{
}

template<class Result, class Arg, class... Rest>
void
process_apply_args(
    alia_context* ctx,
    apply_result_data<Result>& data,
    bool& args_ready,
    Arg const& arg,
    Rest const&... rest)
{
    process_apply_arg(ctx, data, args_ready, arg);
    process_apply_args(ctx, data, args_ready, rest...);
}

template<class Value, class Function, class... Args>
void
process_apply_body(
    alia_context* ctx,
    apply_result_data<Value>& data,
    bool args_ready,
    Function&& f,
    Args const&... args)
{
    if (!is_refresh_event(*ctx))
        return;

    if ((data.status == apply_status::UNCOMPUTED
         || data.status == apply_status::MOVED)
        && args_ready)
    {
        try
        {
            data.value = std::forward<Function>(f)(forward_signal(args)...);
            data.status = apply_status::READY;
            data.error = nullptr;
        }
        catch (...)
        {
            data.error = std::current_exception();
            data.status = apply_status::FAILED;
            // TODO: Surface or rethrow apply failures.
        }
    }
}

} // namespace detail

template<class Value>
struct apply_signal
    : signal<
          apply_signal<Value>,
          Value,
          view_caps<signal_move_activated>,
          uint32_t>
{
    explicit apply_signal(detail::apply_result_data<Value>& data)
        : data_(&data)
    {
    }
    uint32_t
    value_id() const
    {
        return data_->version;
    }
    bool
    has_value() const override
    {
        return data_->status == detail::apply_status::READY
            || data_->status == detail::apply_status::MOVED;
    }
    Value const&
    read() const override
    {
        return data_->value;
    }
    Value
    move_out() const override
    {
        auto moved_out = std::move(data_->value);
        data_->status = detail::apply_status::MOVED;
        return moved_out;
    }
    Value&
    destructive_ref() const override
    {
        data_->status = detail::apply_status::MOVED;
        return data_->value;
    }

 private:
    detail::apply_result_data<Value>* data_;
};

template<class Function, view_signal... Args>
auto
apply(alia_context* ctx, Function&& f, Args const&... args)
{
    using result_type = decltype(std::declval<Function>()(
        forward_signal(std::declval<Args>())...));
    auto data = use_object<detail::apply_result_data<result_type>>(ctx);
    bool args_ready = true;
    detail::process_apply_args(ctx, *data, args_ready, args...);
    detail::process_apply_body(
        ctx, *data, args_ready, std::forward<Function>(f), args...);
    return apply_signal<result_type>(*data);
}

// `lift(f)` yields a function that takes a context and signal arguments and
// returns `apply(ctx, f, args...)`.
template<class Function>
auto
lift(Function f)
{
    return [f = std::move(f)](alia_context* ctx, auto const&... args) {
        return apply(ctx, f, args...);
    };
}

} // namespace alia
