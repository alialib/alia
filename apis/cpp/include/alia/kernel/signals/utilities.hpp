#pragma once

#include <alia/kernel/signals/core.hpp>

// This file provides utilities for implementing signals.

namespace alia {

// `regular_signal` is a partial signal implementation whose value ID is
// just the signal value itself.
template<class Derived, class Value, class Capabilities>
    requires identifiable<Value>
struct regular_signal : signal<Derived, Value, Capabilities, Value>
{
    Value const&
    value_id() const
    {
        return static_cast<Derived const*>(this)->read();
    }
};

// `lazy_signal` is a signal that lazily materializes its value via
// `move_out()`.
//
// The idea here is that `move_out()` is already defined as a function that
// returns the value *by value*, so for a signal that lazily materializes its
// value, the natural implementation of `move_out()` is to just generate that
// value and return it. `read()` and `destructive_ref()` are both required to
// return references, so we can implement those mechanically on top of
// `move_out()`.
//
template<class Derived, class Value, class Capabilities, class ValueId>
struct lazy_signal : signal<Derived, Value, Capabilities, ValueId>
{
    Value const&
    read() const override
    {
        value_ = static_cast<Derived const&>(*this).move_out();
        return value_;
    }
    Value&
    destructive_ref() const override
    {
        value_ = static_cast<Derived const&>(*this).move_out();
        return value_;
    }

 private:
    mutable Value value_;
};

// `signal_wrapper` is a utility for wrapping another signal. It's designed to
// be used as a base class. By default, it passes every signal function through
// to the wrapped signal (a protected member named `wrapped_`). You customize
// your wrapper by overriding what's different.
template<
    class Derived,
    class Wrapped,
    class Value = typename Wrapped::value_type,
    class Capabilities = typename Wrapped::capabilities,
    class ValueId = typename Wrapped::value_id_type>
struct signal_wrapper : signal<Derived, Value, Capabilities, ValueId>
{
    signal_wrapper(Wrapped wrapped) : wrapped_(std::move(wrapped))
    {
    }
    bool
    has_value() const override
    {
        return wrapped_.has_value();
    }
    typename Wrapped::value_type const&
    read() const override
    {
        return wrapped_.read();
    }
    typename Wrapped::value_type
    move_out() const override
    {
        return wrapped_.move_out();
    }
    typename Wrapped::value_type&
    destructive_ref() const override
    {
        return wrapped_.destructive_ref();
    }
    decltype(auto)
    value_id() const
    {
        return wrapped_.value_id();
    }
    bool
    ready_to_write() const override
    {
        return wrapped_.ready_to_write();
    }
    void
    write(typename Wrapped::value_type value) const override
    {
        wrapped_.write(std::move(value));
    }
    void
    clear() const override
    {
        wrapped_.clear();
    }
    bool
    invalidate(std::exception_ptr error) const override
    {
        return wrapped_.invalidate(error);
    }
    bool
    is_invalidated() const override
    {
        return wrapped_.is_invalidated();
    }

 protected:
    Wrapped wrapped_;
};

// `casting_signal_wrapper` is similar to `signal_wrapper` but it doesn't try
// to implement any functions that depend on the value type of the signal. It's
// intended for wrappers that plan to cast the wrapped signal value to a
// different type. Using `signal_wrapper` in those cases would result in errors
// in the default implementations of `read()`, `write()`, etc.
template<
    class Derived,
    class Wrapped,
    class Value,
    class Capabilities = typename Wrapped::capabilities,
    class ValueId = typename Wrapped::value_id_type>
struct casting_signal_wrapper
    : signal<Derived, Value, Capabilities, ValueId>
{
    casting_signal_wrapper(Wrapped wrapped) : wrapped_(std::move(wrapped))
    {
    }
    bool
    has_value() const override
    {
        return wrapped_.has_value();
    }
    decltype(auto)
    value_id() const
    {
        return wrapped_.value_id();
    }
    bool
    ready_to_write() const override
    {
        return wrapped_.ready_to_write();
    }
    void
    clear() const override
    {
        wrapped_.clear();
    }
    bool
    invalidate(std::exception_ptr error) const override
    {
        return wrapped_.invalidate(error);
    }
    bool
    is_invalidated() const override
    {
        return wrapped_.is_invalidated();
    }

 protected:
    Wrapped wrapped_;
};

// `lazy_signal_wrapper` is the combination of `signal_wrapper` and
// `lazy_signal`.
template<
    class Derived,
    class Wrapped,
    class Value = typename Wrapped::value_type,
    class Capabilities = typename Wrapped::capabilities,
    class ValueId = typename Wrapped::value_id_type>
struct lazy_signal_wrapper
    : signal_wrapper<Derived, Wrapped, Value, Capabilities, ValueId>
{
    lazy_signal_wrapper(Wrapped wrapped)
        : lazy_signal_wrapper::signal_wrapper(std::move(wrapped))
    {
    }
    Value const&
    read() const override
    {
        value_ = static_cast<Derived const&>(*this).move_out();
        return value_;
    }
    Value&
    destructive_ref() const override
    {
        value_ = static_cast<Derived const&>(*this).move_out();
        return value_;
    }

 private:
    mutable Value value_;
};

} // namespace alia
