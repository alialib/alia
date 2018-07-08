#ifndef ALIA_SIGNALS_UTILITIES_HPP
#define ALIA_SIGNALS_UTILITIES_HPP

#include <alia/signals/core.hpp>

// This file defines various utilities for working with signals.
// (These are mostly meant to be used internally.)

namespace alia {

// regular_signal is a partial implementation of the signal interface for
// cases where the value ID of the signal is simply the value itself.
template<class Value, class Direction>
struct regular_signal : signal<Value, Direction>
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

// is_true(x), where x is a boolean signal, returns true iff x is readable and
// its value is true.
template<class Signal>
std::enable_if_t<is_readable_signal_type<Signal>::value, bool>
is_true(Signal const& x)
{
    return signal_is_readable(x) && read_signal(x);
}

// is_false(x), where x is a boolean signal, returns true iff x is readable and
// its value is false.
template<class Signal>
std::enable_if_t<is_readable_signal_type<Signal>::value, bool>
is_false(Signal const& x)
{
    return signal_is_readable(x) && !read_signal(x);
}

} // namespace alia

#endif
