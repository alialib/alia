#pragma once

#include <alia/kernel/actions/core.hpp>
#include <alia/kernel/effects.hpp>

#include <cassert>
#include <tuple>
#include <utility>

namespace alia {

// The noop action is always ready to post but does nothing.

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
    post(alia_context*, Args...) const override
    {
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

// The unready action is never ready to post.

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
    post(alia_context*, Args...) const override
    {
        // This action is never supposed to be posted!
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

// callback(is_ready, fn) creates an action whose behavior is defined by two
// function objects.
//
// :is_ready takes no arguments and simply returns true or false to indicate if
// the action is ready to be posted.
//
// :fn can take any number/type of arguments and defines the signature of the
// action. It runs at commit time when the posted effect is applied.

namespace detail {

template<class Function>
struct call_operator_action_signature
{
};

template<class T, class R, class... Args>
struct call_operator_action_signature<R (T::*)(Args...) const>
{
    using type = action_interface<Args...>;
};

template<class Lambda>
struct callback_action_signature
    : call_operator_action_signature<decltype(&Lambda::operator())>
{
};

} // namespace detail

template<class IsReady, class Fn, class Interface>
struct callback_action;

template<class IsReady, class Fn, class... Args>
struct callback_action<IsReady, Fn, action_interface<Args...>>
    : action_interface<Args...>
{
    callback_action(IsReady is_ready, Fn fn)
        : is_ready_(is_ready), fn_(std::move(fn))
    {
    }

    bool
    is_ready() const override
    {
        return is_ready_();
    }

    void
    post(alia_context* ctx, Args... args) const override
    {
        post_call(
            ctx,
            [fn = fn_, args = std::make_tuple(std::move(args)...)]() mutable {
                std::apply(fn, std::move(args));
            });
    }

 private:
    IsReady is_ready_;
    Fn fn_;
};

template<class IsReady, class Fn>
auto
callback(IsReady is_ready, Fn fn)
{
    return callback_action<
        IsReady,
        Fn,
        typename detail::callback_action_signature<Fn>::type>(
        is_ready, std::move(fn));
}

// The single-argument version of callback() creates an action that's always
// ready to post.
template<class Fn>
auto
callback(Fn fn)
{
    return callback([]() { return true; }, std::move(fn));
}

} // namespace alia
