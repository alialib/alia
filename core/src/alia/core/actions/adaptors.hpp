#ifndef ALIA_CORE_ACTIONS_ADAPTORS_HPP
#define ALIA_CORE_ACTIONS_ADAPTORS_HPP

#include <type_traits>

#include <alia/core/actions/basic.hpp>
#include <alia/core/signals/adaptors.hpp>

namespace alia {

// only_if_ready(a), where :a is an action, returns an equivalent action that
// claims to always be ready to perform but will only actually perform :a if :a
// is ready.
//
// This is useful when you want to fold in an action if it's ready but don't
// want it to block the larger action that it's part of.

namespace detail {

template<class Wrapped, class Action>
struct only_if_ready_adaptor;

template<class Wrapped, class... Args>
struct only_if_ready_adaptor<Wrapped, action_interface<Args...>>
    : Wrapped::action_type
{
    only_if_ready_adaptor(Wrapped wrapped) : wrapped_(std::move(wrapped))
    {
    }

    bool
    is_ready() const override
    {
        return true;
    }

    void
    perform(
        function_view<void()> const& intermediary, Args... args) const override
    {
        if (wrapped_.is_ready())
            wrapped_.perform(intermediary, std::move(args)...);
        else
            intermediary();
    }

 private:
    Wrapped wrapped_;
};

} // namespace detail

template<class Wrapped>
auto
only_if_ready(Wrapped wrapped)
{
    return detail::
        only_if_ready_adaptor<Wrapped, typename Wrapped::action_type>(
            std::move(wrapped));
}

// actionize(x) returns the action form of x (if it isn't already one).
// Specifically, if x is a callable object, this returns callback(x).
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
    return callback(std::move(x));
}

// mask(a, s), where :a is an action and :s is a signal, returns an equivalent
// action that's only ready if :a is ready and :s has a value that evaluates to
// :true.
//
template<class Wrapped, class ReadinessMask, class Action>
struct action_masking_adaptor;

template<class Wrapped, class ReadinessMask, class... Args>
struct action_masking_adaptor<
    Wrapped,
    ReadinessMask,
    action_interface<Args...>> : Wrapped::action_type
{
    action_masking_adaptor(Wrapped wrapped, ReadinessMask mask)
        : wrapped_(std::move(wrapped)), mask_(std::move(mask))
    {
    }

    bool
    is_ready() const override
    {
        return wrapped_.is_ready() && signal_has_value(mask_)
               && read_signal(mask_);
    }

    void
    perform(
        function_view<void()> const& intermediary, Args... args) const override
    {
        wrapped_.perform(intermediary, std::move(args)...);
    }

 private:
    Wrapped wrapped_;
    ReadinessMask mask_;
};

template<
    class Action,
    class ReadinessMask,
    std::enable_if_t<is_action_type<Action>::value, int> = 0>
auto
mask(Action action, ReadinessMask readiness_mask)
{
    return action_masking_adaptor<
        Action,
        decltype(signalize(readiness_mask)),
        typename Action::action_type>(
        std::move(action), signalize(readiness_mask));
}

} // namespace alia

#endif
