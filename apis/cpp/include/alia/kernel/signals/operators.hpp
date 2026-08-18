#pragma once

#include <alia/kernel/actions/core.hpp>
#include <alia/kernel/signals/adaptors.hpp>
#include <alia/kernel/signals/application.hpp>
#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/core.hpp>

#include <type_traits>
#include <utility>

// This file defines operators for signals.

namespace alia {

// Given a signal to a structure, `structure->*field` yields a signal to that
// field. (`field` is a pointer-to-member.)
//
// The field signal's capabilities are the intersection of the structure's
// capabilities with a movable binding. Writes move (or copy) the structure,
// assign the field, and write the structure back.
//
// If the field type is `identifiable`, the value ID is the field value.
// Otherwise it is the structure ID paired with an ID for the field pointer.
//
template<class StructureSignal, class Field>
struct field_signal
    : signal<
          field_signal<StructureSignal, Field>,
          Field,
          signal_capabilities_intersection<
              typename StructureSignal::capabilities,
              binding_caps<signal_movable>>>
{
    using structure_type = typename StructureSignal::value_type;
    using field_ptr = Field structure_type::*;

    field_signal(StructureSignal structure, field_ptr field)
        : structure_(std::move(structure)), field_(field)
    {
    }
    bool
    has_value() const override
    {
        return structure_.has_value();
    }
    Field const&
    read() const override
    {
        return structure_.read().*field_;
    }
    Field
    move_out() const override
    {
        return std::move(structure_.destructive_ref().*field_);
    }
    Field&
    destructive_ref() const override
    {
        return structure_.destructive_ref().*field_;
    }
    id_view
    value_id() const override
    {
        if constexpr (identifiable<Field>)
        {
            if (!this->has_value())
                return null_id();
            return make_id_by_reference(this->read());
        }
        else
        {
            return make_id_pair(
                pair_, structure_.value_id(), make_id_by_reference(field_));
        }
    }
    bool
    ready_to_write() const override
    {
        return structure_.has_value() && structure_.ready_to_write();
    }
    id_view
    write(Field x) const override
    {
        structure_type s = forward_signal(alia::move(structure_));
        s.*field_ = std::move(x);
        return structure_.write(std::move(s));
    }

 private:
    StructureSignal structure_;
    field_ptr field_;
    mutable alia_id_pair pair_{};
};

template<view_signal StructureSignal, class Field>
field_signal<StructureSignal, Field>
operator->*(
    StructureSignal const& structure,
    Field StructureSignal::value_type::* field)
{
    return field_signal<StructureSignal, Field>(structure, field);
}

// `ALIA_FIELD(x, f)` is equivalent to `x->*T::f` where `T` is the value type
// of `x`.
#define ALIA_FIELD(x, f) ((x)->*&std::decay<decltype(read_signal(x))>::type::f)
#ifndef ALIA_STRICT_MACROS
#define alia_field(x, f) ALIA_FIELD(x, f)
#endif

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

// The || and && operators require special implementations because they follow
// the normal short-circuiting evaluation rules of C++.

template<class Arg0, class Arg1>
struct logical_or_signal
    : signal<logical_or_signal<Arg0, Arg1>, bool, view_caps<signal_readable>>
{
    logical_or_signal(Arg0 arg0, Arg1 arg1)
        : arg0_(std::move(arg0)), arg1_(std::move(arg1))
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
        // This has a value if both arguments have values, or if either
        // argument is true.
        return (arg0_.has_value() && arg1_.has_value())
            || (arg0_.has_value() && arg0_.read())
            || (arg1_.has_value() && arg1_.read());
    }
    bool const&
    read() const override
    {
        value_ = (arg0_.has_value() && arg0_.read())
              || (arg1_.has_value() && arg1_.read());
        return value_;
    }

 private:
    Arg0 arg0_;
    Arg1 arg1_;
    mutable alia_id_pair pair_{};
    mutable bool value_;
};

template<view_signal A, view_signal B>
auto
operator||(A const& a, B const& b)
{
    return logical_or_signal<A, B>(a, b);
}

template<view_signal A, class B>
    requires(!signal_type<B>)
auto
operator||(A const& a, B const& b)
{
    return a || value(b);
}

template<class A, view_signal B>
    requires(!signal_type<A> && !action_type<A>)
auto
operator||(A const& a, B const& b)
{
    return value(a) || b;
}

template<class Arg0, class Arg1>
struct logical_and_signal
    : signal<logical_and_signal<Arg0, Arg1>, bool, view_caps<signal_readable>>
{
    logical_and_signal(Arg0 arg0, Arg1 arg1)
        : arg0_(std::move(arg0)), arg1_(std::move(arg1))
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
        // This has a value if both arguments have values, or if either
        // argument is false.
        return (arg0_.has_value() && arg1_.has_value())
            || (arg0_.has_value() && !arg0_.read())
            || (arg1_.has_value() && !arg1_.read());
    }
    bool const&
    read() const override
    {
        value_
            = !((arg0_.has_value() && !arg0_.read())
                || (arg1_.has_value() && !arg1_.read()));
        return value_;
    }

 private:
    Arg0 arg0_;
    Arg1 arg1_;
    mutable alia_id_pair pair_{};
    mutable bool value_;
};

template<view_signal A, view_signal B>
auto
operator&&(A const& a, B const& b)
{
    return logical_and_signal<A, B>(a, b);
}

template<view_signal A, class B>
    requires(!signal_type<B>)
auto
operator&&(A const& a, B const& b)
{
    return a && value(b);
}

template<class A, view_signal B>
    requires(!signal_type<A> && !action_type<A>)
auto
operator&&(A const& a, B const& b)
{
    return value(a) && b;
}

// `conditional(b, t, f)`, where `b`, `t`, and `f` are signals, yields `t` if
// `b`'s value is true and `f` if `b`'s value is false.
//
// `t` and `f` must have the same value type, and `b`'s value type must be
// testable in a boolean context.
//
// This is a normal function call, so unlike an if statement or the ternary
// operator, both `t` and `f` are constructed. (But the a signal's value will
// only be touched if it is selected by the condition.)
//
template<class Condition, class T, class F>
struct signal_mux
    : signal<
          signal_mux<Condition, T, F>,
          typename T::value_type,
          signal_capabilities_intersection<
              typename T::capabilities,
              typename F::capabilities>>
{
    signal_mux(Condition condition, T t, F f)
        : condition_(std::move(condition)), t_(std::move(t)), f_(std::move(f))
    {
    }
    bool
    has_value() const override
    {
        return condition_.has_value()
            && (condition_.read() ? t_.has_value() : f_.has_value());
    }
    typename T::value_type const&
    read() const override
    {
        return condition_.read() ? t_.read() : f_.read();
    }
    typename T::value_type
    move_out() const override
    {
        return condition_.read() ? t_.move_out() : f_.move_out();
    }
    typename T::value_type&
    destructive_ref() const override
    {
        return condition_.read() ? t_.destructive_ref() : f_.destructive_ref();
    }
    id_view
    value_id() const override
    {
        if (!condition_.has_value())
            return null_id();
        return make_id_pair(
            pair_,
            make_id(condition_.read() ? true : false),
            condition_.read() ? t_.value_id() : f_.value_id());
    }
    bool
    ready_to_write() const override
    {
        return condition_.has_value()
            && (condition_.read() ? t_.ready_to_write() : f_.ready_to_write());
    }
    id_view
    write(typename T::value_type value) const override
    {
        if (condition_.read())
            return t_.write(std::move(value));
        return f_.write(std::move(value));
    }
    void
    clear() const override
    {
        if (condition_.read())
            t_.clear();
        else
            f_.clear();
    }
    bool
    invalidate(std::exception_ptr error) const override
    {
        if (condition_.read())
            return t_.invalidate(error);
        return f_.invalidate(error);
    }
    bool
    is_invalidated() const override
    {
        if (condition_.read())
            return t_.is_invalidated();
        return f_.is_invalidated();
    }

 private:
    Condition condition_;
    T t_;
    F f_;
    mutable alia_id_pair pair_{};
};

template<class Condition, class T, class F>
auto
conditional(Condition condition, T t, F f)
{
    auto condition_signal = signalize(std::move(condition));
    auto t_signal = signalize(std::move(t));
    auto f_signal = signalize(std::move(f));
    return signal_mux<
        decltype(condition_signal),
        decltype(t_signal),
        decltype(f_signal)>(
        std::move(condition_signal), std::move(t_signal), std::move(f_signal));
}

} // namespace alia
