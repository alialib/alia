#ifndef ALIA_SIGNALS_UTILITIES_HPP
#define ALIA_SIGNALS_UTILITIES_HPP

#include <alia/signals/core.hpp>

// This file defines various utilities for working with signals.
// (These are mostly meant to be used internally.)

namespace alia {

// regular_signal is a partial implementation of the signal interface for
// cases where the value ID of the signal is simply the value itself.
template<class Derived, class Value, class Capabilities>
struct regular_signal : signal<Derived, Value, Capabilities>
{
    id_interface const&
    value_id() const
    {
        if (this->has_value())
        {
            id_ = make_id_by_reference(this->read());
            return id_;
        }
        return null_id;
    }

 private:
    mutable simple_id_by_reference<Value> id_;
};

// signals_all_have_values(signal_a, signal_b, ...) is a variadic function that
// returns true iff all its input signals have values.
inline bool
signals_all_have_values()
{
    return true;
}
template<class Signal, class... Rest>
bool
signals_all_have_values(Signal const& signal, Rest const&... rest)
{
    return signal_has_value(signal) && signals_all_have_values(rest...);
}

// When assigning value IDs for signals with value type Value, should we prefer
// to use a simple ID?
template<class Value>
struct type_prefers_simple_id : std::is_fundamental<Value>
{
};

// preferred_id_signal is used to decide whether to assign a 'simple' value ID
// (one that is simply the value itself) when a more complex form is available.
// The simple form will of course eliminate spurious ID changes, but for large
// values, it might be unreasonably expensive, so this tries to use the simple
// form only for value types where it would be appropriate.

template<
    class Derived,
    class Value,
    class Capabilities,
    class ComplexId,
    class = void>
struct preferred_id_signal
{
};

template<class Derived, class Value, class Capabilities, class ComplexId>
struct preferred_id_signal<
    Derived,
    Value,
    Capabilities,
    ComplexId,
    std::enable_if_t<type_prefers_simple_id<Value>::value>>
    : signal<Derived, Value, Capabilities>
{
    id_interface const&
    value_id() const
    {
        if (this->has_value())
        {
            id_ = make_id_by_reference(this->read());
            return id_;
        }
        return null_id;
    }

 private:
    mutable simple_id_by_reference<Value> id_;
};

template<class Derived, class Value, class Capabilities, class ComplexId>
struct preferred_id_signal<
    Derived,
    Value,
    Capabilities,
    ComplexId,
    std::enable_if_t<!type_prefers_simple_id<Value>::value>>
    : signal<Derived, Value, Capabilities>
{
    id_interface const&
    value_id() const
    {
        id_ = static_cast<Derived const*>(this)->complex_value_id();
        return id_;
    }

 private:
    mutable ComplexId id_;
};

// refresh_signal_shadow is useful to monitoring a signal and responding to
// changes in its value.
template<class Signal, class OnNewValue, class OnLostValue>
void
refresh_signal_shadow(
    captured_id& id,
    Signal signal,
    OnNewValue&& on_new_value,
    OnLostValue&& on_lost_value)
{
    if (signal.has_value())
    {
        if (!id.matches(signal.value_id()))
        {
            on_new_value(signal.read());
            id.capture(signal.value_id());
        }
    }
    else
    {
        if (!id.matches(null_id))
        {
            on_lost_value();
            id.capture(null_id);
        }
    }
}

// signal_wrapper is a utility for wrapping another signal. It's designed to be
// used as a base class. By default, it passes every signal function through to
// the wrapped signal (a protected member named wrapped_). You customize your
// wrapper by overriding what's different.
template<
    class Derived,
    class Wrapped,
    class Value = typename Wrapped::value_type,
    class Capabilities = typename Wrapped::capabilities>
struct signal_wrapper : signal<Derived, Value, Capabilities>
{
    signal_wrapper(Wrapped wrapped) : wrapped_(std::move(wrapped))
    {
    }
    bool
    has_value() const
    {
        return wrapped_.has_value();
    }
    typename Wrapped::value_type const&
    read() const
    {
        return wrapped_.read();
    }
    typename Wrapped::value_type
    movable_value() const
    {
        return wrapped_.movable_value();
    }
    id_interface const&
    value_id() const
    {
        return wrapped_.value_id();
    }
    bool
    ready_to_write() const
    {
        return wrapped_.ready_to_write();
    }
    void
    write(typename Wrapped::value_type value) const
    {
        return wrapped_.write(std::move(value));
    }
    bool
    invalidate(std::exception_ptr error) const
    {
        return wrapped_.invalidate(error);
    }
    bool
    is_invalidated() const
    {
        return wrapped_.is_invalidated();
    }

 protected:
    Wrapped wrapped_;
};

// casting_signal_wrapper is similar to signal_wrapper but it doesn't try to
// implement any functions that depend on the value type of the signal. It's
// intended for wrappers that plan to cast the wrapped signal value to a
// different type. Using signal_wrapper in those cases would result in errors
// in the default implementations of read(), write(), etc.
template<
    class Derived,
    class Wrapped,
    class Value,
    class Capabilities = typename Wrapped::capabilities>
struct casting_signal_wrapper : signal<Derived, Value, Capabilities>
{
    casting_signal_wrapper(Wrapped wrapped) : wrapped_(std::move(wrapped))
    {
    }
    bool
    has_value() const
    {
        return wrapped_.has_value();
    }
    id_interface const&
    value_id() const
    {
        return wrapped_.value_id();
    }
    bool
    ready_to_write() const
    {
        return wrapped_.ready_to_write();
    }
    bool
    invalidate(std::exception_ptr error) const
    {
        return wrapped_.invalidate(error);
    }
    bool
    is_invalidated() const
    {
        return wrapped_.is_invalidated();
    }

 protected:
    Wrapped wrapped_;
};

// lazy_signal is used to create signals that lazily generate their values.
// It provides storage for the computed value and automatically implements
// read() in terms of movable_value().
template<class Derived, class Value, class Capabilities>
struct lazy_signal : signal<Derived, Value, Capabilities>
{
    Value const&
    read() const override
    {
        value_ = static_cast<Derived const&>(*this).movable_value();
        return value_;
    }

 private:
    mutable Value value_;
};

// lazy_signal_wrapper is the combination of signal_wrapper and lazy_signal.
template<
    class Derived,
    class Wrapped,
    class Value = typename Wrapped::value_type,
    class Capabilities = typename Wrapped::capabilities>
struct lazy_signal_wrapper
    : signal_wrapper<Derived, Wrapped, Value, Capabilities>
{
    lazy_signal_wrapper(Wrapped wrapped)
        : lazy_signal_wrapper::signal_wrapper(std::move(wrapped))
    {
    }
    Value const&
    read() const override
    {
        value_ = static_cast<Derived const&>(*this).movable_value();
        return value_;
    }

 private:
    mutable Value value_;
};

} // namespace alia

#endif
