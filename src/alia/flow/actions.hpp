#ifndef ALIA_FLOW_ACTIONS_HPP
#define ALIA_FLOW_ACTIONS_HPP

#include <alia/signals/basic.hpp>
#include <alia/signals/core.hpp>
#include <alia/signals/operators.hpp>

// This file defines the alia action interface, some common implementations of
// it, and some utilities for working with it.
//
// An action is essentially a response to an event that's dispatched by alia.
// When specifying a component element that can generate events, the application
// supplies the action that should be performed when the corresponding event is
// generated. Using this style allows event handling to be written in a safer
// and more reactive manner.
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
    virtual void
    perform(Args... args) const = 0;
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
    assert(action.is_ready());
    action.perform(args...);
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
    // Construct from another action_ref. - This is meant to prevent unnecessary
    // layers of indirection.
    action_ref(action_ref<Args...> const& other) : action_(other.action_)
    {
    }

    bool
    is_ready() const
    {
        return action_->is_ready();
    }

    void
    perform(Args... args) const
    {
        action_->perform(args...);
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

template<class First, class Second, class... Args>
struct action_pair : First::action_interface
{
    action_pair()
    {
    }

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
    perform(Args... args) const
    {
        first_.perform(args...);
        second_.perform(args...);
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
    return action_pair<First, Second>(first, second);
}

// operator <<=
//
// sink <<= source, where :sink and :source are both signals, creates an action
// that will set the value of :sink to the value held in :source. In order for
// the action to be considered ready, :source must be readable and :sink must be
// writable.

template<class Sink, class Source>
struct copy_action : action_interface<>
{
    copy_action(Sink const& sink, Source const& source)
        : sink_(sink), source_(source)
    {
    }

    bool
    is_ready() const
    {
        return source_.is_readable() && sink_.is_writable();
    }

    void
    perform() const
    {
        sink_.write(source_.read());
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
operator<<=(Sink const& sink, Source const& source)
{
    return copy_action<Sink, Source>(sink, source);
}

#ifndef ALIA_STRICT_OPERATORS

template<
    class Sink,
    class Source,
    std::enable_if_t<
        is_writable_signal_type<Sink>::value && !is_signal_type<Source>::value,
        int> = 0>
auto
operator<<=(Sink const& sink, Source const& source)
{
    return sink <<= value(source);
}

#endif

// For most compound assignment operators (e.g., +=), a += b, where :a and :b
// are signals, creates an action that sets :a equal to :a + :b.

#define ALIA_DEFINE_COMPOUND_ASSIGNMENT_OPERATOR(assignment_form, normal_form) \
    template<                                                                  \
        class A,                                                               \
        class B,                                                               \
        std::enable_if_t<                                                      \
            is_bidirectional_signal_type<A>::value                             \
                && is_readable_signal_type<B>::value,                          \
            int> = 0>                                                          \
    auto operator assignment_form(A const& a, B const& b)                      \
    {                                                                          \
        return a <<= (a normal_form b);                                        \
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

#ifndef ALIA_STRICT_OPERATORS

#define ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(                      \
    assignment_form, normal_form)                                              \
    template<                                                                  \
        class A,                                                               \
        class B,                                                               \
        std::enable_if_t<                                                      \
            is_bidirectional_signal_type<A>::value                             \
                && !is_signal_type<B>::value,                                  \
            int> = 0>                                                          \
    auto operator assignment_form(A const& a, B const& b)                      \
    {                                                                          \
        return a <<= (a normal_form value(b));                                 \
    }

ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(+=, +)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(-=, -)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(*=, *)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(/=, /)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(^=, ^)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(%=, %)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(&=, &)
ALIA_DEFINE_LIBERAL_COMPOUND_ASSIGNMENT_OPERATOR(|=, |)

#endif

// The increment and decrement operators work similarly.

#define ALIA_DEFINE_BY_ONE_OPERATOR(assignment_form, normal_form)              \
    template<                                                                  \
        class A,                                                               \
        std::enable_if_t<is_bidirectional_signal_type<A>::value, int> = 0>     \
    auto operator assignment_form(A const& a)                                  \
    {                                                                          \
        return a <<= (a normal_form value(typename A::value_type(1)));         \
    }                                                                          \
    template<                                                                  \
        class A,                                                               \
        std::enable_if_t<is_bidirectional_signal_type<A>::value, int> = 0>     \
    auto operator assignment_form(A const& a, int)                             \
    {                                                                          \
        return a <<= (a normal_form value(typename A::value_type(1)));         \
    }

ALIA_DEFINE_BY_ONE_OPERATOR(++, +)
ALIA_DEFINE_BY_ONE_OPERATOR(--, -)

#undef ALIA_DEFINE_BY_ONE_OPERATOR

// toggle(flag), where :flag is a signal to a boolean, creates an action that
// will toggle the value of :flag between true and false.
//
// Note that this could also be used with other value types as long as the !
// operator provides a reasonable "toggle" function.
//
template<class Flag>
auto
toggle(Flag const& flag)
{
    return flag <<= !flag;
}

// make_push_back_action(collection, item), where both :collection and :item are
// signals, creates an action that will push the value of :item onto the back
// of :collection.

template<class Collection, class Item>
struct push_back_action : action_interface<>
{
    push_back_action(Collection const& collection, Item const& item)
        : collection_(collection), item_(item)
    {
    }

    bool
    is_ready() const
    {
        return collection_.is_readable() && collection_.is_writable()
               && item_.is_readable();
    }

    void
    perform() const
    {
        auto new_collection = collection_.read();
        new_collection.push_back(item_.read());
        collection_.write(new_collection);
    }

 private:
    Collection collection_;
    Item item_;
};

template<class Collection, class Item>
auto
make_push_back_action(Collection const& collection, Item const& item)
{
    return push_back_action<Collection, Item>(collection, item);
}

// lambda_action(is_ready, perform) creates an action whose behavior is defined
// by two function objects.
//
// :is_ready takes no parameters and simply returns true or false to show if the
// action is ready to be performed.
//
// :perform can take any number/type of arguments and this defines the signature
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
    perform(Args... args) const
    {
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

// This is just a clear and concise way of indicating that a lambda action is
// always ready.
inline bool
always_ready()
{
    return true;
}

// parameterized_action(f, args...) is used to construct actions that require
// parameters that are represented as signals. :args should all be signals. The
// action won't be considered ready until all signals are readable. f() is the
// function that performs the action. Its arguments are the values read from the
// argument signals.
template<class Function, class... Args>
auto
parameterized_action(Function f, Args... args)
{
    return lambda_action(
        [=]() { return signals_all_readable(args...); },
        [=]() { return f(read_signal(args)...); });
}

} // namespace alia

#endif
