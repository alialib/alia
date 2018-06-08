#ifndef ALIA_SIGNALS_BASIC_HPP
#define ALIA_SIGNALS_BASIC_HPP

// This file defines various utilities for constructing basic signals.

namespace alia {

// constant(x) creates a read-only signal which always has the value :x.
// A copy of :x is stored within the signal.
template<class Value>
struct constant_signal : signal_interface<Value, read_only_signal>
{
    constant_signal()
    {
    }
    constant_signal(Value const& v) : v_(v)
    {
    }
    id_interface const&
    id() const
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
template<class T>
input_accessor<T>
constant(T const& value)
{
    return input_accessor<T>(value);
}

// empty_signal is a signal that never has a value.
template<class Value>
struct empty_signal : signal_interface<Value, two_way_signal>
{
    empty_signal()
    {
    }
    id_interface const&
    id() const
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

// regular_signal is a partial implementation of the signal interface for
// cases where the ID of the signal is simply the value itself.
template<class Value, class Direction>
struct regular_signal : signal_interface<Value, Direction>
{
    id_interface const&
    id() const
    {
        if (this->is_readable())
        {
            id_ = make_id_by_reference(this->read());
            return id_;
        }
        return no_id;
    }

 private:
    mutable value_id_by_reference<Value> id_;
};

// make_input(x) creates a read-only signal that carries the value of the
// variable :x.
// (Unlike constant(x), this signal can change
template<class Value>
struct direct_input_signal : regular_signal<Value, read_only_signal>
{
    direct_input_signal()
    {
    }
    explicit direct_input_signal(Value const& v) : v_(v)
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
        return false;
    }
    void
    write(Value const& value) const
    {
    }

 private:
    Value* v_;
};
template<class Value>
direct_input_signal<Value>
make_input(Value const& x)
{
    return direct_input_signal<Value>(x);
}

// direct_input(x) creates a read-only signal that carries the value :x.
// (Unlike constant(x), the value of :x is allowed to change here.)
template<class Value>
struct direct_input_signal : regular_signal<Value, read_only_signal>
{
    direct_input_signal()
    {
    }
    explicit direct_input_signal(Value const& v) : v_(v)
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
direct_input_signal<Value>
direct_input(Value const& x)
{
    return direct_input_signal<Value>(x);
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

// lambda_input(lambda) creates a read-only signal whose value is produced by
// calling :lambda.
input_signal <

} // namespace alia
