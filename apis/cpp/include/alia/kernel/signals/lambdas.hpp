#pragma once

#include <alia/kernel/signals/core.hpp>
#include <alia/kernel/signals/utilities.hpp>

#include <utility>

// This file defines utilities for constructing signals from callables.

namespace alia {

// `lambda_constant(get)` creates a read-only signal whose value is constant
// and is produced by calling `get`. The signal's ID is the unit ID. `get` is
// invoked lazily, when the value is read. It's the caller's responsibility to
// ensure that `get` always returns the same value.
template<class Value, class Get>
struct lambda_constant_signal
    : lazy_signal<
          lambda_constant_signal<Value, Get>,
          Value,
          view_caps<signal_move_activated>,
          constant_value_tag>
{
    explicit lambda_constant_signal(Get get) : get_(std::move(get))
    {
    }
    bool
    has_value() const override
    {
        return true;
    }
    constant_value_tag
    value_id() const
    {
        return {};
    }
    Value
    move_out() const override
    {
        return get_();
    }

 private:
    Get get_;
};
template<class Get>
auto
lambda_constant(Get get)
{
    return lambda_constant_signal<
        std::decay_t<decltype(get())>,
        std::decay_t<Get>>(std::move(get));
}

// `lambda_view(get)` creates a read-only signal whose value is produced by
// calling `get`. The signal always has a value, and its ID is the value
// itself.
template<class Value, class Get>
    requires identifiable<Value>
struct lambda_view_signal
    : lazy_signal<
          lambda_view_signal<Value, Get>,
          Value,
          view_caps<signal_move_activated>,
          Value>
{
    explicit lambda_view_signal(Get get) : get_(std::move(get))
    {
    }
    bool
    has_value() const override
    {
        return true;
    }
    Value const&
    value_id() const
    {
        return this->read();
    }
    Value
    move_out() const override
    {
        return get_();
    }

 private:
    Get get_;
};
template<class Get>
auto
lambda_view(Get get)
    requires identifiable<std::decay_t<decltype(get())>>
{
    return lambda_view_signal<
        std::decay_t<decltype(get())>,
        std::decay_t<Get>>(std::move(get));
}

// `lambda_binding(get, set)` creates a binding whose value is produced by
// calling `get` and written by calling `set`. The signal always has a value
// and is always ready to write. Its ID is the value itself.
template<class Value, class Get, class Set>
    requires identifiable<Value>
struct lambda_binding_signal
    : lazy_signal<
          lambda_binding_signal<Value, Get, Set>,
          Value,
          binding_caps<signal_move_activated>,
          Value>
{
    lambda_binding_signal(Get get, Set set)
        : get_(std::move(get)), set_(std::move(set))
    {
    }
    bool
    has_value() const override
    {
        return true;
    }
    Value const&
    value_id() const
    {
        return this->read();
    }
    Value
    move_out() const override
    {
        return get_();
    }
    bool
    ready_to_write() const override
    {
        return true;
    }
    void
    write(Value value) const override
    {
        set_(std::move(value));
    }

 private:
    Get get_;
    Set set_;
};
template<class Get, class Set>
auto
lambda_binding(Get get, Set set)
    requires identifiable<std::decay_t<decltype(get())>>
{
    return lambda_binding_signal<
        std::decay_t<decltype(get())>,
        std::decay_t<Get>,
        std::decay_t<Set>>(std::move(get), std::move(set));
}

} // namespace alia
