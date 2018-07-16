#ifndef ALIA_SIGNALS_APPLICATION_HPP
#define ALIA_SIGNALS_APPLICATION_HPP

#include <alia/signals/utilities.hpp>

namespace alia {

// lazy_apply(f, args...), where :args are all signals, yields a signal
// to the result of lazily applying the function :f to the values of :args.

// Note that doing this in true variadic fashion is a little insane, so I'm
// just doing the two overloads I need for now...

template<class Result, class Function, class Arg>
struct lazy_apply1_signal : signal<Result, read_only_signal>
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
struct lazy_apply2_signal : signal<Result, read_only_signal>
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

// alia_method(m) wraps a method name in a lambda so that it can be passed as a
// function object.
#define ALIA_METHOD(m) [](auto const& x) { return x.m(); }
#ifdef ALIA_LOWERCASE_MACROS
#define alia_method(m) ALIA_METHOD(m)
#endif

} // namespace alia

#endif
