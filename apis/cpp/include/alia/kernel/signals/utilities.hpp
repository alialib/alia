#pragma once

#include <alia/impl/context.hpp>

#include <alia/kernel/signals/core.hpp>

#include <optional>

// This file provides utilities for implementing signals.

namespace alia {

// `stored_signal` is a base class for signals that carry a value that is
// stored somewhere and naturally exposed by reference (via `read()`).
//
// Derived signals implement `read()`, and `read_into()` is implemented on top
// of that.
//
template<class Derived, class Value, class Capabilities, class ValueId>
struct stored_signal : signal<Derived, Value, Capabilities, ValueId>
{
    void
    read_into(Value* dst) const override
    {
        *dst = static_cast<Derived const&>(*this).read();
    }
};

// `lazy_signal` is a base class for signals that lazily materialize their
// values via `read_into()`.
//
// Derived signals implement `read_into()` to produce the value. `read()` is
// implemented mechanically on top of that.
//
template<class Derived, class Value, class Capabilities, class ValueId>
struct lazy_signal : signal<Derived, Value, Capabilities, ValueId>
{
    Value const&
    read() const override
    {
        static_cast<Derived const&>(*this).read_into(&value_);
        return value_;
    }

 private:
    mutable Value value_;
};

// `regular_signal` is a base class for signals whose value ID is the signal
// value itself.
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

// `regular_stored_signal` is the combination of `regular_signal` and
// `stored_signal`.
template<class Derived, class Value, class Capabilities>
    requires identifiable<Value>
struct regular_stored_signal
    : stored_signal<Derived, Value, Capabilities, Value>
{
    Value const&
    value_id() const
    {
        return static_cast<Derived const*>(this)->read();
    }
};

// `regular_lazy_signal` is the combination of `regular_signal` and
// `lazy_signal`.
template<class Derived, class Value, class Capabilities>
    requires identifiable<Value>
struct regular_lazy_signal : lazy_signal<Derived, Value, Capabilities, Value>
{
    Value const&
    value_id() const
    {
        return static_cast<Derived const*>(this)->read();
    }
};

// `signal_wrapper` is a base class utility for wrapping another signal type.
// By default, it passes every signal function through to the wrapper signal (a
// protected member named `wrapped_`). You customize your wrapper by overriding
// what's different.
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
    void
    read_into(typename Wrapped::value_type* dst) const override
    {
        wrapped_.read_into(dst);
    }
    typename Wrapped::value_type&
    durable_ref() const override
    {
        return wrapped_.durable_ref();
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
    std::optional<ValueId>
    post_write(
        kernel_context ctx, typename Wrapped::value_type value) const override
    {
        return wrapped_.post_write(ctx, std::move(value));
    }
    std::optional<ValueId>
    post_clear(kernel_context ctx) const override
    {
        return wrapped_.post_clear(ctx);
    }
    std::optional<ValueId>
    post_mutation_commit(kernel_context ctx) const override
    {
        return wrapped_.post_mutation_commit(ctx);
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

// `custom_id_signal_wrapper` is like `signal_wrapper` but doesn't forward
// anything related to the signal's value ID. Derived wrappers must implement
// `value_id()` and, when writable, `post_write()` and `post_clear()`.
template<
    class Derived,
    class Wrapped,
    class Value = typename Wrapped::value_type,
    class Capabilities = typename Wrapped::capabilities,
    class ValueId = typename Wrapped::value_id_type>
struct custom_id_signal_wrapper : signal<Derived, Value, Capabilities, ValueId>
{
    custom_id_signal_wrapper(Wrapped wrapped) : wrapped_(std::move(wrapped))
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
    void
    read_into(typename Wrapped::value_type* dst) const override
    {
        wrapped_.read_into(dst);
    }
    typename Wrapped::value_type&
    durable_ref() const override
    {
        return wrapped_.durable_ref();
    }
    bool
    ready_to_write() const override
    {
        return wrapped_.ready_to_write();
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
// different type.
template<
    class Derived,
    class Wrapped,
    class Value,
    class Capabilities = typename Wrapped::capabilities,
    class ValueId = typename Wrapped::value_id_type>
struct casting_signal_wrapper : signal<Derived, Value, Capabilities, ValueId>
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
    std::optional<ValueId>
    post_clear(kernel_context ctx) const override
    {
        return wrapped_.post_clear(ctx);
    }
    std::optional<ValueId>
    post_mutation_commit(kernel_context ctx) const override
    {
        return wrapped_.post_mutation_commit(ctx);
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
        static_cast<Derived const&>(*this).read_into(&value_);
        return value_;
    }

 private:
    mutable Value value_;
};

// `lazy_custom_id_signal_wrapper` is the combination of
// `custom_id_signal_wrapper` and `lazy_signal`.
template<
    class Derived,
    class Wrapped,
    class Value = typename Wrapped::value_type,
    class Capabilities = typename Wrapped::capabilities,
    class ValueId = typename Wrapped::value_id_type>
struct lazy_custom_id_signal_wrapper
    : custom_id_signal_wrapper<Derived, Wrapped, Value, Capabilities, ValueId>
{
    lazy_custom_id_signal_wrapper(Wrapped wrapped)
        : lazy_custom_id_signal_wrapper::custom_id_signal_wrapper(
              std::move(wrapped))
    {
    }
    Value const&
    read() const override
    {
        static_cast<Derived const&>(*this).read_into(&value_);
        return value_;
    }

 private:
    mutable Value value_;
};

} // namespace alia
