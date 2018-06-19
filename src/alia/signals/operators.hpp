#ifndef ALIA_SIGNALS_OPERATORS_HPP
#define ALIA_SIGNALS_OPERATORS_HPP

#include <alia/signals/application.hpp>
#include <alia/signals/utilities.hpp>

// This file defines the operators for signals.

namespace alia {

#define ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(op)                                 \
    template<                                                                  \
        class A,                                                               \
        class B,                                                               \
        std::enable_if_t<                                                      \
            is_signal_type<A>::value && is_signal_type<B>::value,              \
            int> = 0>                                                          \
    auto operator op(A const& a, B const& b)                                   \
    {                                                                          \
        return lazy_apply([](auto a, auto b) { return a op b; }, a, b);        \
    }

ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(+)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(-)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(*)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(/)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR (^)
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

#define ALIA_DEFINE_UNARY_SIGNAL_OPERATOR(op)                                  \
    template<class A, std::enable_if_t<is_signal_type<A>::value, int> = 0>     \
    auto operator op(A const& a)                                               \
    {                                                                          \
        return lazy_apply([](auto a) { return op a; }, a);                     \
    }

ALIA_DEFINE_UNARY_SIGNAL_OPERATOR(-)
ALIA_DEFINE_UNARY_SIGNAL_OPERATOR(!)

#undef ALIA_DEFINE_UNARY_SIGNAL_OPERATOR

// The || and && operators require special implementations because they don't
// necessarily need to evaluate both of their arguments...

template<class Arg0, class Arg1>
struct logical_or_signal : signal<bool, read_only_signal>
{
    logical_or_signal()
    {
    }
    logical_or_signal(Arg0 const& arg0, Arg1 const& arg1)
        : arg0_(arg0), arg1_(arg1)
    {
    }
    id_interface const&
    id() const
    {
        id_ = combine_ids(ref(arg0_.id()), ref(arg1_.id()));
        return id_;
    }
    bool
    is_readable() const
    {
        // Obviously, this is readable if both of its arguments are readable.
        // However, it's also readable if only one is readable and its value is
        // true.
        return arg0_.is_readable() && arg1_.is_readable()
               || arg0_.is_readable() && arg0_.read()
               || arg1_.is_readable() && arg1_.read();
    }
    bool const&
    read() const
    {
        value_ = arg0_.is_readable() && arg0_.read()
                 || arg1_.is_readable() && arg1_.read();
        return value_;
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(bool const& value) const
    {
    }

 private:
    Arg0 arg0_;
    Arg1 arg1_;
    mutable id_pair<id_ref, id_ref> id_;
    mutable bool value_;
};
template<
    class A,
    class B,
    std::enable_if_t<
        std::is_base_of<untyped_signal_base, A>::value
            && std::is_base_of<untyped_signal_base, B>::value,
        int> = 0>
auto
operator||(A const& a, B const& b)
{
    return logical_or_signal<A, B>(a, b);
}

template<class Arg0, class Arg1>
struct logical_and_signal : signal<bool, read_only_signal>
{
    logical_and_signal()
    {
    }
    logical_and_signal(Arg0 const& arg0, Arg1 const& arg1)
        : arg0_(arg0), arg1_(arg1)
    {
    }
    id_interface const&
    id() const
    {
        id_ = combine_ids(ref(arg0_.id()), ref(arg1_.id()));
        return id_;
    }
    bool
    is_readable() const
    {
        // Obviously, this is readable if both of its arguments are readable.
        // However, it's also readable if only one is readable and its value is
        // false.
        return arg0_.is_readable() && arg1_.is_readable()
               || arg0_.is_readable() && !arg0_.read()
               || arg1_.is_readable() && !arg1_.read();
    }
    bool const&
    read() const
    {
        value_
            = !(arg0_.is_readable() && !arg0_.read()
                || arg1_.is_readable() && !arg1_.read());
        return value_;
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(bool const& value) const
    {
    }

 private:
    Arg0 arg0_;
    Arg1 arg1_;
    mutable id_pair<id_ref, id_ref> id_;
    mutable bool value_;
};
template<
    class A,
    class B,
    std::enable_if_t<
        std::is_base_of<untyped_signal_base, A>::value
            && std::is_base_of<untyped_signal_base, B>::value,
        int> = 0>
auto
operator&&(A const& a, B const& b)
{
    return logical_and_signal<A, B>(a, b);
}

// This is the equivalent of the ternary operator for signals.
// select_signal(condition, t, f), where condition, t and f are signals,
// yields t if condition's value is true and f if condition's value is false.
// Note that this is a normal function call, so, unlike an if statement or the
// ternary operator, both t and f are fully evaluated. However, they are only
// accessed if they're selected.
// t and f must have the same value type, and condition's value type must be
// testable in a boolean context.
template<class Condition, class T, class F>
struct signal_mux : signal<
                        typename T::value_type,
                        typename signal_direction_intersection<
                            typename T::direction_tag,
                            typename F::direction_tag>::type>
{
    signal_mux()
    {
    }
    signal_mux(Condition condition, T t, F f)
        : condition_(condition), t_(t), f_(f)
    {
    }
    bool
    is_readable() const
    {
        return condition_.is_readable() && condition_.read() ? t_.is_readable()
                                                             : f_.is_readable();
    }
    typename T::value_type const&
    read() const
    {
        return condition_.read() ? t_.read() : f_.read();
    }
    id_interface const&
    value_id() const
    {
        return condition_.read() ? t_.value_id() : f_.value_id();
    }
    bool
    is_writable() const
    {
        return condition_.is_readable() && condition_.read() ? t_.is_writable()
                                                             : f_.is_writable();
    }
    void
    write(typename T::value_type const& value) const
    {
        if (condition_.read())
            t_.write(value);
        else
            f_.write(value);
    }

 private:
    Condition condition_;
    T t_;
    F f_;
};
template<class Condition, class T, class F>
signal_mux<Condition, T, F>
select_signal(Condition const& condition, T const& t, F const& f)
{
    return signal_mux<Condition, T, F>(condition, t, f);
}

} // namespace alia

#endif
