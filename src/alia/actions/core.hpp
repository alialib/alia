#ifndef ALIA_ACTIONS_CORE_HPP
#define ALIA_ACTIONS_CORE_HPP

#include <alia/common.hpp>

// This file defines the core action interface.
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
    typedef action_interface action_type;

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
    is_ready() const override
    {
        return action_->is_ready();
    }

    void
    perform(
        function_view<void()> const& intermediary, Args... args) const override
    {
        action_->perform(intermediary, std::move(args)...);
    }

 private:
    action_interface<Args...> const* action_;
};

template<class... Args>
using action = action_ref<Args...>;

} // namespace alia

#endif
