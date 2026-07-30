#pragma once

#include <alia/kernel/signals/core.hpp>

// This file provides utilities for implementing signals.

namespace alia {

// `regular_signal` is a partial signal implementation whose value ID is
// just the signal value itself.
template<class Derived, class Value, class Capabilities>
struct regular_signal : signal<Derived, Value, Capabilities>
{
    id_view
    value_id() const override
    {
        if (this->has_value())
            return make_id_by_reference(this->read());
        return null_id();
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
template<class Derived, class Value, class Capabilities>
struct lazy_signal : signal<Derived, Value, Capabilities>
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

} // namespace alia
