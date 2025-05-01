#ifndef ALIA_CORE_ACTIONS_LIBRARY_HPP
#define ALIA_CORE_ACTIONS_LIBRARY_HPP

#include <alia/core/actions/core.hpp>
#include <alia/core/actions/operators.hpp>
#include <alia/core/signals/adaptors.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/core.hpp>

namespace alia {

// actions::toggle(flag), where :flag is a signal to a boolean, creates an
// action that will toggle the value of :flag between true and false.
//
// Note that this could also be used with other value types as long as the !
// operator provides a reasonable "toggle" function.

namespace actions {

template<class Flag>
auto
toggle(Flag flag)
{
    return flag <<= !flag;
}

} // namespace actions

// actions::push_back(container), where :container is a signal, creates an
// action that takes an item as a parameter and pushes it onto the back of
// :container.

template<class Container, class Item>
struct push_back_action : action_interface<Item>
{
    push_back_action(Container container) : container_(std::move(container))
    {
    }

    bool
    is_ready() const override
    {
        return container_.has_value() && container_.ready_to_write();
    }

    void
    perform(
        function_view<void()> const& intermediary, Item item) const override
    {
        auto new_container = forward_signal(alia::move(container_));
        new_container.push_back(std::move(item));
        intermediary();
        container_.write(std::move(new_container));
    }

 private:
    Container container_;
};

namespace actions {

template<class Container>
auto
push_back(Container container)
{
    return push_back_action<
        Container,
        typename Container::value_type::value_type>(std::move(container));
}

} // namespace actions

// actions::erase_index(container, index) creates an actions that erases the
// item at :index from :container.
// :container must be a duplex signal carrying a random access container.
// :index can be either a raw size_t or a readable signal carrying a size_t.

template<class Container, class Index>
struct erase_index_action : action_interface<>
{
    erase_index_action(Container container, Index index)
        : container_(std::move(container)), index_(std::move(index))
    {
    }

    bool
    is_ready() const override
    {
        return container_.has_value() && container_.ready_to_write()
            && index_.has_value();
    }

    void
    perform(function_view<void()> const& intermediary) const override
    {
        auto new_container = forward_signal(alia::move(container_));
        new_container.erase(new_container.begin() + read_signal(index_));
        intermediary();
        container_.write(std::move(new_container));
    }

 private:
    Container container_;
    Index index_;
};

namespace actions {

template<class Container, class Index>
auto
erase_index(Container container, Index index)
{
    auto index_signal = signalize(std::move(index));
    return erase_index_action<Container, decltype(index_signal)>(
        std::move(container), std::move(index_signal));
}

} // namespace actions

// actions::erase_key(container, key) creates an actions that erases the
// item associated with :key from :container.
// :container must be a duplex signal carrying an associative container.
// :key can be either a raw key or a readable signal carrying a key.

template<class Container, class Key>
struct erase_key_action : action_interface<>
{
    erase_key_action(Container container, Key key)
        : container_(std::move(container)), key_(std::move(key))
    {
    }

    bool
    is_ready() const override
    {
        return container_.has_value() && container_.ready_to_write()
            && key_.has_value();
    }

    void
    perform(function_view<void()> const& intermediary) const override
    {
        auto new_container = forward_signal(alia::move(container_));
        new_container.erase(read_signal(key_));
        intermediary();
        container_.write(std::move(new_container));
    }

 private:
    Container container_;
    Key key_;
};

namespace actions {

template<class Container, class Key>
auto
erase_key(Container container, Key key)
{
    auto key_signal = signalize(std::move(key));
    return erase_key_action<Container, decltype(key_signal)>(
        std::move(container), std::move(key_signal));
}

} // namespace actions

// actions::apply(f, state, [args...]), where :state is a signal, creates an
// action that will apply :f to the value of :state and write the result back
// to :state. Any :args should also be signals and will be passed along as
// additional arguments to :f.

namespace actions {

template<class Function, class PrimaryState, class... Args>
auto
apply(Function&& f, PrimaryState state, Args... args)
{
    return state <<= lazy_apply(
               std::forward<Function>(f),
               alia::move(state),
               std::move(args)...);
}

} // namespace actions

} // namespace alia

#endif
