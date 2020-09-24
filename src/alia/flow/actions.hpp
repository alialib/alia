#ifndef ALIA_FLOW_ACTIONS_HPP
#define ALIA_FLOW_ACTIONS_HPP

#include <alia/signals/adaptors.hpp>
#include <alia/signals/basic.hpp>
#include <alia/signals/core.hpp>

// This file defines the alia action interface, some common implementations of
// it, and some utilities for working with it.
//
// An action is essentially a response to an event that's dispatched by alia.
// When specifying a component that can generate events, the application
// supplies the action that should be performed when the corresponding event is
// generated. Using this style allows event handling to be written in a safer
// and more declarative manner.
//
// Actions are very similar to signals in the way that they are used in an
// application. Like signals, they're typically created directly at the call
// site as function arguments and are only valid for the life of the function
// call.

namespace alia {

// untyped_action_interface defines functionality common to all actions,
// irrespective of the type of arguments that the action takes.
struct untyped_action_interface
{
    // Is this action ready to be performed?
    virtual bool
    is_ready() const = 0;
};

template<class... Args>
struct action_interface : untyped_action_interface
{
    // Perform this action.
    //
    // :intermediary is used to implement the latch-like semantics of actions.
    // It should be invoked AFTER reading any signals you need to read but
    // BEFORE invoking any side effects.
    //
    virtual void
    perform(function_view<void()> const& intermediary, Args... args) const = 0;
};

// is_action_type<T>::value yields a compile-time boolean indicating whether or
// not T is an alia action type.
template<class T>
struct is_action_type : std::is_base_of<untyped_action_interface, T>
{
};

// Is the given action ready?
template<class... Args>
bool
action_is_ready(action_interface<Args...> const& action)
{
    return action.is_ready();
}

// Perform an action.
template<class... Args>
void
perform_action(action_interface<Args...> const& action, Args... args)
{
    if (action.is_ready())
        action.perform([]() {}, std::move(args)...);
}

// action_ref is a reference to an action that implements the action interface
// itself.
template<class... Args>
struct action_ref : action_interface<Args...>
{
    // Construct from a reference to another action.
    action_ref(action_interface<Args...> const& ref) : action_(&ref)
    {
    }
    // Construct from another action_ref. - This is meant to prevent
    // unnecessary layers of indirection.
    action_ref(action_ref<Args...> const& other) : action_(other.action_)
    {
    }

    bool
    is_ready() const
    {
        return action_->is_ready();
    }

    void
    perform(function_view<void()> const& intermediary, Args... args) const
    {
        action_->perform(intermediary, std::move(args)...);
    }

 private:
    action_interface<Args...> const* action_;
};

template<class... Args>
using action = action_ref<Args...>;

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
    action_pair(First const& first, Second const& second)
        : first_(first), second_(second)
    {
    }

    bool
    is_ready() const
    {
        return first_.is_ready() && second_.is_ready();
    }

    void
    perform(function_view<void()> const& intermediary, Args... args) const
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
operator,(First const& first, Second const& second)
{
    return action_pair<First, Second, typename First::action_interface>(
        first, second);
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
    is_ready() const
    {
        return action_.is_ready() && signal_.has_value();
    }

    void
    perform(function_view<void()> const& intermediary, Args... args) const
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
    is_ready() const
    {
        return source_.has_value() && sink_.ready_to_write();
    }

    void
    perform(function_view<void()> const& intermediary) const
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

// The noop_action is always ready to perform but does nothing.
template<class... Args>
struct noop_action_type : action_interface<Args...>
{
    noop_action_type()
    {
    }

    bool
    is_ready() const
    {
        return true;
    }

    void
    perform(function_view<void()> const& intermediary, Args...) const
    {
        intermediary();
    }
};
template<class... Args>
noop_action_type<Args...>
noop_action()
{
    return noop_action_type<Args...>();
}

// The unready_action is never ready to perform.
template<class... Args>
struct unready_action_type : action_interface<Args...>
{
    unready_action_type()
    {
    }

    bool
    is_ready() const
    {
        return false;
    }

    // LCOV_EXCL_START
    void
    perform(function_view<void()> const&, Args...) const
    {
        // This action is never supposed to be performed!
        assert(0);
    }
    // LCOV_EXCL_STOP
};
template<class... Args>
unready_action_type<Args...>
unready_action()
{
    return unready_action_type<Args...>();
}

// toggle(flag), where :flag is a signal to a boolean, creates an action
// that will toggle the value of :flag between true and false.
//
// Note that this could also be used with other value types as long as the !
// operator provides a reasonable "toggle" function.
//
template<class Flag>
auto
toggle(Flag flag)
{
    return flag <<= !flag;
}

// push_back(container), where :container is a signal, creates an action that
// takes an item as a parameter and pushes it onto the back of :container.

template<class Container, class Item>
struct push_back_action : action_interface<Item>
{
    push_back_action(Container container) : container_(std::move(container))
    {
    }

    bool
    is_ready() const
    {
        return container_.has_value() && container_.ready_to_write();
    }

    void
    perform(function_view<void()> const& intermediary, Item item) const
    {
        auto new_container = forward_signal(alia::move(container_));
        new_container.push_back(std::move(item));
        intermediary();
        container_.write(std::move(new_container));
    }

 private:
    Container container_;
};

template<class Container>
auto
push_back(Container container)
{
    return push_back_action<
        Container,
        typename Container::value_type::value_type>(std::move(container));
}

// lambda_action(is_ready, perform) creates an action whose behavior is
// defined by two function objects.
//
// :is_ready takes no arguments and simply returns true or false to indicate if
// the action is ready to be performed.
//
// :perform can take any number/type of arguments and defines the signature
// of the action.

template<class Function>
struct call_operator_action_signature
{
};

template<class T, class R, class... Args>
struct call_operator_action_signature<R (T::*)(Args...) const>
{
    typedef action_interface<Args...> type;
};

template<class Lambda>
struct lambda_action_signature
    : call_operator_action_signature<decltype(&Lambda::operator())>
{
};

template<class IsReady, class Perform, class Interface>
struct lambda_action_object;

template<class IsReady, class Perform, class... Args>
struct lambda_action_object<IsReady, Perform, action_interface<Args...>>
    : action_interface<Args...>
{
    lambda_action_object(IsReady is_ready, Perform perform)
        : is_ready_(is_ready), perform_(perform)
    {
    }

    bool
    is_ready() const
    {
        return is_ready_();
    }

    void
    perform(function_view<void()> const& intermediary, Args... args) const
    {
        intermediary();
        perform_(args...);
    }

 private:
    IsReady is_ready_;
    Perform perform_;
};

template<class IsReady, class Perform>
auto
lambda_action(IsReady is_ready, Perform perform)
{
    return lambda_action_object<
        IsReady,
        Perform,
        typename lambda_action_signature<Perform>::type>(is_ready, perform);
}

// The single-argument version of lambda_action creates an action that's always
// ready to perform.
template<class Perform>
auto
lambda_action(Perform perform)
{
    return lambda_action([]() { return true; }, perform);
}

// actionize(x) returns the action form of x (if it isn't already one).
// Specifically, if x is a callable object, this returns lambda_action(x).
// If x is an action, this returns x itself.
template<class Action>
std::enable_if_t<is_action_type<Action>::value, Action>
actionize(Action x)
{
    return x;
}
template<
    class Callable,
    std::enable_if_t<!is_action_type<Callable>::value, int> = 0>
auto
actionize(Callable x)
{
    return lambda_action(std::move(x));
}

// add_write_action(signal, on_write) wraps signal in a similar signal that
// will invoke on_write whenever the signal is written to. (on_write will be
// passed the value that was written to the signal.)
template<class Wrapped, class OnWrite>
struct write_action_signal
    : signal_wrapper<write_action_signal<Wrapped, OnWrite>, Wrapped>
{
    write_action_signal(Wrapped wrapped, OnWrite on_write)
        : write_action_signal::signal_wrapper(std::move(wrapped)),
          on_write_(std::move(on_write))
    {
    }
    bool
    ready_to_write() const
    {
        return this->wrapped_.ready_to_write() && on_write_.is_ready();
    }
    void
    write(typename Wrapped::value_type value) const
    {
        perform_action(on_write_, value);
        this->wrapped_.write(std::move(value));
    }

 private:
    OnWrite on_write_;
};
template<class Wrapped, class OnWrite>
auto
add_write_action(Wrapped wrapped, OnWrite on_write)
{
    return write_action_signal<Wrapped, OnWrite>(
        std::move(wrapped), std::move(on_write));
}

} // namespace alia

#endif
