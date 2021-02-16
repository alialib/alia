#ifndef ALIA_SIGNALS_APPLICATION_HPP
#define ALIA_SIGNALS_APPLICATION_HPP

#include <alia/context/interface.hpp>
#include <alia/flow/data_graph.hpp>
#include <alia/flow/events.hpp>
#include <alia/signals/utilities.hpp>

namespace alia {

// lazy_apply(f, args...), where :args are all signals, yields a signal
// to the result of lazily applying the function :f to the values of :args.

// Note that doing this in true variadic fashion is a little insane, so I'm
// just doing the two overloads I need for now...

template<class Result, class Function, class Arg>
struct lazy_apply1_signal : lazy_signal<
                                lazy_apply1_signal<Result, Function, Arg>,
                                Result,
                                move_activated_signal>
{
    lazy_apply1_signal(Function f, Arg arg)
        : f_(std::move(f)), arg_(std::move(arg))
    {
    }
    id_interface const&
    value_id() const
    {
        return arg_.value_id();
    }
    bool
    has_value() const
    {
        return arg_.has_value();
    }
    Result
    move_out() const
    {
        return f_(forward_signal(arg_));
    }

 private:
    Function f_;
    Arg arg_;
};
template<class Function, class Arg>
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
          move_activated_signal>
{
    lazy_apply2_signal(Function f, Arg0 arg0, Arg1 arg1)
        : f_(std::move(f)), arg0_(std::move(arg0)), arg1_(std::move(arg1))
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = combine_ids(ref(arg0_.value_id()), ref(arg1_.value_id()));
        return id_;
    }
    bool
    has_value() const
    {
        return arg0_.has_value() && arg1_.has_value();
    }
    Result
    move_out() const
    {
        return f_(forward_signal(arg0_), forward_signal(arg1_));
    }

 private:
    Function f_;
    Arg0 arg0_;
    Arg1 arg1_;
    mutable id_pair<id_ref, id_ref> id_;
};
template<class Function, class Arg0, class Arg1>
auto
lazy_apply(Function f, Arg0 arg0, Arg1 arg1)
{
    return lazy_apply2_signal<
        decltype(f(forward_signal(arg0), forward_signal(arg1))),
        Function,
        Arg0,
        Arg1>(f, std::move(arg0), std::move(arg1));
}

template<class Function>
auto
lazy_lift(Function f)
{
    return [=](auto... args) { return lazy_apply(f, std::move(args)...); };
}

// duplex_lazy_apply(forward, reverse, arg), where :arg is a duplex signal,
// yields another duplex signal whose value is the result of applying :forward
// to the value of :arg. Writing to the resulting signal applies :reverse and
// writes the result to :arg.
// The applications in both directions are done lazily, on demand.

namespace detail {

template<class Result, class Forward, class Reverse, class Arg>
struct lazy_duplex_apply_signal
    : lazy_signal<
          lazy_duplex_apply_signal<Result, Forward, Reverse, Arg>,
          Result,
          move_activated_duplex_signal>
{
    lazy_duplex_apply_signal(Forward forward, Reverse reverse, Arg arg)
        : forward_(std::move(forward)),
          reverse_(std::move(reverse)),
          arg_(std::move(arg))
    {
    }
    id_interface const&
    value_id() const
    {
        return arg_.value_id();
    }
    bool
    has_value() const
    {
        return arg_.has_value();
    }
    Result
    move_out() const
    {
        return forward_(forward_signal(arg_));
    }
    bool
    ready_to_write() const
    {
        return arg_.ready_to_write();
    }
    void
    write(Result value) const
    {
        arg_.write(reverse_(std::move(value)));
    }

 private:
    Forward forward_;
    Reverse reverse_;
    Arg arg_;
};

} // namespace detail

template<class Forward, class Reverse, class Arg>
auto
lazy_duplex_apply(Forward forward, Reverse reverse, Arg arg)
{
    return detail::lazy_duplex_apply_signal<
        decltype(forward(forward_signal(arg))),
        Forward,
        Reverse,
        Arg>(std::move(forward), std::move(reverse), std::move(arg));
}

// apply(ctx, f, args...), where :args are all signals, yields a signal to the
// result of applying the function :f to the values of :args. Unlike
// lazy_apply, this is eager and caches and the result.

namespace detail {

enum class apply_status
{
    UNCOMPUTED,
    READY,
    FAILED
};

template<class Value>
struct apply_result_data
{
    apply_status status = apply_status::UNCOMPUTED;
    // This is used to identify the result of the apply. It's incremented every
    // time the inputs change.
    counter_type version = 0;
    // If status is READY, this is the result.
    Value value;
    // If status is FAILED, this is the error.
    std::exception_ptr error;
};

template<class Value>
void
reset(apply_result_data<Value>& data)
{
    ++data.version;
    data.status = apply_status::UNCOMPUTED;
}

} // namespace detail

template<class Value>
struct apply_signal
    : signal<apply_signal<Value>, Value, movable_read_only_signal>
{
    apply_signal(detail::apply_result_data<Value>& data) : data_(&data)
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = make_id(data_->version);
        return id_;
    }
    bool
    has_value() const
    {
        return data_->status == detail::apply_status::READY;
    }
    Value const&
    read() const
    {
        return data_->value;
    }
    Value
    move_out() const
    {
        auto moved_out = std::move(data_->value);
        data_->status = detail::apply_status::UNCOMPUTED;
        return moved_out;
    }
    Value&
    destructive_ref() const
    {
        data_->status = detail::apply_status::UNCOMPUTED;
        return data_->value;
    }

 private:
    detail::apply_result_data<Value>* data_;
    mutable simple_id<counter_type> id_;
};

namespace detail {

template<class Value>
apply_signal<Value>
make_apply_signal(apply_result_data<Value>& data)
{
    return apply_signal<Value>(data);
}

template<class Result, class Arg>
void
process_apply_arg(
    context ctx,
    apply_result_data<Result>& data,
    bool& args_ready,
    captured_id& cached_id,
    Arg const& arg)
{
    if (is_refresh_event(ctx))
    {
        if (!signal_has_value(arg))
        {
            reset(data);
            args_ready = false;
        }
        else if (!cached_id.matches(arg.value_id()))
        {
            reset(data);
            cached_id.capture(arg.value_id());
        }
    }
}

template<class Result>
void
process_apply_args(context, apply_result_data<Result>&, bool&)
{
}
template<class Result, class Arg, class... Rest>
void
process_apply_args(
    context ctx,
    apply_result_data<Result>& data,
    bool& args_ready,
    Arg const& arg,
    Rest const&... rest)
{
    captured_id* cached_id;
    get_cached_data(ctx, &cached_id);
    process_apply_arg(ctx, data, args_ready, *cached_id, arg);
    process_apply_args(ctx, data, args_ready, rest...);
}

template<class Value, class Function, class... Args>
void
process_apply_body(
    context ctx,
    apply_result_data<Value>& data,
    bool args_ready,
    Function&& f,
    Args const&... args)
{
    if (is_refresh_event(ctx))
    {
        if (data.status == apply_status::UNCOMPUTED && args_ready)
        {
            try
            {
                data.value
                    = std::forward<Function>(f)(forward_signal(args)...);
                data.status = apply_status::READY;
            }
            catch (...)
            {
                data.error = std::current_exception();
                data.status = apply_status::FAILED;
            }
        }
        if (data.status == apply_status::FAILED)
            std::rethrow_exception(data.error);
    }
}

} // namespace detail

template<class Function, class... Args>
auto
apply(context ctx, Function&& f, Args const&... args)
{
    detail::apply_result_data<decltype(f(forward_signal(args)...))>* data_ptr;
    get_cached_data(ctx, &data_ptr);
    auto& data = *data_ptr;
    bool args_ready = true;
    process_apply_args(ctx, data, args_ready, args...);
    process_apply_body(
        ctx, data, args_ready, std::forward<Function>(f), args...);
    return detail::make_apply_signal(data);
}

template<class Function>
auto
lift(Function f)
{
    return [=](context ctx, auto&&... args) {
        return apply(ctx, std::move(f), std::move(args)...);
    };
}

// duplex_apply(ctx, forward, reverse, arg), where :arg is a duplex signal,
// yields another duplex signal whose value is the result of applying :forward
// to the value of :arg. Writing to the resulting signal applies :reverse and
// writes the result to :arg.
// Similar to apply(ctx, f, arg), this is eager and caches the forward result.

namespace detail {

template<class Value>
struct duplex_apply_data
{
    apply_result_data<Value> result;
    captured_id input_id;
};

template<class Value, class Input, class Reverse>
struct duplex_apply_signal : signal<
                                 duplex_apply_signal<Value, Input, Reverse>,
                                 Value,
                                 movable_duplex_signal>
{
    duplex_apply_signal(
        duplex_apply_data<Value>& data, Input input, Reverse reverse)
        : data_(&data), input_(std::move(input)), reverse_(std::move(reverse))
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = make_id(data_->result.version);
        return id_;
    }
    bool
    has_value() const
    {
        return data_->result.status == apply_status::READY;
    }
    Value const&
    read() const
    {
        return data_->result.value;
    }
    Value
    move_out() const
    {
        auto moved_out = std::move(data_->result.value);
        data_->result.status = apply_status::UNCOMPUTED;
        return moved_out;
    }
    Value&
    destructive_ref() const
    {
        data_->result.status = apply_status::UNCOMPUTED;
        return data_->result.value;
    }
    bool
    ready_to_write() const
    {
        return input_.ready_to_write();
    }
    void
    write(Value value) const
    {
        input_.write(reverse_(value));
        // This is sort of hackish, but the idea here is that if we do nothing
        // right now, on the next refresh, we're going to detect that the input
        // signal has changed, and then apply the forward mapping to convert
        // that value back to the one we're writing right now, which is
        // obviously wasted effort.
        // To attempt to avoid this, we capture the value ID of the input after
        // writing to it in the hopes that it has already changed. (That is the
        // case for some signal types, but not all.)
        // To do this properly, we should probably allow signal write()
        // functions to return a new value ID.
        data_->input_id.capture(input_.value_id());
        ++data_->result.version;
        data_->result.value = std::move(value);
        data_->result.status = apply_status::READY;
    }

 private:
    duplex_apply_data<Value>* data_;
    mutable simple_id<counter_type> id_;
    Input input_;
    Reverse reverse_;
};
template<class Value, class Input, class Reverse>
auto
make_duplex_apply_signal(
    duplex_apply_data<Value>& data, Input input, Reverse reverse)
{
    return duplex_apply_signal<Value, Input, Reverse>(
        data, std::move(input), std::move(reverse));
}

} // namespace detail

template<class Forward, class Reverse, class Arg>
auto
duplex_apply(context ctx, Forward&& forward, Reverse reverse, Arg arg)
{
    detail::duplex_apply_data<decltype(forward(forward_signal(arg)))>*
        data_ptr;
    get_cached_data(ctx, &data_ptr);
    auto& data = *data_ptr;
    bool args_ready = true;
    process_apply_arg(ctx, data.result, args_ready, data.input_id, arg);
    process_apply_body(
        ctx, data.result, args_ready, std::forward<Forward>(forward), arg);
    return detail::make_duplex_apply_signal(
        data, std::move(arg), std::move(reverse));
}

// alia_mem_fn(m) wraps a member function name in a lambda so that it can be
// passed as a function object. (It's the equivalent of std::mem_fn, but
// there's no need to provide the type name.)
#define ALIA_MEM_FN(m)                                                        \
    [](auto&& x, auto&&... args) {                                            \
        return x.m(std::forward<decltype(args)>(args)...);                    \
    }
#ifndef ALIA_STRICT_MACROS
#define alia_mem_fn(m) ALIA_MEM_FN(m)
#endif

} // namespace alia

#endif
