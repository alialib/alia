#ifndef ALIA_SIGNALS_APPLICATION_HPP
#define ALIA_SIGNALS_APPLICATION_HPP

#include <alia/components/context.hpp>
#include <alia/flow/data_graph.hpp>
#include <alia/signals/utilities.hpp>

namespace alia {

// lazy_apply(f, args...), where :args are all signals, yields a signal
// to the result of lazily applying the function :f to the values of :args.

// Note that doing this in true variadic fashion is a little insane, so I'm
// just doing the two overloads I need for now...

template<class Result, class Function, class Arg>
struct lazy_apply1_signal : signal<
                                lazy_apply1_signal<Result, Function, Arg>,
                                Result,
                                read_only_signal>
{
    lazy_apply1_signal(Function const& f, Arg const& arg) : f_(f), arg_(arg)
    {
    }
    id_interface const&
    value_id() const
    {
        return arg_.value_id();
    }
    bool
    is_readable() const
    {
        return arg_.is_readable();
    }
    Result const&
    read() const
    {
        return lazy_reader_.read([&] { return f_(arg_.read()); });
    }

 private:
    Function f_;
    Arg arg_;
    lazy_reader<Result> lazy_reader_;
};
template<class Function, class Arg>
auto
lazy_apply(Function const& f, Arg const& arg)
{
    return lazy_apply1_signal<decltype(f(read_signal(arg))), Function, Arg>(
        f, arg);
}

template<class Result, class Function, class Arg0, class Arg1>
struct lazy_apply2_signal
    : signal<
          lazy_apply2_signal<Result, Function, Arg0, Arg1>,
          Result,
          read_only_signal>
{
    lazy_apply2_signal(Function const& f, Arg0 const& arg0, Arg1 const& arg1)
        : f_(f), arg0_(arg0), arg1_(arg1)
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = combine_ids(ref(arg0_.value_id()), ref(arg1_.value_id()));
        return id_;
    }
    bool
    is_readable() const
    {
        return arg0_.is_readable() && arg1_.is_readable();
    }
    Result const&
    read() const
    {
        return lazy_reader_.read(
            [&]() { return f_(arg0_.read(), arg1_.read()); });
    }

 private:
    Function f_;
    Arg0 arg0_;
    Arg1 arg1_;
    mutable id_pair<id_ref, id_ref> id_;
    lazy_reader<Result> lazy_reader_;
};
template<class Function, class Arg0, class Arg1>
auto
lazy_apply(Function const& f, Arg0 const& arg0, Arg1 const& arg1)
{
    return lazy_apply2_signal<
        decltype(f(read_signal(arg0), read_signal(arg1))),
        Function,
        Arg0,
        Arg1>(f, arg0, arg1);
}

template<class Function>
auto
lazy_lift(Function const& f)
{
    return [=](auto&&... args) { return lazy_apply(f, args...); };
}

// apply(ctx, f, args...), where :args are all signals, yields a signal to the
// result of applying the function :f to the values of :args. Unlike lazy_apply,
// this is eager and caches and the result.

enum class apply_status
{
    UNCOMPUTED,
    READY,
    FAILED
};

template<class Value>
struct apply_result_data
{
    counter_type result_version = 0;
    Value result;
    apply_status status = apply_status::UNCOMPUTED;
};

template<class Value>
void
reset(apply_result_data<Value>& data)
{
    if (data.status != apply_status::UNCOMPUTED)
    {
        ++data.result_version;
        data.status = apply_status::UNCOMPUTED;
    }
}

template<class Value>
struct apply_signal : signal<apply_signal<Value>, Value, read_only_signal>
{
    apply_signal(apply_result_data<Value>& data) : data_(&data)
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = make_id(data_->result_version);
        return id_;
    }
    bool
    is_readable() const
    {
        return data_->status == apply_status::READY;
    }
    Value const&
    read() const
    {
        return data_->result;
    }

 private:
    apply_result_data<Value>* data_;
    mutable simple_id<counter_type> id_;
};

template<class Value>
apply_signal<Value>
make_apply_signal(apply_result_data<Value>& data)
{
    return apply_signal<Value>(data);
}

template<class Result>
void
process_apply_args(
    context ctx, apply_result_data<Result>& data, bool& args_ready)
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
    if (!signal_is_readable(arg))
    {
        reset(data);
        args_ready = false;
    }
    else if (!cached_id->matches(arg.value_id()))
    {
        reset(data);
        cached_id->capture(arg.value_id());
    }
    process_apply_args(ctx, data, args_ready, rest...);
}

template<class Function, class... Args>
auto
apply(context ctx, Function const& f, Args const&... args)
{
    apply_result_data<decltype(f(read_signal(args)...))>* data_ptr;
    get_cached_data(ctx, &data_ptr);
    auto& data = *data_ptr;
    if (is_refresh_pass(ctx))
    {
        bool args_ready = true;
        process_apply_args(ctx, data, args_ready, args...);
        if (data.status == apply_status::UNCOMPUTED && args_ready)
        {
            try
            {
                data.result = f(read_signal(args)...);
                data.status = apply_status::READY;
            }
            catch (...)
            {
                data.status = apply_status::FAILED;
            }
        }
    }
    return make_apply_signal(data);
}

template<class Function>
auto
lift(context ctx, Function const& f)
{
    return [=](auto&&... args) { return apply(ctx, f, args...); };
}

// alia_method(m) wraps a method name in a lambda so that it can be passed as a
// function object.
#define ALIA_METHOD(m) [](auto&& x, auto&&... args) { return x.m(args...); }
#ifdef ALIA_LOWERCASE_MACROS
#define alia_method(m) ALIA_METHOD(m)
#endif

} // namespace alia

#endif
