#ifndef ALIA_SIGNALS_BASIC_HPP
#define ALIA_SIGNALS_BASIC_HPP

#include <alia/signals/core.hpp>
#include <alia/signals/utilities.hpp>

// This file defines various utilities for constructing basic signals.

namespace alia {

// empty<Value>() gives a signal that never has a value.
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
    // LCOV_EXCL_START
    Value const&
    read() const
    {
        return *(Value*) nullptr;
    }
    // LCOV_EXCL_STOP
    bool
    is_writable() const
    {
        return false;
    }
    // LCOV_EXCL_START
    void
    write(Value const& value) const
    {
    }
    // LCOV_EXCL_STOP
};
template<class Value>
empty_signal<Value>
empty()
{
    return empty_signal<Value>();
}

// value(v) creates a read-only signal that carries the value v.
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

 private:
    Value v_;
};
template<class Value>
value_signal<Value>
value(Value const& v)
{
    return value_signal<Value>(v);
}

// direct(&x) creates a two-way signal that directly exposes the value of :x.
template<class Value>
struct direct_signal : regular_signal<Value, two_way_signal>
{
    direct_signal()
    {
    }
    explicit direct_signal(Value* v) : v_(v)
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
direct_signal<Value>
direct(Value* x)
{
    return direct_signal<Value>(x);
}

// text(x), where x is a string constant, creates a read-only signal for
// accessing x as a string.
struct text : signal<string, read_only_signal>
{
    text()
    {
    }
    text(char const* x) : text_(x)
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
    string const&
    read() const
    {
        return lazy_reader_.read(*this);
    }

 private:
    char const* text_;
    friend struct lazy_reader<string>;
    string
    generate() const
    {
        return string(text_);
    }
    lazy_reader<string> lazy_reader_;
};

} // namespace alia

#endif
