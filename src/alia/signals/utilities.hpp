#ifndef ALIA_SIGNALS_UTILITIES_HPP
#define ALIA_SIGNALS_UTILITIES_HPP

#include <alia/signals/core.hpp>

// This file defines various utilities for working with signals.
// (These are mostly meant to be used internally.)

namespace alia {

// regular_signal is a partial implementation of the signal interface for
// cases where the value ID of the signal is simply the value itself.
template<class Derived, class Value, class Direction>
struct regular_signal : signal<Derived, Value, Direction>
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

// lazy_reader is used to create signals that lazily generate their values.
// It provides storage for the computed value and ensures that it's only
// computed once.
template<class Value>
struct lazy_reader
{
    lazy_reader() : already_generated_(false)
    {
    }
    template<class Generator>
    Value const&
    read(Generator const& generator) const
    {
        if (!already_generated_)
        {
            value_ = generator();
            already_generated_ = true;
        }
        return value_;
    }

 private:
    mutable bool already_generated_;
    mutable Value value_;
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
    class Direction,
    class ComplexId,
    class = void>
struct preferred_id_signal
{
};

template<class Derived, class Value, class Direction, class ComplexId>
struct preferred_id_signal<
    Derived,
    Value,
    Direction,
    ComplexId,
    std::enable_if_t<type_prefers_simple_id<Value>::value>>
    : signal<Derived, Value, Direction>
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

template<class Derived, class Value, class Direction, class ComplexId>
struct preferred_id_signal<
    Derived,
    Value,
    Direction,
    ComplexId,
    std::enable_if_t<!type_prefers_simple_id<Value>::value>>
    : signal<Derived, Value, Direction>
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

} // namespace alia

#endif
