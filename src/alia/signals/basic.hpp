#ifndef ALIA_SIGNALS_BASIC_HPP
#define ALIA_SIGNALS_BASIC_HPP

#include <alia/signals/core.hpp>
#include <alia/signals/utilities.hpp>

// This file defines various utilities for constructing basic signals.

namespace alia {

// empty_signal is a signal that never has a value.
template<class Value>
struct empty_signal : signal<Value, two_way_signal>
{
    empty_signal()
    {
    }
    id_interface const&
    value_id() const
    {
        return no_id;
    }
    bool
    is_readable() const
    {
        return false;
    }
    Value const&
    read() const
    {
        return *(Value*) nullptr;
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(Value const& value) const
    {
    }
};

// constant(x) creates a read-only signal which always has the value :x.
// A copy of :x is stored within the signal.
template<class Value>
struct constant_signal : signal<Value, read_only_signal>
{
    constant_signal()
    {
    }
    constant_signal(Value const& v) : v_(v)
    {
    }
    id_interface const&
    value_id() const
    {
        return unit_id;
    }
    bool
    is_readable() const
    {
        return true;
    }
    Value const&
    read() const
    {
        return v_;
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(Value const& value) const
    {
    }

 private:
    Value v_;
};
template<class Value>
constant_signal<Value>
constant(Value const& value)
{
    return constant_signal<Value>(value);
}

// value(v) creates a read-only signal that carries the value v.
// (Unlike constant(), this signal can take on different values over time.)
template<class Value>
struct value_signal : regular_signal<Value, read_only_signal>
{
    value_signal()
    {
    }
    explicit value_signal(Value const& v) : v_(v)
    {
    }
    bool
    is_readable() const
    {
        return true;
    }
    Value const&
    read() const
    {
        return v_;
    }
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(Value const& value) const
    {
    }

 private:
    Value v_;
};
template<class Value>
value_signal<Value>
value(Value const& x)
{
    return value_signal<Value>(x);
}

// direct_inout(&x) creates a two-way signal that directly exposes the value of
// the variable :x.
template<class Value>
struct direct_inout_signal : regular_signal<Value, two_way_signal>
{
    direct_inout_signal()
    {
    }
    explicit direct_inout_signal(Value* v) : v_(v)
    {
    }
    bool
    is_readable() const
    {
        return true;
    }
    Value const&
    read() const
    {
        return *v_;
    }
    bool
    is_writable() const
    {
        return true;
    }
    void
    write(Value const& value) const
    {
        *v_ = value;
    }

 private:
    Value* v_;
};
template<class Value>
direct_inout_signal<Value>
direct_inout(Value* x)
{
    return direct_inout_signal<Value>(x);
}

} // namespace alia

#endif
