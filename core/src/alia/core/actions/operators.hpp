#ifndef ALIA_CORE_ACTIONS_OPERATORS_HPP
#define ALIA_CORE_ACTIONS_OPERATORS_HPP

#include <alia/core/actions/core.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/core.hpp>

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

template<
    class First,
    class Second,
    std::enable_if_t<
        is_action_type<First>::value && is_action_type<Second>::value,
        int> = 0>
auto
operator,(First first, Second second)
{
    return action_pair<First, Second, typename First::action_interface>(
        std::move(first), std::move(second));
}

// operator <<
//
// (a << s), where a is an action and s is a readable signal, returns another
// action that is like :a but with the value of :s bound to its first argument.
//
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
template<
    class Action,
    class Signal,
    std::enable_if_t<
        is_action_type<Action>::value
            && is_readable_signal_type<Signal>::value,
        int> = 0>
auto
operator<<(Action action, Signal signal)
{
    return bound_action<Action, Signal, typename Action::action_interface>(
        std::move(action), std::move(signal));
}
template<
    class Action,
    class Value,
    std::enable_if_t<
        is_action_type<Action>::value && !is_signal_type<Value>::value,
        int> = 0>
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

template<
    class Sink,
    class Source,
    std::enable_if_t<
        is_writable_signal_type<Sink>::value
            && is_readable_signal_type<Source>::value,
        int> = 0>
auto
operator<<=(Sink sink, Source source)
{
    return copy_action<Sink, Source>(std::move(sink), std::move(source));
}

template<
    class Sink,
    class Source,
    std::enable_if_t<
        is_writable_signal_type<Sink>::value && !is_signal_type<Source>::value,
        int> = 0>
auto
operator<<=(Sink sink, Source source)
{
    return sink <<= value(source);
}

} // namespace alia

#endif
