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
        if (this->is_readable())
        {
            id_ = make_id_by_reference(this->read());
            return id_;
        }
        return no_id;
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

// signals_all_readable(signal_a, signal_b, ...) is a variadic function that
// returns true iff all its input signals are readable.
inline bool
signals_all_readable()
{
    return true;
}
template<class Signal, class... Rest>
bool
signals_all_readable(Signal const& signal, Rest const&... rest)
{
    return signal_is_readable(signal) && signals_all_readable(rest...);
}

} // namespace alia

#endif
