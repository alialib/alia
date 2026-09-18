#pragma once

#include <alia/abi/context.h>
#include <alia/kernel/effects.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

// This file defines the core action interface.
//
// In Alia, an action is a declarative description of effects that will be
// posted in response to an event. When invoking a component that can generate
// an event, the application supplies a corresponding action. When that event
// occurs, the component invokes the action to post the desired effect(s).
//
// Actions are very similar to signals in the way that they are used in an
// application. Like signals, they're typically created directly at the call
// site as function arguments and are only valid for the life of the function
// call.

namespace alia {

// `untyped_action_interface` defines functionality common to all actions,
// irrespective of the type of arguments that the action takes.
struct untyped_action_interface
{
    // Is this action ready to be posted?
    virtual bool
    is_ready() const = 0;
};

template<class... Args>
struct action_interface : untyped_action_interface
{
    using action_type = action_interface;

    // Read any inputs and post the desired effects.
    virtual void
    post(alia_context* ctx, Args... args) const = 0;
};

// `action_type<T>` is true iff `T` is an Alia action type.
// The `sizeof` check keeps this from being a hard error on incomplete types
// (e.g. `std::ostream` when `operator<<` is considered for stringification).
template<class T>
concept action_type = requires {
    sizeof(T);
    requires std::is_base_of_v<untyped_action_interface, T>;
};

// Is the given action ready?
template<class... Args>
bool
action_is_ready(action_interface<Args...> const& action)
{
    return action.is_ready();
}

// Post an action's effects.
template<class... Args>
void
post_action(
    alia_context* ctx, action_interface<Args...> const& action, Args... args)
{
    if (action.is_ready())
        action.post(ctx, std::move(args)...);
}

// `action_ref` is a reference to an action that implements the action
// interface itself.
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
    is_ready() const override
    {
        return action_->is_ready();
    }

    void
    post(alia_context* ctx, Args... args) const override
    {
        action_->post(ctx, std::move(args)...);
    }

 private:
    action_interface<Args...> const* action_;
};

template<class... Args>
using action = action_ref<Args...>;

} // namespace alia
