#pragma once

#include <alia/kernel/actions/core.hpp>
#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/core.hpp>
#include <alia/kernel/signals/operators.hpp>

#include <utility>

namespace alia {

// comma operator
//
// Using the comma operator between two actions creates a combined action that
// performs the two actions in sequence.

template<class First, class Second, class Interface>
struct action_pair;

template<class First, class Second, class... Args>
struct action_pair<First, Second, action_interface<Args...>>
    : action_interface<Args...>
{
    action_pair(First first, Second second)
        : first_(std::move(first)), second_(std::move(second))
    {
    }

    bool
    is_ready() const override
    {
        return first_.is_ready() && second_.is_ready();
    }

    void
    perform(
        function_view<void()> const& intermediary, Args... args) const override
    {
        second_.perform(
            [&]() { first_.perform(intermediary, args...); }, args...);
    }

 private:
    First first_;
    Second second_;
};

template<action_type First, action_type Second>
auto
operator,(First first, Second second)
{
    return action_pair<First, Second, typename First::action_type>(
        std::move(first), std::move(second));
}

// operator <<
//
// (a << s), where a is an action and s is a readable signal, returns another
// action that is like :a but with the value of :s bound to its first argument.

template<class Action, class Signal, class Interface>
struct bound_action;
template<class Action, class Signal, class BoundArg, class... Args>
struct bound_action<Action, Signal, action_interface<BoundArg, Args...>>
    : action_interface<Args...>
{
    bound_action(Action action, Signal signal)
        : action_(std::move(action)), signal_(std::move(signal))
    {
    }

    bool
    is_ready() const override
    {
        return action_.is_ready() && signal_.has_value();
    }

    void
    perform(
        function_view<void()> const& intermediary, Args... args) const override
    {
        action_.perform(
            intermediary, forward_signal(signal_), std::move(args)...);
    }

 private:
    Action action_;
    Signal signal_;
};

template<action_type Action, view_signal Signal>
auto
operator<<(Action action, Signal signal)
{
    return bound_action<Action, Signal, typename Action::action_type>(
        std::move(action), std::move(signal));
}

template<action_type Action, class Value>
    requires(!signal_type<Value>)
auto
operator<<(Action action, Value v)
{
    return std::move(action) << value(std::move(v));
}

// operator <<=
//
// sink <<= source, where :sink and :source are both signals, creates an
// action that will set the value of :sink to the value held in :source. In
// order for the action to be considered ready, :source must have a value and
// :sink must be ready to write.

template<class Sink, class Source>
struct copy_action : action_interface<>
{
    copy_action(Sink sink, Source source)
        : sink_(std::move(sink)), source_(std::move(source))
    {
    }

    bool
    is_ready() const override
    {
        return source_.has_value() && sink_.ready_to_write();
    }

    void
    perform(function_view<void()> const& intermediary) const override
    {
        typename Source::value_type source_value = forward_signal(source_);
        intermediary();
        sink_.write(std::move(source_value));
    }

 private:
    Sink sink_;
    Source source_;
};

template<sink_signal Sink, view_signal Source>
auto
operator<<=(Sink sink, Source source)
{
    return copy_action<Sink, Source>(std::move(sink), std::move(source));
}

template<sink_signal Sink, class Source>
    requires(!signal_type<Source>)
auto
operator<<=(Sink sink, Source source)
{
    return sink <<= value(source);
}

// For most compound assignment operators (e.g., `+=`), `a += b`, where `a` and
// `b` are signals, creates an action that sets `a` equal to `a + b`.

#define ALIA_DEFINE_COMPOUND_ASSIGNMENT_OPERATOR(                             \
    assignment_form, normal_form)                                             \
    template<binding_signal A, view_signal B>                                 \
    auto operator assignment_form(A const& a, B const& b)                     \
    {                                                                         \
        return a <<= (a normal_form b);                                       \
    }

ALIA_DEFINE_COMPOUND_ASSIGNMENT_OPERATOR(+=, +)
ALIA_DEFINE_COMPOUND_ASSIGNMENT_OPERATOR(-=, -)
ALIA_DEFINE_COMPOUND_ASSIGNMENT_OPERATOR(*=, *)
ALIA_DEFINE_COMPOUND_ASSIGNMENT_OPERATOR(/=, /)
ALIA_DEFINE_COMPOUND_ASSIGNMENT_OPERATOR(^=, ^)
ALIA_DEFINE_COMPOUND_ASSIGNMENT_OPERATOR(%=, %)
ALIA_DEFINE_COMPOUND_ASSIGNMENT_OPERATOR(&=, &)
ALIA_DEFINE_COMPOUND_ASSIGNMENT_OPERATOR(|=, |)

#undef ALIA_DEFINE_COMPOUND_ASSIGNMENT_OPERATOR

#define ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(                     \
    assignment_form, normal_form)                                             \
    template<binding_signal A, class B>                                       \
        requires(!signal_type<B>)                                             \
    auto operator assignment_form(A const& a, B const& b)                     \
    {                                                                         \
        return a <<= (a normal_form value(b));                                \
    }

ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(+=, +)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(-=, -)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(*=, *)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(/=, /)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(^=, ^)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(%=, %)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(&=, &)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(|=, |)

#undef ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR

#define ALIA_DEFINE_BY_ONE_OPERATOR(assignment_form, normal_form)             \
    template<binding_signal A>                                                \
    auto operator assignment_form(A const& a)                                 \
    {                                                                         \
        return a <<= (a normal_form value(typename A::value_type(1)));        \
    }                                                                         \
    template<binding_signal A>                                                \
    auto operator assignment_form(A const& a, int)                            \
    {                                                                         \
        return a <<= (a normal_form value(typename A::value_type(1)));        \
    }

ALIA_DEFINE_BY_ONE_OPERATOR(++, +)
ALIA_DEFINE_BY_ONE_OPERATOR(--, -)

#undef ALIA_DEFINE_BY_ONE_OPERATOR

} // namespace alia
