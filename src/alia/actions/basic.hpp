#ifndef ALIA_ACTIONS_BASIC_HPP
#define ALIA_ACTIONS_BASIC_HPP

#include <alia/actions/core.hpp>

namespace alia {

// The noop action is always ready to perform but does nothing.

template<class... Args>
struct noop_action : action_interface<Args...>
{
    noop_action()
    {
    }

    bool
    is_ready() const override
    {
        return true;
    }

    void
    perform(function_view<void()> const& intermediary, Args...) const override
    {
        intermediary();
    }
};

namespace actions {

template<class... Args>
noop_action<Args...>
noop()
{
    return noop_action<Args...>();
}

} // namespace actions

// The unready action is never ready to perform.

template<class... Args>
struct unready_action : action_interface<Args...>
{
    unready_action()
    {
    }

    bool
    is_ready() const override
    {
        return false;
    }

    // LCOV_EXCL_START
    void
    perform(function_view<void()> const&, Args...) const override
    {
        // This action is never supposed to be performed!
        assert(0);
    }
    // LCOV_EXCL_STOP
};

namespace actions {

template<class... Args>
unready_action<Args...>
unready()
{
    return unready_action<Args...>();
}

} // namespace actions

// callback(is_ready, perform) creates an action whose behavior is defined by
// two function objects.
//
// :is_ready takes no arguments and simply returns true or false to indicate if
// the action is ready to be performed.
//
// :perform can take any number/type of arguments and defines the signature
// of the action.

namespace detail {

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
struct callback_action_signature
    : call_operator_action_signature<decltype(&Lambda::operator())>
{
};

} // namespace detail

template<class IsReady, class Perform, class Interface>
struct callback_action;

template<class IsReady, class Perform, class... Args>
struct callback_action<IsReady, Perform, action_interface<Args...>>
    : action_interface<Args...>
{
    callback_action(IsReady is_ready, Perform perform)
        : is_ready_(is_ready), perform_(perform)
    {
    }

    bool
    is_ready() const override
    {
        return is_ready_();
    }

    void
    perform(
        function_view<void()> const& intermediary, Args... args) const override
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
callback(IsReady is_ready, Perform perform)
{
    return callback_action<
        IsReady,
        Perform,
        typename detail::callback_action_signature<Perform>::type>(
        is_ready, perform);
}

// The single-argument version of callback() creates an action that's always
// ready to perform.
template<class Perform>
auto
callback(Perform perform)
{
    return callback([]() { return true; }, perform);
}

} // namespace alia

#endif
